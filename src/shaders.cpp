#include "./shaders.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
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
	// Since we registered a callback, this shouldn't be necessary
	__attribute__((unused))
	void check_gl_error() {
		GLenum e = glGetError();
		if (e == GL_NO_ERROR) return;

		switch (e) {
			case GL_INVALID_ENUM: todo();
			case GL_INVALID_VALUE: todo();
			case GL_INVALID_OPERATION: todo();
			case GL_INVALID_FRAMEBUFFER_OPERATION: todo();
			case GL_OUT_OF_MEMORY: todo();
			case GL_STACK_UNDERFLOW: todo();
			case GL_STACK_OVERFLOW: todo();
			default: todo();
		}
	}
}

namespace {
	//TODO: not const
	const uint VIEWPORT_WIDTH = 1024;
	const uint VIEWPORT_HEIGHT = 1024;

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

	template <typename TUniforms>
	struct VAOInfo {
		VAO vao;
		VBO vbo;
		Shaders shaders;
		TUniforms uniforms;

		void free() {
			//todo: free vao
			vbo.free();
			shaders.free();
		}
	};

	struct FrameBuffer {
		GLuint id;
		Texture output_texture;

		void free() {
			glDeleteFramebuffers(1, &id);
		}
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

		GLint success;
		glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
		if (success == 0) {
			GLchar InfoLog[1024];
			glGetShaderInfoLog(shader_id, 1024, nullptr, InfoLog);
			std::cerr << "Error compiling shader " << std::endl << shader_source << std::endl << InfoLog << std::endl;
			todo();
		}

		glAttachShader(shader_program.id, shader_id);
		//TODO: glBindFragDataLocation(shader_program.id, 0, "outColor"); (it's the default though)

		return shader_id;
	}

	Shaders compile_shaders(const std::string& name) {
		ShaderProgram shader_program { gluint_to_u32(glCreateProgram()) };
		check(shader_program.id != 0);

		std::string cwd = get_current_directory();
		std::string vs = read_file(cwd + "/shaders/" + name + ".vert");
		std::string fs = read_file(cwd + "/shaders/" + name + ".frag");

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

		shader_program.use();//TODO:NEEDED?

		return Shaders { shader_program, vertex_shader_id, fragment_shader_id };
	}

	Texture bind_texture_from_image(const Matrix<u32>& mat) {
		// Generate the OpenGL texture object
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, u32_to_glsizei(mat.width()), u32_to_glsizei(mat.height()), 0, GL_RGBA, GL_UNSIGNED_BYTE, mat.raw());
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		return Texture { gluint_to_u32(texture) };
	}

	struct Size { u32 width; u32 height; };
	std::pair<Texture, Size> load_texture() {
		Matrix<u32> m = png_texture_load((get_current_directory() + "/textures/foo.png").c_str());
		return { bind_texture_from_image(m), { m.width(), m.height() } };
		//TODO: how to remove the texture when done?
	}

	//TODO:MOVE
	struct Matrices {
		glm::mat4 model;
		glm::mat4 viewModel; // excludes proj
		glm::mat4 total;
	};
	Matrices get_matrices(float time __attribute__((unused))) {
		glm::mat4 model = glm::mat4{};//glm::rotate(glm::mat4{}, time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 view = glm::lookAt(
			// Z axis points towards me
			/*eye*/ glm::vec3(0.0f, 0.0f, 4.0f),
			/*center*/ glm::vec3(0.0f, 0.0f, 0.0f),
			/*up*/ glm::vec3(0.0f, 1.0f, 0.0f)
		);
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), static_cast<float>(VIEWPORT_WIDTH) / VIEWPORT_HEIGHT, 1.0f, 10.0f);
		if ((false))
			proj = glm::ortho<float>(
				/*left*/ 0,
				/*right*/ VIEWPORT_WIDTH,
				/*bottom*/ 0,
				/*top*/ VIEWPORT_HEIGHT,
				/*zNear*/ -2.0f,
				/*zFar*/ 2.0f);

		glm::mat4 viewModel = view * model;
		glm::mat4 total =  proj * viewModel;
		return { model, viewModel, total };
	}

	Texture create_framebuffer_texture() {
		GLuint id; // render to this
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(
			GL_TEXTURE_2D,
			/*mipmap level*/ 0,
			GL_R32UI,
			VIEWPORT_WIDTH,
			VIEWPORT_HEIGHT,
			/*must be 0*/ 0,
			GL_RED_INTEGER,
			GL_UNSIGNED_BYTE,
			nullptr); // data uninitialized (will write to it using framebuffer)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		return Texture { id };
	}

	FrameBuffer create_framebuffer() {
		GLuint frame_buffer_id;
		glGenFramebuffers(1, &frame_buffer_id);
		glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id);

		Texture rendered_texture = create_framebuffer_texture();

		//TODO: why specify render buffer? Why not just the texture?
		//I think this is just for depth.
		/*GLuint render_buffer_id;
		glGenRenderbuffers(1, &render_buffer_id);
		glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_id);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, VIEWPORT_WIDTH, VIEWPORT_HEIGHT); //GL_DEPTH_COMPONENT
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, render_buffer_id); //GL_DEPTH_ATTACHMENT
		*/


		// Need a renderbuffer for depth.
		GLuint render_buffer_id;
		glGenRenderbuffers(1, &render_buffer_id);
		glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_id);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buffer_id);


		// This frame buffer only needs one attachment. At least one color attachment must exist, so use that.

		// Set rendered_texture to be GL_COLOR_ATTACHMENT0, then set the draw buffer to be GL_COLOR_ATTACHMENT0.
		// So, we're drawing to rendered_texture.

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rendered_texture.id, /*mipmap level*/ 0);
		//glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rendered_texture.id, /*mipmap level*/ 0);
		// We need to have the depth attachment enabled, or else depth testing won't work
		constexpr uint n_draw_buffers = 1;
		const GLenum draw_buffers[n_draw_buffers] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(n_draw_buffers, draw_buffers);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			switch (status) {
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: todo();
				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: todo();
				case GL_FRAMEBUFFER_UNSUPPORTED: todo();
				default: todo();
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);//TODO: shouldn't be necessary

		return { frame_buffer_id, rendered_texture };
	}
}

namespace {
	template <typename TVertexAttributes>
	VBO create_and_bind_vertex_buffer(const DynArray<TVertexAttributes>& vertices) {
		GLuint vbo_id;
		glGenBuffers(1, &vbo_id);
		check(vbo_id != 0);
		VBO vbo { vbo_id, vertices.size() };
		vbo.bind();
		glBufferData(GL_ARRAY_BUFFER, vbo.n_vertices * sizeof(TVertexAttributes), vertices.begin(), GL_STATIC_DRAW);
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

	//TODO:NEATER
	enum class ShadersKind { Tri, Dot };
	Shaders init_shaders(const std::string& name, ShadersKind kind) {
		Shaders shaders = compile_shaders(name);

		uint stride = kind == ShadersKind::Tri ? sizeof(VertexAttributesTri) : sizeof(VertexAttributesDot);

		auto get_attrib = [&](const char* attr_name, uint expected_index) {
			GLuint attrib = to_gluint(glGetAttribLocation(shaders.program.id, attr_name));
			check(attrib == expected_index);
			glEnableVertexAttribArray(attrib);
			return attrib;
		};

		auto vec3_attr = [&](const char* attr_name, uint expected_index, size_t offset) {
			GLuint attrib = get_attrib(attr_name, expected_index);
			glVertexAttribPointer(attrib, safe_div(sizeof(glm::vec3), sizeof(float)), /*type*/ GL_FLOAT, /*normalized*/ GL_FALSE, stride, reinterpret_cast<void*>(offset));
		};
		auto int_attr = [&](const char* attr_name, uint expected_index, size_t offset) {
			GLuint attrib = get_attrib(attr_name, expected_index);
			glVertexAttribIPointer(attrib, 1, /*type*/ GL_UNSIGNED_INT, stride, reinterpret_cast<void*>(offset));
		};

		switch (kind) {
			case ShadersKind::Tri: {
				vec3_attr("a_position", 0, offsetof(VertexAttributesTri, a_position));
				int_attr("a_material_id", 1, offsetof(VertexAttributesTri, a_material_id));
				break;
			}

			case ShadersKind::Dot: {
				vec3_attr("a_position", 0, offsetof(VertexAttributesDot, a_position));
				vec3_attr("a_normal", 1, offsetof(VertexAttributesDot, a_normal));
				int_attr("a_material_id", 2, offsetof(VertexAttributesDot, a_material_id));
				break;
			}
		}

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

	template <typename Cb>
	inline void enabling(GLenum enabled, Cb cb) {
		assert(!glIsEnabled(enabled));
		glEnable(enabled);
		cb();
		glDisable(enabled);
	}


	void save_image(const Texture& texture, uint width, uint height, const char* file_name) {
		DynArray<u8> image_data { width * height * 3 }; // * 10 to make sure error isn't due to size
		glBindTexture(GL_TEXTURE_2D, texture.id);
		glGetTexImage(GL_TEXTURE_2D, /*mipmap level*/ 0, GL_RGB, GL_UNSIGNED_BYTE, image_data.begin());
		//glGetTextureImage(texture.id, /*mipmap level*/ 0, GL_RGB, GL_UNSIGNED_BYTE, image_data.size(), image_data.begin());
		write_png(width, height, image_data.slice(), file_name);
	}
}

struct GraphicsImpl {
	GLFWwindow* window;
	VAOInfo<TriUniforms> vao_info_tris;
	VAOInfo<DotUniforms> vao_info_dots;
	FrameBuffer frame_buffer;
	Texture loaded_texture; //TODO:KILL

	void render(float time) {
		//glUniform3f(uniforms.uniColor.id, time - flo(time), 0.0f, 0.0f);
		Matrices matrices = get_matrices(time);

		// In the tri pass: set stencil buffer to object_id wherever we draw a pixel.
		//constexpr int object_id = 1; // TODO
		//glStencilOp(/*stencil fail*/ GL_REPLACE, /*stencil pass, depth fail*/ GL_REPLACE, /*stencil pass, depth pass*/ GL_REPLACE);
		//glStencilFunc(GL_ALWAYS, object_id, 0xff);

		enabling(GL_DEPTH_TEST, [&]() {
			// First pass: draw to the frame buffer
			glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.id);
			//TODO:needed?
			glViewport(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

			glClear(static_cast<uint>(GL_COLOR_BUFFER_BIT) | static_cast<uint>(GL_DEPTH_BUFFER_BIT));
			glClearColor(0.0f, 0.0f, 0.1f, 0.0f);

			// Draw triangles
			vao_info_tris.vao.bind();
			vao_info_tris.shaders.use();
			glUniformMatrix4fv(vao_info_tris.uniforms.u_transform.id, 1, /*transpose*/ GL_FALSE, glm::value_ptr(matrices.total));
			vao_info_tris.vbo.bind();
			glDrawArrays(GL_TRIANGLES, 0, u32_to_glsizei(vao_info_tris.vbo.n_vertices));

			//Verified: we're indeed writing to the texture
			if ((false)) {
				DynArray<u8> image_data { VIEWPORT_WIDTH * VIEWPORT_HEIGHT * 3 };
				save_image(frame_buffer.output_texture, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, "/home/andy/CLionProjects/myproject/texture.png");

				// Reads pixels directly from frame buffer
				//glReadnPixels(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, image_data.size(), image_data.begin());
				//write_png(VIEWPORT_WIDTH, VIEWPORT_HEIGHT, image_data.slice(), "/home/andy/CLionProjects/myproject/screen.png");

				todo();
			}
		});

		constexpr uint MAX_MATERIALS = 5u;
		Material materials[MAX_MATERIALS] = {
			// Note: first object id is 1, so this is offset.
			// r g b r g b
			{ glm::vec3(1.0, 1.0, 0.0), glm::vec3(1.0, 1.0, 1.0) },
			{ glm::vec3(0.0, 1.0, 1.0), glm::vec3(1.0, 1.0, 1.0) },
			{ glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 0.0) },
			{ glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 0.0) },
			{ glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 0.0) },
		};

		if ((true)) {
			enabling(GL_VERTEX_PROGRAM_POINT_SIZE, [&]() {
				// Now render to screen.
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				//TODO:needed?
				glViewport(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

				glClear(static_cast<uint>(GL_COLOR_BUFFER_BIT));
				glClearColor(0.0f, 0.0f, 0.1f, 0.0f);

				vao_info_dots.vao.bind();
				vao_info_dots.shaders.use();
				glUniformMatrix4fv(vao_info_dots.uniforms.u_model.id, 1, /*transpose*/ GL_FALSE, glm::value_ptr(matrices.model));
				glUniformMatrix4fv(vao_info_dots.uniforms.u_transform.id, 1, /*transpose*/ GL_FALSE, glm::value_ptr(matrices.total));
				glUniform1fv(vao_info_dots.uniforms.u_materials.id, MAX_MATERIALS * sizeof(Material) / sizeof(float), reinterpret_cast<float*>(materials));

				//TODO: check for error after trying to set uniform?
				//TODO: Yes this is tricky. Have to bind a texture and set the uniform to the texture *unit*, not the texture id.
				glBindTexture(GL_TEXTURE_2D, frame_buffer.output_texture.id);
				glUniform1i(vao_info_dots.uniforms.u_depth_texture.id, 0);//frame_buffer.output_texture.id); //TODO: just guessing here

				vao_info_dots.vbo.bind();
				glDrawArrays(GL_POINTS, 0, u32_to_glsizei(vao_info_dots.vbo.n_vertices));
			});
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	~GraphicsImpl() {
		vao_info_tris.free();
		vao_info_dots.free();
		frame_buffer.free();

		glfwDestroyWindow(window);
		glfwTerminate();
	}
};

namespace {
	GLFWwindow* init_glfw() {
		glfwInit();
		GLFWwindow* window = glfwCreateWindow(VIEWPORT_WIDTH, VIEWPORT_HEIGHT, "My window", nullptr, nullptr);
		assert(window != nullptr);
		glfwMakeContextCurrent(window);
		return window;
	}

	void gl_debug_message_callback(
		GLenum source __attribute__((unused)),
		GLenum type,
		GLuint id __attribute__((unused)),
		GLenum severity,
		GLsizei length __attribute__((unused)),
		const GLchar* message,
		const void* userParam __attribute__((unused))
	) {
		switch (type) {
			case GL_DEBUG_TYPE_ERROR:
				std::cerr << "GL error: type = " << std::hex << type << ", severity = " << std::hex << severity << ", message = " << message << std::endl;
				todo();
			case GL_DEBUG_TYPE_OTHER:
				std::cout << "GL message: " << message << std::endl;
				break;
			default:
				todo();
		}
	}
}

Graphics Graphics::start(const RenderableModel& renderable_model) {
	GLFWwindow* window = init_glfw();

	init_glew();

	glEnable(GL_TEXTURE_2D);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_debug_message_callback, nullptr);

	FrameBuffer frame_buffer = create_framebuffer();

	VAO vao_tris = create_and_bind_vao();
	VBO vbo_tris = create_and_bind_vertex_buffer(renderable_model.tris);
	Shaders shaders_tri = init_shaders("tri", ShadersKind::Tri);
	TriUniforms uniforms_tri { get_uniform(shaders_tri, "u_transform") };
	VAOInfo<TriUniforms> vao_info_tris { vao_tris, vbo_tris, shaders_tri, uniforms_tri };

	VAO vao_dots = create_and_bind_vao();
	VBO vbo_dots = create_and_bind_vertex_buffer(renderable_model.dots);
	Shaders shaders_dot = init_shaders("dot", ShadersKind::Dot);
	DotUniforms uniforms_dot { get_uniform(shaders_dot, "u_model"), get_uniform(shaders_dot, "u_transform"), get_uniform(shaders_dot, "u_materials"), get_uniform(shaders_dot, "u_depth_texture") };
	VAOInfo<DotUniforms> vao_info_dots { vao_dots, vbo_dots, shaders_dot, uniforms_dot };

	Texture loaded_texture = load_texture().first;

	return { new GraphicsImpl { window, vao_info_tris, vao_info_dots, frame_buffer, loaded_texture } };
}

bool Graphics::window_should_close() {
	return glfwWindowShouldClose(_impl->window);
}

Graphics::~Graphics() {
	delete _impl;
	//TODO: free VAO
}

void Graphics::render(float time) {
	_impl->render(time);
}
