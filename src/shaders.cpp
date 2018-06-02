#include "./shaders.h"

#include <glm/gtc/type_ptr.hpp> // glm::value_ptr

#include "./util/read_png.h"
#include "./util/math.h"

namespace {
	GLsizei u32_to_glsizei(u32 u) {
		assert(u < std::numeric_limits<GLsizei>::max());
		return static_cast<GLsizei>(u);
	}

	GLuint to_gluint(GLint i) {
		assert(i >= 0);
		return static_cast<GLuint>(i);
	}

	GLuint add_shader(ShaderProgram shader_program, const std::string& shader_source, GLenum ShaderType) {
		GLuint shader_id = glCreateShader(ShaderType);
		assert(shader_id != 0);

		const char* c_str = shader_source.c_str();
		GLint len = to_glint(shader_source.size());
		glShaderSource(shader_id, 1, &c_str, &len);
		glCompileShader(shader_id);
		GLint success;
		glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
		if (success == 0) {
			GLchar InfoLog[1024];
			glGetShaderInfoLog(shader_id, 1024, nullptr, InfoLog);
			std::cerr << "Error compiling shader " << std::endl << shader_source << std::endl << InfoLog << std::endl;
			assert(false);
		}

		glAttachShader(shader_program.id, shader_id);
		//TODO: glBindFragDataLocation(shader_program.id, 0, "outColor"); (it's the default though)

		return shader_id;
	}

	Shaders compile_shaders(const std::string& name) {
		ShaderProgram shader_program { glCreateProgram() };
		assert(shader_program.id != 0);

		std::string cwd = get_current_directory();
		std::string vs = read_file(cwd + "/shaders/" + name + ".vs");
		std::string fs = read_file(cwd + "/shaders/" + name + ".fs");

		GLuint vertex_shader_id = add_shader(shader_program, vs, GL_VERTEX_SHADER);
		GLuint fragment_shader_id = add_shader(shader_program, fs, GL_FRAGMENT_SHADER);

		GLint success = 0;
		GLchar error_log[1024];

		glLinkProgram(shader_program.id);

		glGetProgramiv(shader_program.id, GL_LINK_STATUS, &success);
		GLsizei error_log_length;
		glGetProgramInfoLog(shader_program.id, sizeof(error_log), &error_log_length, error_log);
		if (error_log_length != 0) {
			std::cout << "Error linking shader program: " << error_log << std::endl;
			assert(false);
		}
		assert(success == 1);

		glValidateProgram(shader_program.id);
		glGetProgramiv(shader_program.id, GL_VALIDATE_STATUS, &success);
		glGetProgramInfoLog(shader_program.id, sizeof(error_log), &error_log_length, error_log);
		if (error_log_length != 0) {
			std::cerr << "Invalid shader program: " << error_log << std::endl;
			assert(false);
		}
		assert(success == 1);

		shader_program.use();

		return { shader_program, vertex_shader_id, fragment_shader_id };
	}

	Texture bind_texture(const Matrix<u32>& mat) {
		// Generate the OpenGL texture object
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, u32_to_glsizei(mat.width()), u32_to_glsizei(mat.height()), 0, GL_RGBA, GL_UNSIGNED_BYTE, mat.raw());
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		return Texture { texture };
	}

	__attribute__((unused))
	Texture load_texture() {
		Matrix<u32> m = png_texture_load((get_current_directory() + "/textures/foo.png").c_str());
		return bind_texture(m);
		//TODO: how to remove the texture when done?
	}

	glm::mat4 total_matrix(float time) {
		glm::mat4 model = glm::rotate(glm::mat4{}, time * glm::radians(108.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 view = glm::lookAt(
		/*eye*/ glm::vec3(4.0f, 4.0f, 4.0f),
		/*center*/ glm::vec3(0.0f, 0.0f, 0.0f),
		/*up*/ glm::vec3(0.0f, 0.0f, 1.0f)
		);
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 1.0f, 10.0f);

		return proj * view * model;
	}
}

VBO create_and_bind_vertex_buffer(const DynArray<VertexAttributes>& vertices) {
	GLuint vbo_id;
	glGenBuffers(1, &vbo_id);
	assert(vbo_id != 0);
	VBO vbo { vbo_id, vertices.size() };
	vbo.bind();
	glBufferData(GL_ARRAY_BUFFER, vbo.n_vertices * sizeof(VertexAttributes), vertices.begin(), GL_STATIC_DRAW);
	return vbo;
}



Uniform get_uniform(const Shaders& shaders, const char* name) {
	GLint id = glGetUniformLocation(shaders.program.id, name);
	assert(id != -1); // NOTE: if this fails, perhaps the uniform was unused
	return Uniform { id };
}

//TODO: create all vaos at once instead of one at a time
VAO create_and_bind_vao() {
	GLuint id;
	glGenVertexArrays(1, &id);
	VAO out { id };
	out.bind();
	return out;
}

Shaders init_shaders(const std::string& name) {
	Shaders shaders = compile_shaders(name);

	GLuint pos_attrib = to_gluint(glGetAttribLocation(shaders.program.id, "position"));
	assert(pos_attrib == 0);
	glEnableVertexAttribArray(pos_attrib);
	glVertexAttribPointer(pos_attrib, safe_div(sizeof(glm::vec3), sizeof(float)), /*type*/ GL_FLOAT, /*normalized*/ GL_FALSE, /*stride*/ sizeof(VertexAttributes), reinterpret_cast<void*>(offsetof(VertexAttributes, pos)));

	GLuint col_attrib = to_gluint(glGetAttribLocation(shaders.program.id, "color"));
	assert(col_attrib == 1);
	glEnableVertexAttribArray(col_attrib);
	glVertexAttribPointer(col_attrib, safe_div(sizeof(glm::vec3), sizeof(float)), /*type*/ GL_FLOAT, /*normalized*/ GL_FALSE, /*stride*/ sizeof(VertexAttributes), reinterpret_cast<void*>(offsetof(VertexAttributes, color)));

	/*GLuint texCoord_attrib = to_gluint(glGetAttribLocation(shaders.program.id, "texCoord"));
	assert(texCoord_attrib == 2);
	glEnableVertexAttribArray(texCoord_attrib);
	glVertexAttribPointer(texCoord_attrib, safe_div(sizeof(glm::vec2), sizeof(float)), / *type* / GL_FLOAT, / *normalized* / GL_FALSE, / *stride* / sizeof(VertexAttributes), reinterpret_cast<void*>(offsetof(Attributes, texture_coords)));*/

	return shaders;
}

void free_shaders(Shaders& shaders) {
	glDeleteProgram(shaders.program.id);
	glDeleteShader(shaders.vertex);
	glDeleteShader(shaders.fragment);
	glDisableVertexAttribArray(0);
}

void render(const VAOInfo& vao_info_tris, const VAOInfo& vao_info_dots, float time) {
	//glUniform3f(uniforms.uniColor.id, time - flo(time), 0.0f, 0.0f);
	glm::mat4 trans = total_matrix(time);


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.1f, 0.0f);

	// In the tri pass: set stencil buffer to "42" wherever we draw a pixel.
	int object_id = 42; // TODO
	glStencilOp(/*stencil fail*/ GL_REPLACE, /*stencil pass, depth fail*/ GL_REPLACE, /*stencil pass, depth pass*/ GL_REPLACE);
	glStencilFunc(GL_ALWAYS, object_id, 0xff);

	//tris
	vao_info_tris.vao.bind();
	vao_info_tris.shaders.use();
	glUniformMatrix4fv(vao_info_tris.uniforms.transform_tri.id, 1, /*transpose*/ GL_FALSE, glm::value_ptr(trans));
	vao_info_tris.vbo.bind();
	glDrawArrays(GL_TRIANGLES, 0, u32_to_glsizei(vao_info_tris.vbo.n_vertices));

	glClear(GL_DEPTH_BUFFER_BIT);

	// In the dot pass: We should only draw if the stencil buffer was drawn to.
	// Not sure why another glStencilOp call is necessary...
	glStencilOp(/*stencil fail*/ GL_ZERO, /*stencil pass, depth fail*/ GL_ZERO, /*stencil pass, depth pass*/ GL_ZERO);
	glStencilFunc(GL_EQUAL, object_id, 0xff);

	//TODO:DOTS
	vao_info_dots.vao.bind();
	vao_info_dots.shaders.use();
	glUniformMatrix4fv(vao_info_dots.uniforms.transform_tri.id, 1, /*transpose*/ GL_FALSE, glm::value_ptr(trans));
	vao_info_dots.vbo.bind();
	glDrawArrays(GL_POINTS, 0, u32_to_glsizei(vao_info_dots.vbo.n_vertices));
	//vbo_dots.bind();
	//glDrawArrays(GL_POINTS, 0, u32_to_glsizei(vbo_dots.n_vertices));
}
