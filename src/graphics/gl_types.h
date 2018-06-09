#pragma once

#include <GL/glew.h>

#include "../util/int.h"

struct ShaderProgram {
	u32 id;

	void use() const {
		glUseProgram(id);
	}
};

struct VBO {
	u32 id;
	u32 n_vertices;

	inline void bind() const {
		glBindBuffer(GL_ARRAY_BUFFER, id);
	}

	inline void free() {
		glDeleteBuffers(1, &id);
	}
};

struct VAO {
	u32 id;
	inline void bind() const {
		glBindVertexArray(id);
	}
};

struct Shaders {
	ShaderProgram program;
	u32 vertex;
	u32 fragment;

	inline void use() const {
		program.use();
	}

	void free() {
		glDeleteProgram(program.id);
		glDeleteShader(vertex);
		glDeleteShader(fragment);
		glDisableVertexAttribArray(0);
	}
};

template <typename TUniforms>
struct ShadersInfo {
	Shaders shaders;
	TUniforms uniforms;

	void free() {
		shaders.free();
	}
};


struct Uniform {
	u32 id;
	Uniform() = delete;
};
struct TriUniforms {
	//Uniform uniColor;
	Uniform u_transform;
};
struct DotUniforms {
	Uniform u_model;
	Uniform u_transform;
	Uniform u_materials;
	Uniform u_depth_texture;
};

struct Texture {
	u32 id;
	explicit Texture(u32 _id) : id{_id} {}
};

struct VAOInfo {
	VAO vao;
	VBO vbo;

	inline void free() {
		//todo: free vao
		vbo.free();
	}
};

struct FrameBuffer {
	GLuint id;
	Texture output_texture;

	inline void free() {
		glDeleteFramebuffers(1, &id);
	}
};

inline GLsizei u32_to_glsizei(u32 u) {
	check(u < std::numeric_limits<GLsizei>::max());
	return static_cast<GLsizei>(u);
}

inline GLint to_glint(unsigned long l) {
	check(l < std::numeric_limits<GLint>::max());
	return static_cast<GLint>(l);
}

inline u32 gluint_to_u32(GLuint u) {
	check(u < std::numeric_limits<u32>::max());
	return static_cast<u32>(u);
}

inline u32 glint_to_u32(GLint i) {
	check(i >= 0);
	return static_cast<u32>(i);
}

inline GLuint glint_to_gluint(GLint i) {
	check(i >= 0);
	return static_cast<GLuint>(i);
}
