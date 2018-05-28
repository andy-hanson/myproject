#include <cassert>
#include <iostream> // std::cerr
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <limits>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "./util/io.h"
#include "./util/Matrix.h"
#include "./util/read_png.h"

namespace {
	struct ShaderProgram { GLuint id; };

	GLint to_glint(unsigned long l) {
		assert(l < std::numeric_limits<GLint>::max());
		return static_cast<GLint>(l);
	}

	struct VBO { GLuint id; };

	// Note: every member must be a float, since we pass this to glVertexAttribPointer.
	struct Attributes {
		glm::vec2 pos;
		glm::vec3 color;
		glm::vec2 texture_coords;
	} __attribute__((packed));

	//If changing this, must also change the call to `glVertexAttribPointer`
	VBO create_and_bind_vertex_buffer() {
		Attributes Vertices[3] = {
			{ { -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
			{ {  1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
			{ {  0.0f,  1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
		};
		GLuint VBO;
		glGenBuffers(1, &VBO);
		assert(VBO != 0);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
		return { VBO };
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
		//TODO: glBindFragDataLocation(shader_program.id, 0, "outColor); (it's the default though)

		return shader_id;
	}

	struct Shaders {
		ShaderProgram program;
		GLuint vertex;
		GLuint fragment;
	};
	Shaders compile_shaders() {
		ShaderProgram shader_program { glCreateProgram() };
		assert(shader_program.id != 0);

		std::string cwd = get_current_directory();
		std::string vs = read_file(cwd + "/shaders/shader.vs");
		std::string fs = read_file(cwd + "/shaders/shader.fs");

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

		glUseProgram(shader_program.id);

		return { shader_program, vertex_shader_id, fragment_shader_id };
	}

	void init_glew() {
		glewExperimental = GL_TRUE;
		// Must be done after glut is initialized!
		GLenum res = glewInit();
		if (res != GLEW_OK) {
			std::cerr << "Error: " << glewGetErrorString(res) << std::endl;
			assert(false);
		}
	}

	struct Uniforms {
		GLint uniColor;
	};

	void render(const Uniforms& uniforms, float r) {
		glUniform3f(uniforms.uniColor, r, 0.0f, 0.0f);

		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.5f, 0.0f);

		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	GLFWwindow* init_glfw() {
		glfwInit();
		GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL", nullptr, nullptr);
		assert(window != nullptr);
		glfwMakeContextCurrent(window);
		return window;
	}

	GLuint to_gluint(GLint i) {
		assert(i >= 0);
		return static_cast<GLuint>(i);
	}

	struct Texture { GLuint id; };

	//TODO:MOVE
	GLsizei to_glsizei(uint32_t u) {
		assert(u < std::numeric_limits<GLsizei>::max());
		return static_cast<GLsizei>(u);
	}

	Texture bind_texture(const Matrix<uint32_t>& mat) {
		// Generate the OpenGL texture object
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, to_glsizei(mat.width()), to_glsizei(mat.height()), 0, GL_RGBA, GL_UNSIGNED_BYTE, mat.raw());
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		return Texture { texture };
	}

	Texture load_texture() {
		Matrix<uint32_t> m = png_texture_load((get_current_directory() + "/textures/foo.png").c_str());
		return bind_texture(m);
		//TODO: how to remove the texture when done?
	}
}

int main() {
	GLFWwindow* window = init_glfw();
	init_glew();

	VBO vbo = create_and_bind_vertex_buffer();

	Shaders shaders = compile_shaders();

	GLuint pos_attrib = to_gluint(glGetAttribLocation(shaders.program.id, "position"));
	assert(pos_attrib == 0);
	glEnableVertexAttribArray(pos_attrib);
	glVertexAttribPointer(pos_attrib, /*size*/ 2, /*type*/ GL_FLOAT, /*normalized*/ GL_FALSE, /*stride*/ sizeof(Attributes), nullptr);

	GLuint col_attrib = to_gluint(glGetAttribLocation(shaders.program.id, "color"));
	assert(col_attrib == 1);
	glEnableVertexAttribArray(col_attrib);
	glVertexAttribPointer(col_attrib, /*size*/ 3, /*type*/ GL_FLOAT, /*normalized*/ GL_FALSE, /*stride*/ sizeof(Attributes), reinterpret_cast<void*>(2 * sizeof(float)));

	GLuint texCoord_attrib = to_gluint(glGetAttribLocation(shaders.program.id, "texCoord"));
	assert(texCoord_attrib == 2);
	glEnableVertexAttribArray(texCoord_attrib);
	glVertexAttribPointer(texCoord_attrib, /*size*/ 2, /*type*/ GL_FLOAT, /*normalized*/ GL_FALSE, /*stride*/ sizeof(Attributes), reinterpret_cast<void*>(5 * sizeof(float)));

	load_texture();
	glUniform1i(glGetUniformLocation(shaders.program.id, "tex"), 0);

	GLint uniColor = glGetUniformLocation(shaders.program.id, "u_triangleColor");
	assert(uniColor == 0); // NOTE: if this fails, perhaps the uniform was unused
	Uniforms uniforms { uniColor };

	float r = 0;
	while (!glfwWindowShouldClose(window)) {
		r += 0.01;
		if (r >= 1) r = 0;
		render(uniforms, r);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteProgram(shaders.program.id);
	glDeleteShader(shaders.vertex);
	glDeleteShader(shaders.fragment);
	glDisableVertexAttribArray(0);

	glDeleteBuffers(1, &vbo.id);

	glfwDestroyWindow(window);
	glfwTerminate();
}
