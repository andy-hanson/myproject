#pragma once

#include <GL/glew.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <cassert>
#include <iostream>
#include <limits>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "./util/int.h"
#include "./util/io.h"
#include "./util/Matrix.h"
#include "./model.h"

struct ShaderProgram {
	GLuint id;

	void use() const {
		glUseProgram(id);
	}
};

inline GLint to_glint(unsigned long l) {
	assert(l < std::numeric_limits<GLint>::max());
	return static_cast<GLint>(l);
}

struct VBO {
	GLuint id;
	u32 n_vertices;

	VBO(const VBO& other) = delete;
	VBO(VBO&& other) = default;

	void bind() const {
		glBindBuffer(GL_ARRAY_BUFFER, id);
	}

	~VBO() {
		glDeleteBuffers(1, &id);
	}
};

struct VAO {
	GLuint id;
	void bind() const {
		glBindVertexArray(id);
	}
};

struct Shaders {
	ShaderProgram program;
	GLuint vertex;
	GLuint fragment;

	Shaders(const Shaders& other) = delete;
	Shaders(Shaders&& other) = default;

	void use() const {
		program.use();
	}
};

struct Uniform { GLint id; };
struct Uniforms {
	//Uniform uniColor;
	Uniform transform_tri;
};

struct Texture { GLuint id; };

struct VAOInfo {
	const VAO& vao;
	const VBO& vbo;
	const Shaders& shaders;
	const Uniforms& uniforms;
};



VAO create_and_bind_vao();
VBO create_and_bind_vertex_buffer(const DynArray<VertexAttributes>& vertices);
Shaders init_shaders(const std::string& name);
Uniform get_uniform(const Shaders& shaders, const char* name);
void render(const VAOInfo& vao_info_tris, const VAOInfo& vao_info_dots, float time);
void free_shaders(Shaders& shaders); // TODO: DESTRUCTOR
