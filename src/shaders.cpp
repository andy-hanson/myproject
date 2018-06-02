#include "./shaders.h"

#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr

#include "./util/DynArray.h"
#include "./util/int.h"
#include "./util/io.h"
#include "./util/Matrix.h"
#include "./util/assert.h"
#include "./util/read_png.h"
#include "./util/math.h"

#include <iostream> // For printing errors

namespace {
	GLsizei u32_to_glsizei(u32 u) {
		check(u < std::numeric_limits<GLsizei>::max());
		return static_cast<GLsizei>(u);
	}

	GLint to_glint(unsigned long l) {
		check(l < std::numeric_limits<GLint>::max());
		return static_cast<GLint>(l);
	}

	u32 gluint_to_u32(GLuint u) {
		check(u < std::numeric_limits<u32>::max());
		return static_cast<u32>(u);
	}

	u32 glint_to_u32(GLint i) {
		check(i >= 0);
		return static_cast<u32>(i);
	}

	GLuint to_gluint(GLint i) {
		check(i >= 0);
		return static_cast<GLuint>(i);
	}
}

namespace {

	struct ShaderProgram {
		u32 id;

		void use() const {
			glUseProgram(id);
		}
	};

	struct VBO {
		u32 id;
		u32 n_vertices;

		void bind() const {
			glBindBuffer(GL_ARRAY_BUFFER, id);
		}

		void free() {
			glDeleteBuffers(1, &id);
		}
	};

	struct VAO {
		u32 id;
		void bind() const {
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

	struct Uniform { u32 id; };
	struct Uniforms {
		//Uniform uniColor;
		Uniform transform_tri;
	};

	struct Texture { u32 id; };

	struct VAOInfo {
		VAO vao;
		VBO vbo;
		Shaders shaders;
		Uniforms uniforms;

		void free() {
			//todo: free vao
			vbo.free();
			shaders.free();
		}
	};

	struct FrameBuffer {
		GLuint id;
		GLuint texture_id;
	};
}

namespace {
	GLuint add_shader(ShaderProgram shader_program, const std::string& shader_source, GLenum ShaderType) {
		GLuint shader_id = glCreateShader(ShaderType);
		check(shader_id != 0);

		const char* c_str = shader_source.c_str();
		GLint len = to_glint(shader_source.size());
		glShaderSource(shader_id, 1, &c_str, &len);
		glCompileShader(shader_id);

#ifdef DEBUG
		GLint success;
		glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
		if (success == 0) {
			GLchar InfoLog[1024];
			glGetShaderInfoLog(shader_id, 1024, nullptr, InfoLog);
			std::cerr << "Error compiling shader " << std::endl << shader_source << std::endl << InfoLog << std::endl;
			todo();
		}
#endif

		glAttachShader(shader_program.id, shader_id);
		//TODO: glBindFragDataLocation(shader_program.id, 0, "outColor"); (it's the default though)

		return shader_id;
	}

	Shaders compile_shaders(const std::string& name) {
		ShaderProgram shader_program { gluint_to_u32(glCreateProgram()) };
		check(shader_program.id != 0);

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
		//TODO: #ifdef DEBUG
		glGetProgramInfoLog(shader_program.id, sizeof(error_log), &error_log_length, error_log);
		if (error_log_length != 0) {
			std::cout << "Error linking shader program: " << error_log << std::endl;
			check(false);
		}
		check(success == 1);

		glValidateProgram(shader_program.id);
		glGetProgramiv(shader_program.id, GL_VALIDATE_STATUS, &success);
		//TODO: #ifdef DEBUG
		glGetProgramInfoLog(shader_program.id, sizeof(error_log), &error_log_length, error_log);
		if (error_log_length != 0) {
			std::cerr << "Invalid shader program: " << error_log << std::endl;
			todo();
		}
		check(success == 1);

		shader_program.use();

		return Shaders { shader_program, vertex_shader_id, fragment_shader_id };
	}

	Texture bind_texture(const Matrix<u32>& mat) {
		// Generate the OpenGL texture object
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, u32_to_glsizei(mat.width()), u32_to_glsizei(mat.height()), 0, GL_RGBA, GL_UNSIGNED_BYTE, mat.raw());
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		return Texture { gluint_to_u32(texture) };
	}

	__attribute__((unused))
	Texture load_texture() {
		Matrix<u32> m = png_texture_load((get_current_directory() + "/textures/foo.png").c_str());
		return bind_texture(m);
		//TODO: how to remove the texture when done?
	}

	//TODO:MOVE
	glm::mat4 total_matrix(float time) {
		glm::mat4 model = glm::rotate(glm::mat4{}, time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 view = glm::lookAt(
		/*eye*/ glm::vec3(4.0f, 4.0f, 4.0f),
		/*center*/ glm::vec3(0.0f, 0.0f, 0.0f),
		/*up*/ glm::vec3(0.0f, 0.0f, 1.0f)
		);
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 1.0f, 10.0f);

		return proj * view * model;
	}

	__attribute__((unused))
	FrameBuffer do_framebuffer() {
		GLuint frame_buffer;
		glGenFramebuffers(1, &frame_buffer);
		glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

		GLuint rendered_texture; // render to this
		glGenTextures(1, &rendered_texture);
		glBindTexture(GL_TEXTURE_2D, rendered_texture);
		GLsizei tex_width = 512, tex_height = 512;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_width, tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		GLuint depth_render_buffer; // the rendering will need a depth buffer
		glGenRenderbuffers(1, &depth_render_buffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_render_buffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, tex_width, tex_height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_render_buffer);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rendered_texture, 0);
		const uint n_draw_buffers = 1;
		GLenum DrawBuffers[n_draw_buffers] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(n_draw_buffers, DrawBuffers);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			todo();

		return { frame_buffer, rendered_texture };
	}
}

namespace {
	VBO create_and_bind_vertex_buffer(const DynArray<VertexAttributes>& vertices) {
		GLuint vbo_id;
		glGenBuffers(1, &vbo_id);
		check(vbo_id != 0);
		VBO vbo { vbo_id, vertices.size() };
		vbo.bind();
		glBufferData(GL_ARRAY_BUFFER, vbo.n_vertices * sizeof(VertexAttributes), vertices.begin(), GL_STATIC_DRAW);
		return vbo;
	}

	Uniform get_uniform(const Shaders& shaders, const char* name) {
		GLint id = glGetUniformLocation(shaders.program.id, name);
		check(id != -1); // NOTE: if this fails, perhaps the uniform was unused
		return Uniform { glint_to_u32(id) };
	}

	//TODO: create all vaos at once instead of one at a time
	VAO create_and_bind_vao() {
		GLuint id;
		glGenVertexArrays(1, &id);
		VAO out { gluint_to_u32(id) };
		out.bind();
		return out;
	}

	Shaders init_shaders(const std::string& name) {
		Shaders shaders = compile_shaders(name);

		GLuint pos_attrib = to_gluint(glGetAttribLocation(shaders.program.id, "position"));
		check(pos_attrib == 0);
		glEnableVertexAttribArray(pos_attrib);
		glVertexAttribPointer(pos_attrib, safe_div(sizeof(glm::vec3), sizeof(float)), /*type*/ GL_FLOAT, /*normalized*/ GL_FALSE, /*stride*/ sizeof(VertexAttributes), reinterpret_cast<void*>(offsetof(VertexAttributes, pos)));

		GLuint col_attrib = to_gluint(glGetAttribLocation(shaders.program.id, "color"));
		check(col_attrib == 1);
		glEnableVertexAttribArray(col_attrib);
		glVertexAttribPointer(col_attrib, safe_div(sizeof(glm::vec3), sizeof(float)), /*type*/ GL_FLOAT, /*normalized*/ GL_FALSE, /*stride*/ sizeof(VertexAttributes), reinterpret_cast<void*>(offsetof(VertexAttributes, color)));

		/*GLuint texCoord_attrib = to_gluint(glGetAttribLocation(shaders.program.id, "texCoord"));
		check(texCoord_attrib == 2);
		glEnableVertexAttribArray(texCoord_attrib);
		glVertexAttribPointer(texCoord_attrib, safe_div(sizeof(glm::vec2), sizeof(float)), / *type* / GL_FLOAT, / *normalized* / GL_FALSE, / *stride* / sizeof(VertexAttributes), reinterpret_cast<void*>(offsetof(Attributes, texture_coords)));*/

		return shaders;
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
}

struct GraphicsImpl {
	VAOInfo vao_info_tris;
	VAOInfo vao_info_dots;
	//FrameBuffer frame_buffer;

	void render(float time) {
		//glUniform3f(uniforms.uniColor.id, time - flo(time), 0.0f, 0.0f);
		glm::mat4 trans = total_matrix(time);

		glClear(static_cast<uint>(GL_COLOR_BUFFER_BIT) | static_cast<uint>(GL_DEPTH_BUFFER_BIT) | static_cast<uint>(GL_STENCIL_BUFFER_BIT));
		glClearColor(0.0f, 0.0f, 0.1f, 0.0f);

		// In the tri pass: set stencil buffer to object_id wherever we draw a pixel.
		constexpr int object_id = 1; // TODO
		glStencilOp(/*stencil fail*/ GL_REPLACE, /*stencil pass, depth fail*/ GL_REPLACE, /*stencil pass, depth pass*/ GL_REPLACE);
		glStencilFunc(GL_ALWAYS, object_id, 0xff);

		// Draw triangles
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

	~GraphicsImpl() {
		vao_info_tris.free();
		vao_info_dots.free();
	}
};

Graphics Graphics::start(const RenderableModel& renderable_model) {
	init_glew();

	VAO vao_tris = create_and_bind_vao();
	VBO vbo_tris = create_and_bind_vertex_buffer(renderable_model.tris);
	Shaders shaders_tri = init_shaders("tri");
	Uniforms uniforms_tri { get_uniform(shaders_tri, "u_transform") };
	VAOInfo vao_info_tris { vao_tris, vbo_tris, shaders_tri, uniforms_tri };

	VAO vao_dots = create_and_bind_vao();
	VBO vbo_dots = create_and_bind_vertex_buffer(renderable_model.dots);
	Shaders shaders_dot = init_shaders("dot");
	Uniforms uniforms_dot { get_uniform(shaders_dot, "u_transform") };
	VAOInfo vao_info_dots { std::move(vao_dots), std::move(vbo_dots), std::move(shaders_dot), uniforms_dot };

	glEnable(GL_DEPTH_TEST); //TODO: do this closer to where it's needed, and pair with glDIsable
	glEnable(GL_STENCIL_TEST); //TODO:KILL
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	return { new GraphicsImpl { std::move(vao_info_tris), std::move(vao_info_dots) } };//, frame_buffer } };
}

Graphics::~Graphics() {
	delete _impl;
	//TODO: free VAO
}

void Graphics::render(float time) {
	_impl->render(time);
}
