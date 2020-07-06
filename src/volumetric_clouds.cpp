#ifdef IBM
#include <Windows.h>
#endif

#include <XPLMDataAccess.h>
#include <XPLMDisplay.h>
#include <XPLMGraphics.h>

#include <GL/glew.h>

#include <dataref_helpers.hpp>
#include <opengl_helpers.hpp>

#include <cstring>
#include <cmath>

#define INFORMATION_BUFFER_SIZE 256

#define CLOUD_LAYER_COUNT 3
#define CLOUD_TYPE_COUNT 5

#define RADIANS_PER_DEGREES 0.01745329251994329576

XPLMDataRef reverse_z_dataref;

XPLMDataRef viewport_dataref;

XPLMDataRef modelview_matrix_dataref;
XPLMDataRef projection_matrix_dataref;

XPLMDataRef cloud_map_scale_dataref;

XPLMDataRef base_noise_scale_dataref;
XPLMDataRef detail_noise_scale_dataref;

XPLMDataRef blue_noise_scale_dataref;

XPLMDataRef cloud_base_datarefs[CLOUD_LAYER_COUNT];
XPLMDataRef cloud_height_datarefs[CLOUD_TYPE_COUNT];

XPLMDataRef cloud_type_datarefs[CLOUD_LAYER_COUNT];
XPLMDataRef cloud_coverage_datarefs[CLOUD_TYPE_COUNT];

XPLMDataRef base_noise_ratio_datarefs[CLOUD_TYPE_COUNT];
XPLMDataRef detail_noise_ratio_datarefs[CLOUD_TYPE_COUNT];

XPLMDataRef cloud_density_datarefs[CLOUD_TYPE_COUNT];

XPLMDataRef cloud_tint_dataref;

XPLMDataRef fade_start_distance_dataref;
XPLMDataRef fade_end_distance_dataref;

XPLMDataRef light_attenuation_dataref;

XPLMDataRef sun_pitch_dataref;
XPLMDataRef sun_heading_dataref;

XPLMDataRef sun_tint_red_dataref;
XPLMDataRef sun_tint_green_dataref;
XPLMDataRef sun_tint_blue_dataref;

XPLMDataRef sun_gain_dataref;

XPLMDataRef atmosphere_tint_dataref;
XPLMDataRef atmospheric_blending_dataref;

XPLMDataRef forward_mie_scattering_dataref;
XPLMDataRef backward_mie_scattering_dataref;

XPLMDataRef local_time_dataref;

GLfloat quad_vertices[] =
{
	-1.0, -1.0,
	-1.0, 1.0,
	1.0, -1.0,

	1.0, -1.0,
	-1.0, 1.0,
	1.0, 1.0
};

GLuint shader_program;

GLuint vertex_array_object;

GLsizei current_viewport_width;
GLsizei current_viewport_height;

int depth_texture;

int cloud_map_texture;

int base_noise_texture;
int detail_noise_texture;

int blue_noise_texture;

GLint shader_near_clip_z;
GLint shader_far_clip_z;

GLint shader_modelview_matrix;
GLint shader_projection_matrix;

GLint shader_cloud_map_scale;

GLint shader_base_noise_scale;
GLint shader_detail_noise_scale;

GLint shader_blue_noise_scale;

GLint shader_cloud_bases;
GLint shader_cloud_heights;

GLint shader_cloud_types;
GLint shader_cloud_coverages;

GLint shader_base_noise_ratios;
GLint shader_detail_noise_ratios;

GLint shader_cloud_densities;

GLint shader_cloud_tint;

GLint shader_fade_start_distance;
GLint shader_fade_end_distance;

GLint shader_light_attenuation;

GLint shader_sun_direction;

GLint shader_sun_tint;
GLint shader_sun_gain;

GLint shader_atmosphere_tint;
GLint shader_atmospheric_blending;

GLint shader_forward_mie_scattering;
GLint shader_backward_mie_scattering;

GLint shader_local_time;

#ifdef IBM
BOOL APIENTRY DllMain(IN HINSTANCE dll_handle, IN DWORD call_reason, IN LPVOID reserved)
{
	return TRUE;
}
#endif

int draw_callback(XPLMDrawingPhase drawing_phase, int is_before, void* callback_reference)
{
	if (is_before == 0)
	{
		XPLMSetGraphicsState(0, 5, 0, 0, 1, 0, 0);
		glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		int reverse_z = XPLMGetDatai(reverse_z_dataref);

		XPLMBindTexture2d(depth_texture, 0); 

		int viewport_coordinates[4];
		XPLMGetDatavi(viewport_dataref, viewport_coordinates, 0, 4);

		GLsizei new_viewport_x = viewport_coordinates[0];
		GLsizei new_viewport_y = viewport_coordinates[1];

		GLsizei new_viewport_width = viewport_coordinates[2] - viewport_coordinates[0];
		GLsizei new_viewport_height = viewport_coordinates[3] - viewport_coordinates[1];

		GLenum internal_format;

		if (reverse_z == 0) internal_format = GL_DEPTH_COMPONENT24;
		else internal_format = GL_DEPTH_COMPONENT32F;

		if ((current_viewport_width != new_viewport_width) || (current_viewport_height != new_viewport_height)) glCopyTexImage2D(GL_TEXTURE_2D, 0, internal_format, new_viewport_x, new_viewport_y, new_viewport_width, new_viewport_height, 0);
		else glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, new_viewport_x, new_viewport_y, new_viewport_width, new_viewport_height);

		current_viewport_width = new_viewport_width;
		current_viewport_height = new_viewport_height;

		XPLMBindTexture2d(cloud_map_texture, 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_3D, base_noise_texture);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_3D, detail_noise_texture);

		XPLMBindTexture2d(blue_noise_texture, 4);

		glBindVertexArray(vertex_array_object);

		glUseProgram(shader_program);

		if (reverse_z == 0)
		{
			glUniform1f(shader_near_clip_z, -1.0);
			glUniform1f(shader_far_clip_z, 1.0);
		}
		else
		{
			glUniform1f(shader_near_clip_z, 1.0);
			glUniform1f(shader_far_clip_z, 0.0);
		}

		float modelview_matrix[16];
		float projection_matrix[16];

		XPLMGetDatavf(modelview_matrix_dataref, modelview_matrix, 0, 16);
		XPLMGetDatavf(projection_matrix_dataref, projection_matrix, 0, 16);

		glUniformMatrix4fv(shader_modelview_matrix, 1, GL_FALSE, modelview_matrix);
		glUniformMatrix4fv(shader_projection_matrix, 1, GL_FALSE, projection_matrix);

		glUniform1f(shader_cloud_map_scale, XPLMGetDataf(cloud_map_scale_dataref));

		glUniform1f(shader_base_noise_scale, XPLMGetDataf(base_noise_scale_dataref));
		glUniform1f(shader_detail_noise_scale, XPLMGetDataf(detail_noise_scale_dataref));

		glUniform1f(shader_blue_noise_scale, XPLMGetDataf(blue_noise_scale_dataref));

		float cloud_bases[CLOUD_LAYER_COUNT];
		float cloud_heights[CLOUD_TYPE_COUNT];

		for (size_t layer_index = 0; layer_index < CLOUD_LAYER_COUNT; layer_index++) cloud_bases[layer_index] = XPLMGetDataf(cloud_base_datarefs[layer_index]);
		for (size_t type_index = 0; type_index < CLOUD_TYPE_COUNT; type_index++) cloud_heights[type_index] = XPLMGetDataf(cloud_height_datarefs[type_index]);

		glUniform1fv(shader_cloud_bases, CLOUD_LAYER_COUNT, cloud_bases);
		glUniform1fv(shader_cloud_heights, CLOUD_TYPE_COUNT, cloud_heights);

		int cloud_types[CLOUD_LAYER_COUNT];
		float cloud_coverages[CLOUD_TYPE_COUNT];

		for (size_t layer_index = 0; layer_index < CLOUD_LAYER_COUNT; layer_index++) cloud_types[layer_index] = XPLMGetDatai(cloud_type_datarefs[layer_index]);
		for (size_t type_index = 0; type_index < CLOUD_TYPE_COUNT; type_index++) cloud_coverages[type_index] = XPLMGetDataf(cloud_coverage_datarefs[type_index]);

		glUniform1iv(shader_cloud_types, CLOUD_LAYER_COUNT, cloud_types);
		glUniform1fv(shader_cloud_coverages, CLOUD_TYPE_COUNT, cloud_coverages);

		float base_noise_ratios[CLOUD_TYPE_COUNT][3];
		float detail_noise_ratios[CLOUD_TYPE_COUNT][3];

		for (size_t type_index = 0; type_index < CLOUD_TYPE_COUNT; type_index++)
		{
			XPLMGetDatavf(base_noise_ratio_datarefs[type_index], base_noise_ratios[type_index], 0, 3);
			XPLMGetDatavf(detail_noise_ratio_datarefs[type_index], detail_noise_ratios[type_index], 0, 3);
		}

		glUniform3fv(shader_base_noise_ratios, CLOUD_TYPE_COUNT, reinterpret_cast<GLfloat*>(base_noise_ratios));
		glUniform3fv(shader_detail_noise_ratios, CLOUD_TYPE_COUNT, reinterpret_cast<GLfloat*>(detail_noise_ratios));

		float cloud_densities[CLOUD_TYPE_COUNT];
		for (size_t type_index = 0; type_index < CLOUD_TYPE_COUNT; type_index++) cloud_densities[type_index] = XPLMGetDataf(cloud_density_datarefs[type_index]);

		glUniform1fv(shader_cloud_densities, CLOUD_TYPE_COUNT, cloud_densities);

		float cloud_tint[3];
		XPLMGetDatavf(cloud_tint_dataref, cloud_tint, 0, 3);

		glUniform3fv(shader_cloud_tint, 1, cloud_tint);

		fade_start_distance_dataref = XPLMFindDataRef("sim/private/stats/skyc/fog/near_fog_cld");
		fade_end_distance_dataref = XPLMFindDataRef("sim/private/stats/skyc/fog/far_fog_cld");

		glUniform1f(shader_fade_start_distance, XPLMGetDataf(fade_start_distance_dataref));
		glUniform1f(shader_fade_end_distance, XPLMGetDataf(fade_end_distance_dataref));

		glUniform1f(shader_light_attenuation, 1.0 - XPLMGetDataf(light_attenuation_dataref));

		float sun_pitch = XPLMGetDataf(sun_pitch_dataref) * RADIANS_PER_DEGREES;
		float sun_heading = XPLMGetDataf(sun_heading_dataref) * RADIANS_PER_DEGREES;

		glUniform3f(shader_sun_direction, cos(sun_pitch) * sin(sun_heading), sin(sun_pitch), -1.0 * cos(sun_pitch) * cos(sun_heading));

		glUniform3f(shader_sun_tint, XPLMGetDataf(sun_tint_red_dataref), XPLMGetDataf(sun_tint_green_dataref), XPLMGetDataf(sun_tint_blue_dataref));
		glUniform1f(shader_sun_gain, XPLMGetDataf(sun_gain_dataref));

		float atmosphere_tint[3];
		XPLMGetDatavf(atmosphere_tint_dataref, atmosphere_tint, 0, 3);

		glUniform3fv(shader_atmosphere_tint, 1, atmosphere_tint);
		glUniform1f(shader_atmospheric_blending, XPLMGetDataf(atmospheric_blending_dataref));

		glUniform1f(shader_forward_mie_scattering, XPLMGetDataf(forward_mie_scattering_dataref));
		glUniform1f(shader_backward_mie_scattering, XPLMGetDataf(backward_mie_scattering_dataref));

		glUniform1f(shader_local_time, XPLMGetDataf(local_time_dataref));

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glUseProgram(0);

		glBindVertexArray(0);

		XPLMBindTexture2d(TEXTURE_INVALID, 0);

		XPLMBindTexture2d(TEXTURE_INVALID, 1);

		XPLMBindTexture2d(TEXTURE_INVALID, 2);
		XPLMBindTexture2d(TEXTURE_INVALID, 3);

		XPLMBindTexture2d(TEXTURE_INVALID, 4);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	return 1;
}

PLUGIN_API int XPluginStart(char* plugin_name, char* plugin_signature, char* plugin_description)
{
	std::strcpy(plugin_name, "Volumetric Clouds");
	std::strcpy(plugin_signature, "FarukEroglu2048.volumetric_clouds");
	std::strcpy(plugin_description, "Volumetric Clouds for X-Plane 11");

	viewport_dataref = XPLMFindDataRef("sim/graphics/view/viewport");

	reverse_z_dataref = XPLMFindDataRef("sim/graphics/view/is_reverse_float_z");

	modelview_matrix_dataref = XPLMFindDataRef("sim/graphics/view/world_matrix");
	projection_matrix_dataref = XPLMFindDataRef("sim/graphics/view/projection_matrix");

	cloud_map_scale_dataref = export_float_dataref("volumetric_clouds/cloud_map_scale", 0.000005);

	base_noise_scale_dataref = export_float_dataref("volumetric_clouds/base_noise_scale", 0.00005);
	detail_noise_scale_dataref = export_float_dataref("volumetric_clouds/detail_noise_scale", 0.0005);

	blue_noise_scale_dataref = export_float_dataref("volumetric_clouds/blue_noise_scale", 0.01);

	cloud_base_datarefs[0] = XPLMFindDataRef("sim/weather/cloud_base_msl_m[0]");
	cloud_base_datarefs[1] = XPLMFindDataRef("sim/weather/cloud_base_msl_m[1]");
	cloud_base_datarefs[2] = XPLMFindDataRef("sim/weather/cloud_base_msl_m[2]");

	cloud_height_datarefs[0] = export_float_dataref("volumetric_clouds/cirrus/height", 1000.0);
	cloud_height_datarefs[1] = export_float_dataref("volumetric_clouds/scattered/height", 2500.0);
	cloud_height_datarefs[2] = export_float_dataref("volumetric_clouds/broken/height", 3000.0);
	cloud_height_datarefs[3] = export_float_dataref("volumetric_clouds/overcast/height", 3000.0);
	cloud_height_datarefs[4] = export_float_dataref("volumetric_clouds/stratus/height", 3500.0);

	cloud_type_datarefs[0] = XPLMFindDataRef("sim/weather/cloud_type[0]");
	cloud_type_datarefs[1] = XPLMFindDataRef("sim/weather/cloud_type[1]");
	cloud_type_datarefs[2] = XPLMFindDataRef("sim/weather/cloud_type[2]");

	cloud_coverage_datarefs[0] = export_float_dataref("volumetric_clouds/cirrus/coverage", 0.25);
	cloud_coverage_datarefs[1] = export_float_dataref("volumetric_clouds/scattered/coverage", 0.65);
	cloud_coverage_datarefs[2] = export_float_dataref("volumetric_clouds/broken/coverage", 0.85);
	cloud_coverage_datarefs[3] = export_float_dataref("volumetric_clouds/overcast/coverage", 1.0);
	cloud_coverage_datarefs[4] = export_float_dataref("volumetric_clouds/stratus/coverage", 1.25);

	base_noise_ratio_datarefs[0] = export_float_vector_dataref("volumetric_clouds/cirrus/base_noise_ratios", {0.625, 0.25, 0.125});
	base_noise_ratio_datarefs[1] = export_float_vector_dataref("volumetric_clouds/scattered/base_noise_ratios", {0.625, 0.25, 0.125});
	base_noise_ratio_datarefs[2] = export_float_vector_dataref("volumetric_clouds/broken/base_noise_ratios", {0.625, 0.25, 0.125});
	base_noise_ratio_datarefs[3] = export_float_vector_dataref("volumetric_clouds/overcast/base_noise_ratios", {0.625, 0.25, 0.125});
	base_noise_ratio_datarefs[4] = export_float_vector_dataref("volumetric_clouds/stratus/base_noise_ratios", {0.625, 0.25, 0.125});

	detail_noise_ratio_datarefs[0] = export_float_vector_dataref("volumetric_clouds/cirrus/detail_noise_ratios", {0.625, 0.25, 0.125});
	detail_noise_ratio_datarefs[1] = export_float_vector_dataref("volumetric_clouds/scattered/detail_noise_ratios", {0.625, 0.25, 0.125});
	detail_noise_ratio_datarefs[2] = export_float_vector_dataref("volumetric_clouds/broken/detail_noise_ratios", {0.625, 0.25, 0.125});
	detail_noise_ratio_datarefs[3] = export_float_vector_dataref("volumetric_clouds/overcast/detail_noise_ratios", {0.625, 0.25, 0.125});
	detail_noise_ratio_datarefs[4] = export_float_vector_dataref("volumetric_clouds/stratus/detail_noise_ratios", {0.625, 0.25, 0.125});

	cloud_density_datarefs[0] = export_float_dataref("volumetric_clouds/cirrus/density", 1.5);
	cloud_density_datarefs[1] = export_float_dataref("volumetric_clouds/scattered/density", 1.5);
	cloud_density_datarefs[2] = export_float_dataref("volumetric_clouds/broken/density", 2.5);
	cloud_density_datarefs[3] = export_float_dataref("volumetric_clouds/overcast/density", 3.5);
	cloud_density_datarefs[4] = export_float_dataref("volumetric_clouds/stratus/density", 4.5);

	cloud_tint_dataref = export_float_vector_dataref("volumetric_clouds/cloud_tint", {0.95, 0.95, 0.95});

	light_attenuation_dataref = XPLMFindDataRef("sim/graphics/misc/light_attenuation");

	sun_pitch_dataref = XPLMFindDataRef("sim/graphics/scenery/sun_pitch_degrees");
	sun_heading_dataref = XPLMFindDataRef("sim/graphics/scenery/sun_heading_degrees");

	sun_tint_red_dataref = XPLMFindDataRef("sim/graphics/misc/outside_light_level_r");
	sun_tint_green_dataref = XPLMFindDataRef("sim/graphics/misc/outside_light_level_g");
	sun_tint_blue_dataref = XPLMFindDataRef("sim/graphics/misc/outside_light_level_b");

	sun_gain_dataref = export_float_dataref("volumetric_clouds/sun_gain", 3.0);

	atmosphere_tint_dataref = export_float_vector_dataref("volumetric_clouds/atmosphere_tint", {0.35, 0.575, 1.0});
	atmospheric_blending_dataref = export_float_dataref("volumetric_clouds/atmospheric_blending", 0.15);

	forward_mie_scattering_dataref = export_float_dataref("volumetric_clouds/forward_mie_scattering", 0.78);
	backward_mie_scattering_dataref = export_float_dataref("volumetric_clouds/backward_mie_scattering", 0.25);

	local_time_dataref = XPLMFindDataRef("sim/time/local_time_sec");

	XPLMDataRef override_clouds_dataref = XPLMFindDataRef("sim/operation/override/override_clouds");
	XPLMSetDatai(override_clouds_dataref, 1);

	glewInit();

	XPLMSetGraphicsState(0, 1, 0, 0, 0, 0, 0);

	XPLMGenerateTextureNumbers(&depth_texture, 1);
	XPLMBindTexture2d(depth_texture, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	XPLMBindTexture2d(TEXTURE_INVALID, 0);

	cloud_map_texture = load_png_texture("Resources/plugins/Volumetric Clouds/textures/cloud_map.png", false);

	base_noise_texture = load_png_texture("Resources/plugins/Volumetric Clouds/textures/base_noise.png", true);
	detail_noise_texture = load_png_texture("Resources/plugins/Volumetric Clouds/textures/detail_noise.png", true);

	blue_noise_texture = load_png_texture("Resources/plugins/Volumetric Clouds/textures/blue_noise.png", false);

	glGenVertexArrays(1, &vertex_array_object);
	glBindVertexArray(vertex_array_object);

	GLuint vertex_buffer;
	glGenBuffers(1, &vertex_buffer);

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	GLuint vertex_shader = load_shader("Resources/plugins/Volumetric Clouds/shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
	GLuint fragment_shader = load_shader("Resources/plugins/Volumetric Clouds/shaders/fragment_shader.glsl", GL_FRAGMENT_SHADER);

	shader_program = glCreateProgram();

	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);

	glLinkProgram(shader_program);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	glUseProgram(shader_program);

	GLint shader_depth_texture = glGetUniformLocation(shader_program, "depth_texture");

	GLint shader_cloud_map_texture = glGetUniformLocation(shader_program, "cloud_map_texture");

	GLint shader_base_noise_texture = glGetUniformLocation(shader_program, "base_noise_texture");
	GLint shader_detail_noise_texture = glGetUniformLocation(shader_program, "detail_noise_texture");

	GLint shader_blue_noise_texture = glGetUniformLocation(shader_program, "blue_noise_texture");

	shader_near_clip_z = glGetUniformLocation(shader_program, "near_clip_z");
	shader_far_clip_z = glGetUniformLocation(shader_program, "far_clip_z");

	shader_modelview_matrix = glGetUniformLocation(shader_program, "modelview_matrix");
	shader_projection_matrix = glGetUniformLocation(shader_program, "projection_matrix");

	shader_cloud_map_scale = glGetUniformLocation(shader_program, "cloud_map_scale");

	shader_base_noise_scale = glGetUniformLocation(shader_program, "base_noise_scale");
	shader_detail_noise_scale = glGetUniformLocation(shader_program, "detail_noise_scale");

	shader_blue_noise_scale = glGetUniformLocation(shader_program, "blue_noise_scale");

	shader_cloud_bases = glGetUniformLocation(shader_program, "cloud_bases");
	shader_cloud_heights = glGetUniformLocation(shader_program, "cloud_heights");

	shader_cloud_types = glGetUniformLocation(shader_program, "cloud_types");
	shader_cloud_coverages = glGetUniformLocation(shader_program, "cloud_coverages");

	shader_base_noise_ratios = glGetUniformLocation(shader_program, "base_noise_ratios");
	shader_detail_noise_ratios = glGetUniformLocation(shader_program, "detail_noise_ratios");

	shader_cloud_densities = glGetUniformLocation(shader_program, "cloud_densities");

	shader_cloud_tint = glGetUniformLocation(shader_program, "cloud_tint");

	shader_fade_start_distance = glGetUniformLocation(shader_program, "fade_start_distance");
	shader_fade_end_distance = glGetUniformLocation(shader_program, "fade_end_distance");

	shader_light_attenuation = glGetUniformLocation(shader_program, "light_attenuation");

	shader_sun_direction = glGetUniformLocation(shader_program, "sun_direction");

	shader_sun_tint = glGetUniformLocation(shader_program, "sun_tint");
	shader_sun_gain = glGetUniformLocation(shader_program, "sun_gain");

	shader_atmosphere_tint = glGetUniformLocation(shader_program, "atmosphere_tint");
	shader_atmospheric_blending = glGetUniformLocation(shader_program, "atmospheric_blending");

	shader_forward_mie_scattering = glGetUniformLocation(shader_program, "forward_mie_scattering");
	shader_backward_mie_scattering = glGetUniformLocation(shader_program, "backward_mie_scattering");

	shader_local_time = glGetUniformLocation(shader_program, "local_time");

	glUniform1i(shader_depth_texture, 0);

	glUniform1i(shader_cloud_map_texture, 1);

	glUniform1i(shader_base_noise_texture, 2);
	glUniform1i(shader_detail_noise_texture, 3);

	glUniform1i(shader_blue_noise_texture, 4);

	glUseProgram(0);

	#ifdef XPLM303
	XPLMRegisterDrawCallback(draw_callback, xplm_Phase_Modern3D, 0, nullptr);
	#else
	XPLMRegisterDrawCallback(draw_callback, xplm_Phase_Airplanes, 0, nullptr);
	#endif

	return 1;
}

PLUGIN_API void XPluginStop(void)
{
	clean_datarefs();
}

PLUGIN_API int XPluginEnable(void)
{
	return 1;
}

PLUGIN_API void XPluginDisable(void)
{

}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID sender_plugin, int message_type, void* callback_parameters)
{
	if (message_type == XPLM_MSG_PLANE_LOADED){
		notify_datarefs();
	}
}