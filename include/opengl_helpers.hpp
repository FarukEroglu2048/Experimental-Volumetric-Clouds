#pragma once

#ifdef APL
#include <OpenGL/gl.h>
#define glBindVertexArray glBindVertexArrayAPPLE
#define glGenVertexArrays glGenVertexArraysAPPLE
#else
#include <GL/glew.h>
#endif

#define SHADER_INVALID 0
#define TEXTURE_INVALID 0

GLuint load_shader(char* shader_path, GLenum shader_type);

int create_texture(GLsizei texture_width, GLsizei texture_height, GLint internal_format, GLenum data_format, void* texture_data);
int create_texture(GLsizei texture_width, GLsizei texture_height, GLsizei texture_depth, GLint internal_format, GLenum data_format, void* texture_data);

int load_png_texture(char* texture_path, bool create_3d_texture);