#include <opengl_helpers.hpp>

#include <XPLMUtilities.h>
#include <XPLMGraphics.h>

#include <png.h>

#include <fstream>

GLsizei integer_square_root(GLsizei input_value)
{
	GLsizei output_value = 0;
	while ((output_value * output_value) < input_value) output_value++;

	return output_value;
}

void read_stream_callback(png_structp png_struct, png_bytep output_values, size_t read_size)
{
	std::ifstream& input_stream = *static_cast<std::ifstream*>(png_get_io_ptr(png_struct));

	input_stream.read(reinterpret_cast<char*>(output_values), read_size);
}

GLuint load_shader(char* shader_path, GLenum shader_type)
{
	std::ifstream shader_file(shader_path, std::ifstream::binary | std::ifstream::ate);

	if (shader_file.fail() == true)
	{
		XPLMDebugString("Could not open shader file!");

		XPLMDebugString("Shader file path is:");
		XPLMDebugString(shader_path);

		return SHADER_INVALID;
	}

	GLint shader_file_size = shader_file.tellg();

	shader_file.seekg(0);

	GLchar* shader_string = new GLchar[shader_file_size];
	shader_file.read(shader_string, shader_file_size);

	GLuint shader_reference = glCreateShader(shader_type);
	const GLchar* const_shader_string = shader_string;
	glShaderSource(shader_reference, 1, &const_shader_string, &shader_file_size);
	glCompileShader(shader_reference);

	delete[] shader_string;

	GLint shader_compilation_status;
	glGetShaderiv(shader_reference, GL_COMPILE_STATUS, &shader_compilation_status);

	if (shader_compilation_status == GL_FALSE)
	{
		GLint compilation_log_length;
		glGetShaderiv(shader_reference, GL_INFO_LOG_LENGTH, &compilation_log_length);

		GLchar* compilation_message = new GLchar[compilation_log_length];
		glGetShaderInfoLog(shader_reference, compilation_log_length, nullptr, compilation_message);

		XPLMDebugString("Shader compilation failed!");

		XPLMDebugString("Compilation error message is:");
		XPLMDebugString(compilation_message);

		delete[] compilation_message;

		return SHADER_INVALID;
	}

	return shader_reference;
}

int create_texture(GLsizei texture_width, GLsizei texture_height, GLint internal_format, GLenum data_format, void* texture_data)
{
	int texture_reference;
	XPLMGenerateTextureNumbers(&texture_reference, 1);

	XPLMBindTexture2d(texture_reference, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture_width, texture_height, 0, data_format, GL_UNSIGNED_BYTE, texture_data);

	XPLMBindTexture2d(TEXTURE_INVALID, 0);

	return texture_reference;
}

int create_texture(GLsizei texture_width, GLsizei texture_height, GLsizei texture_depth, GLint internal_format, GLenum data_format, void* texture_data)
{
	int texture_reference;
	XPLMGenerateTextureNumbers(&texture_reference, 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, texture_reference);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage3D(GL_TEXTURE_3D, 0, internal_format, texture_width, texture_height, texture_depth, 0, data_format, GL_UNSIGNED_BYTE, texture_data);

	XPLMBindTexture2d(TEXTURE_INVALID, 0);

	return texture_reference;
}

int load_png_texture(char* texture_path, bool create_3d_texture)
{
	std::ifstream texture_file(texture_path, std::ifstream::binary);

	if (texture_file.fail() == true)
	{
		XPLMDebugString("Could not open texture file!");

		XPLMDebugString("Texture file path is:");
		XPLMDebugString(texture_path);

		return TEXTURE_INVALID;
	}

	png_structp png_struct = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	png_infop png_info = png_create_info_struct(png_struct);

	png_set_read_fn(png_struct, &texture_file, read_stream_callback);

	png_read_info(png_struct, png_info);

	png_set_scale_16(png_struct);
	png_set_expand(png_struct);

	GLsizei image_width = png_get_image_width(png_struct, png_info);
	GLsizei image_height = png_get_image_height(png_struct, png_info);

	png_byte color_type = png_get_color_type(png_struct, png_info);

	GLsizei bytes_per_pixel;

	if (color_type == PNG_COLOR_TYPE_GRAY) bytes_per_pixel = 1;
	else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) bytes_per_pixel = 2;
	else if (color_type == PNG_COLOR_TYPE_RGB) bytes_per_pixel = 3;
	else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) bytes_per_pixel = 4;

	png_bytep texture_data = new png_byte[image_width * image_height * bytes_per_pixel];
	png_bytepp texture_row_pointers = new png_bytep[image_height];

	for (GLsizei row_index = 0; row_index < image_height; row_index++) texture_row_pointers[row_index] = texture_data + (row_index * image_width * bytes_per_pixel);
	png_read_image(png_struct, texture_row_pointers);

	GLint texture_format;

	if (bytes_per_pixel == 1) texture_format = GL_RED;
	else if (bytes_per_pixel == 2) texture_format = GL_RG;
	else if (bytes_per_pixel == 3) texture_format = GL_RGB;
	else if (bytes_per_pixel == 4) texture_format = GL_RGBA;

	int texture_reference;

	if (create_3d_texture == true) texture_reference = create_texture(image_width, integer_square_root(image_height), integer_square_root(image_height), texture_format, texture_format, texture_data);
	else texture_reference = create_texture(image_width, image_height, texture_format, texture_format, texture_data);

	delete[] texture_row_pointers;
	delete[] texture_data;

	png_destroy_read_struct(&png_struct, &png_info, nullptr);

	return texture_reference;
}