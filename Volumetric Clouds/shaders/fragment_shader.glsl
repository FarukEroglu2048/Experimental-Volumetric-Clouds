#version 410 core

#define EARTH_RADIUS 6378100.0
#define EARTH_CENTER vec3(0.0, -1.0 * EARTH_RADIUS, 0.0)

#define SAMPLE_STEP_COUNT 64
#define SUN_STEP_COUNT 4

#define MAXIMUM_SAMPLE_STEP_SIZE 100.0

#define WIND_LAYER_COUNT 3
#define CLOUD_LAYER_COUNT 3
#define CLOUD_TYPE_COUNT 5

#define PI 3.141592653589793

flat in mat4 inverse_modelview_matrix;
flat in mat4 inverse_projection_matrix;

in vec3 ray_start_position;
in vec3 ray_end_position;

in vec2 depth_texture_position;

uniform sampler2D depth_texture;

uniform sampler2D cloud_map_texture;

uniform sampler3D base_noise_texture;
uniform sampler3D detail_noise_texture;

uniform sampler2D blue_noise_texture;

uniform float near_clip_z;
uniform float far_clip_z;

uniform float cloud_map_scale;

uniform float base_noise_scale;
uniform float detail_noise_scale;

uniform float blue_noise_scale;

uniform float[CLOUD_LAYER_COUNT] cloud_bases;
uniform float[CLOUD_TYPE_COUNT] cloud_heights;

uniform int[CLOUD_LAYER_COUNT] cloud_types;
uniform float[CLOUD_TYPE_COUNT] cloud_coverages;

uniform vec3[CLOUD_LAYER_COUNT] windspeeds;
uniform vec3[CLOUD_TYPE_COUNT] base_noise_ratios;
uniform vec3[CLOUD_TYPE_COUNT] detail_noise_ratios;

uniform float[CLOUD_TYPE_COUNT] cloud_densities;

uniform vec3 cloud_tint;

uniform float fade_start_distance;
uniform float fade_end_distance;

uniform float light_attenuation;

uniform vec3 sun_direction;

uniform vec3 sun_tint;
uniform float sun_gain;

uniform vec3 atmosphere_tint;
uniform float atmospheric_blending;

uniform float forward_mie_scattering;
uniform float backward_mie_scattering;

uniform float local_time;

layout(location = 0) out vec4 fragment_color;

float map(in float input_value, in float input_start, in float input_end, in float output_start, in float output_end)
{
	float slope = (output_end - output_start) / (input_end - input_start);

	return clamp(output_start + (slope * (input_value - input_start)), min(output_start, output_end), max(output_start, output_end));
}

float henyey_greenstein(in float dot_angle, in float scattering_value)
{
	float squared_scattering_value = pow(scattering_value, 2.0);

	return (1.0 - squared_scattering_value) / (4.0 * PI * pow(squared_scattering_value - (2.0 * scattering_value * dot_angle) + 1.0, 1.5));
}

float get_height_ratio(in vec3 ray_position, in int layer_index)
{
	return map(length(ray_position - EARTH_CENTER) - EARTH_RADIUS, cloud_bases[layer_index], cloud_bases[layer_index] + cloud_heights[cloud_types[layer_index] - 1], 0.0, 1.0);
}

float sample_clouds(in vec3 ray_position, in int layer_index)
{
	//vec3 wind_offset = vec3(1.0, 0.25, 1.0) * local_time;
	vec3 wind_offset = windspeeds[layer_index] * local_time;
	vec4 base_noise_sample = texture(base_noise_texture, (ray_position + wind_offset) * base_noise_scale);
	float base_noise = map(base_noise_sample.x, dot(base_noise_sample.yzw, base_noise_ratios[cloud_types[layer_index] - 1]), 1.0, 0.0, 1.0);

	float height_ratio = get_height_ratio(ray_position, layer_index);
	float height_multiplier = map(height_ratio, 0.0, 0.25, 0.0, 1.0) * map(height_ratio, 0.5, 1.0, 1.0, 0.0);

	float base_erosion = map(base_noise * height_multiplier, 1.0 - max(texture(cloud_map_texture, (ray_position.xz + wind_offset.xz) * cloud_map_scale).x, cloud_coverages[cloud_types[layer_index] - 1]), 1.0, 0.0, 1.0);

	if (base_erosion > 0.01)
	{
		vec3 detail_noise_sample = texture(detail_noise_texture, ray_position * detail_noise_scale).xyz;
		float detail_noise = dot(detail_noise_sample, detail_noise_ratios[cloud_types[layer_index] - 1]);

		return map(base_erosion, 0.75 * detail_noise, 1.0, 0.0, 1.0);
	}
	else return base_erosion;
}

vec2 ray_sphere_intersections(in vec3 ray_position, in vec3 ray_direction, in float sphere_height)
{
	vec3 ray_earth_vector = ray_position - EARTH_CENTER;

	float coefficient_1 = 2.0 * dot(ray_direction, ray_earth_vector);
	float coefficient_2 = dot(ray_earth_vector, ray_earth_vector) - pow(EARTH_RADIUS + sphere_height, 2.0);

	float discriminant = pow(coefficient_1, 2.0) - (4.0 * coefficient_2);

	if (discriminant < 0.0) return vec2(0.0, 0.0);
	else
	{
		float lower_solution = ((-1.0 * coefficient_1) - sqrt(discriminant)) / 2.0;
		float higher_solution = ((-1.0 * coefficient_1) + sqrt(discriminant)) / 2.0;

		if (lower_solution < 0.0) return vec2(max(higher_solution, 0.0), 0.0);
		else return vec2(lower_solution, higher_solution);
	}
}

vec4 ray_march(in int layer_index, in vec4 input_color)
{
	vec4 output_color = input_color;
	
	if (cloud_types[layer_index] != 0)
	{
		vec3 sample_ray_direction = normalize(ray_end_position - ray_start_position);

		vec2 inner_sphere_intersections = ray_sphere_intersections(ray_start_position, sample_ray_direction, cloud_bases[layer_index]);
		vec2 outer_sphere_intersections = ray_sphere_intersections(ray_start_position, sample_ray_direction, cloud_bases[layer_index] + cloud_heights[cloud_types[layer_index] - 1]);

		vec4 world_vector = inverse_projection_matrix * vec4((depth_texture_position * 2.0) - 1.0, map(texture(depth_texture, depth_texture_position).x, 0.0, 1.0, min(near_clip_z, far_clip_z), max(near_clip_z, far_clip_z)), 1.0);
		world_vector /= world_vector.w;

		vec3 world_position = vec3(inverse_modelview_matrix * world_vector);
		float world_distance = length(world_position - ray_start_position);

		float height_ratio = get_height_ratio(ray_start_position, layer_index);

		float ray_start_distance = 0.0;
		float ray_march_distance = 0.0;

		if (height_ratio == 0.0)
		{
			ray_start_distance = inner_sphere_intersections.x;
			ray_march_distance = outer_sphere_intersections.x - inner_sphere_intersections.x;
		}
		else if ((height_ratio > 0.0) && (height_ratio < 1.0))
		{
			float lower_distance = min(inner_sphere_intersections.x, outer_sphere_intersections.x);
			float higher_distance = max(inner_sphere_intersections.x, outer_sphere_intersections.x);

			if (lower_distance == 0.0) ray_march_distance = higher_distance;
			else ray_march_distance = lower_distance;
		}
		else if (height_ratio == 1.0)
		{
			if (inner_sphere_intersections.x == 0.0)
			{
				ray_start_distance = outer_sphere_intersections.x;
				ray_march_distance = outer_sphere_intersections.y - outer_sphere_intersections.x;
			}
			else
			{
				ray_start_distance = outer_sphere_intersections.x;
				ray_march_distance = inner_sphere_intersections.x - outer_sphere_intersections.x;
			}
		}

		ray_start_distance = min(ray_start_distance, world_distance);
		ray_march_distance = min(min(ray_march_distance, world_distance - ray_start_distance), 250000.0);

		if (ray_march_distance > 5.0)
		{
			vec3 sample_ray_position = ray_start_position + (sample_ray_direction * ray_start_distance);
			float sample_ray_distance = 0.0;

			float sample_step_size = min(ray_march_distance / SAMPLE_STEP_COUNT, MAXIMUM_SAMPLE_STEP_SIZE);
			float sun_step_size = cloud_heights[cloud_types[layer_index] - 1] / SUN_STEP_COUNT;

			float sun_dot_angle = dot(sample_ray_direction, sun_direction);
			float mie_scattering_gain = clamp(mix(henyey_greenstein(sun_dot_angle, forward_mie_scattering), henyey_greenstein(sun_dot_angle, -1.0 * backward_mie_scattering), 0.5), 1.0, 2.5);

			while (sample_ray_distance <= ray_march_distance)
			{
				float cloud_sample = sample_clouds(sample_ray_position, layer_index);

				if (cloud_sample != 0.0)
				{
					float blocking_density = 0.0;

					vec3 sun_ray_position = sample_ray_position;

					for (int sun_step = 0; sun_step < SUN_STEP_COUNT; sun_step++)
					{
						blocking_density += sample_clouds(sun_ray_position, layer_index) * cloud_densities[cloud_types[layer_index] - 1];

						sun_ray_position += sun_direction * sun_step_size;
					}

					float sample_attenuation = 1.5 * sqrt(3.0) * exp(-1.0 * blocking_density) * (1.0 - exp(-2.0 * blocking_density));
					vec3 sample_color = clamp(mix(mix(cloud_tint, atmosphere_tint, atmospheric_blending), sun_tint * sun_gain * mie_scattering_gain, sample_attenuation) * clamp(sample_attenuation, 0.75, 1.0) * light_attenuation, 0.0, 1.0);

					float alpha_multiplier = map(length(sample_ray_position - ray_start_position), fade_start_distance, fade_end_distance, 1.0, 0.0);
					if (alpha_multiplier < 0.01) break;

					float sample_alpha = cloud_sample * alpha_multiplier;

					output_color.xyz += sample_color * sample_alpha * output_color.w;
					output_color.w *= 1.0 - sample_alpha;

					if (output_color.w < 0.01) break;
				}

				
				float current_step_size = sample_step_size * map(sample_ray_distance, 0.0, ray_march_distance, 1.0, 2.5);
				sample_ray_position += sample_ray_direction * current_step_size;
				sample_ray_distance += current_step_size;
			}
		}
	}

	return output_color;
}

void main()
{
	vec4 output_color = vec4(0.0, 0.0, 0.0, 1.0);

	int first_higher_layer = 0;
	while ((first_higher_layer < CLOUD_LAYER_COUNT) && ((cloud_bases[first_higher_layer] + cloud_heights[cloud_types[first_higher_layer] - 1]) < (length(ray_start_position - EARTH_CENTER) - EARTH_RADIUS))) first_higher_layer++;

	for (int layer_index = first_higher_layer; layer_index < CLOUD_LAYER_COUNT; layer_index++) output_color = ray_march(layer_index, output_color);
	for (int layer_index = first_higher_layer - 1; layer_index >= 0; layer_index--) output_color = ray_march(layer_index, output_color);

	fragment_color = vec4(output_color.xyz, 1.0 - output_color.w);
}
