#include "Graphics.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <iostream> // std::cerr

#include "../util/DynArray.h"
#include "../util/int.h"
#include "../util/Matrix.h"
#include "../util/assert.h"
#include "../util/math.h"

#include "./convert_model.h"
#include "./gl_types.h"
#include "./read_png.h"
#include "./shader_utils.h"

namespace {
	//TODO: not const
	const uint VIEWPORT_WIDTH = 1024;
	const uint VIEWPORT_HEIGHT = 1024;
}

namespace {

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
	__attribute__((unused))
	std::pair<Texture, Size> load_texture(const std::string& cwd) {
		Matrix<u32> m = png_texture_load((cwd + "/textures/foo.png").c_str());
		return { bind_texture_from_image(m), { m.width(), m.height() } };
		//TODO: how to remove the texture when done?
	}

	//TODO:MOVE
	struct Matrices {
		glm::mat4 model;
		glm::mat4 viewModel; // excludes proj
		glm::mat4 total;
	};
	Matrices get_matrices() {
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

	struct RenderableModelInfo {
		RenderableModel renderable_model;
		VAOInfo vao_info_tris;
		VAOInfo vao_info_dots;

		void free() {
			vao_info_tris.free();
			vao_info_dots.free();
		}
	};
}

struct GraphicsImpl {
	GLFWwindow* window;
	FrameBuffer frame_buffer;
	ShadersInfo<TriUniforms> tri_shader_info;
	ShadersInfo<DotUniforms> dot_shader_info;
	// This should be as long as ModelKind has entries. (TODO: use a fixed-size array then.)
	std::vector<RenderableModelInfo> renderable_models;

	u32 model_kind_to_u32(ModelKind m) {
		return u32(m);
	}

	void render(Slice<DrawEntity> to_draw) {
		//glUniform3f(uniforms.uniColor.id, time - flo(time), 0.0f, 0.0f);
		Matrices matrices = get_matrices();

		//TODO: sort to_draw by the model

		// In the tri pass: set stencil buffer to object_id wherever we draw a pixel.
		//constexpr int object_id = 1; // TODO
		//glStencilOp(/*stencil fail*/ GL_REPLACE, /*stencil pass, depth fail*/ GL_REPLACE, /*stencil pass, depth pass*/ GL_REPLACE);
		//glStencilFunc(GL_ALWAYS, object_id, 0xff);

		enabling(GL_DEPTH_TEST, [&]() {
			// First pass: draw to the frame buffer
			//glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.id);
			//TODO:needed?
			glViewport(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

			glClear(static_cast<uint>(GL_COLOR_BUFFER_BIT) | static_cast<uint>(GL_DEPTH_BUFFER_BIT));
			glClearColor(0.0f, 0.0f, 0.1f, 0.0f);

			tri_shader_info.shaders.use();

			// Draw triangles
			for (const DrawEntity& d : to_draw) {
				const RenderableModelInfo& r = renderable_models[model_kind_to_u32(d.model)];
				r.vao_info_tris.vao.bind();
				glUniformMatrix4fv(tri_shader_info.uniforms.u_transform.id, 1, /*transpose*/ GL_FALSE, glm::value_ptr(matrices.total));
				r.vao_info_tris.vbo.bind();
				glDrawArrays(GL_TRIANGLES, 0, u32_to_glsizei(r.vao_info_tris.vbo.n_vertices));
			}

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

		//TODO:MOVE
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

		if ((false)) {
			enabling(GL_VERTEX_PROGRAM_POINT_SIZE, [&]() {
				// Now render to screen.
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				//TODO:needed?
				glViewport(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

				glClear(static_cast<uint>(GL_COLOR_BUFFER_BIT));
				glClearColor(0.0f, 0.0f, 0.1f, 0.0f);

				dot_shader_info.shaders.use();

				for (const DrawEntity& d : to_draw) {
					const RenderableModelInfo& r = renderable_models[model_kind_to_u32(d.model)];

					const VAOInfo& dots = r.vao_info_dots;
					dots.vao.bind();
					glUniformMatrix4fv(dot_shader_info.uniforms.u_model.id, 1, /*transpose*/ GL_FALSE, glm::value_ptr(matrices.model));
					glUniformMatrix4fv(dot_shader_info.uniforms.u_transform.id, 1, /*transpose*/ GL_FALSE, glm::value_ptr(matrices.total));
					glUniform1fv(dot_shader_info.uniforms.u_materials.id, MAX_MATERIALS * sizeof(Material) / sizeof(float), reinterpret_cast<float*>(materials));

					//TODO: check for error after trying to set uniform?
					//TODO: Yes this is tricky. Have to bind a texture and set the uniform to the texture *unit*, not the texture id.
					glBindTexture(GL_TEXTURE_2D, frame_buffer.output_texture.id);
					glUniform1i(dot_shader_info.uniforms.u_depth_texture.id, 0);//frame_buffer.output_texture.id); //TODO: just guessing here

					dots.vbo.bind();
					glDrawArrays(GL_POINTS, 0, u32_to_glsizei(dots.vbo.n_vertices));
				}
			});
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	~GraphicsImpl() {
		for (RenderableModelInfo& i : renderable_models) {
			i.free();
		}
		frame_buffer.free();

		tri_shader_info.free();
		dot_shader_info.free();

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

Graphics Graphics::start(const Model& model, const std::string& cwd) {
	GLFWwindow* window = init_glfw();

	init_glew();

	glEnable(GL_TEXTURE_2D);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_debug_message_callback, nullptr);

	FrameBuffer frame_buffer = create_framebuffer();

	Shaders shaders_tri = init_shaders("tri", cwd, ShadersKind::Tri);
	TriUniforms uniforms_tri { get_uniform(shaders_tri, "u_transform") };

	Shaders shaders_dot = init_shaders("dot", cwd, ShadersKind::Dot);
	DotUniforms uniforms_dot { get_uniform(shaders_dot, "u_model"), get_uniform(shaders_dot, "u_transform"), get_uniform(shaders_dot, "u_materials"), get_uniform(shaders_dot, "u_depth_texture") };

	GraphicsImpl* res = new GraphicsImpl { window, frame_buffer, ShadersInfo<TriUniforms> { shaders_tri, uniforms_tri }, ShadersInfo<DotUniforms> { shaders_dot, uniforms_dot }, {} };

	{
		RenderableModel renderable_model = convert_model(model);

		VAO vao_tris = create_and_bind_vao();
		VBO vbo_tris = create_and_bind_vertex_buffer(renderable_model.tris);
		VAOInfo vao_info_tris { vao_tris, vbo_tris };

		VAO vao_dots = create_and_bind_vao();
		VBO vbo_dots = create_and_bind_vertex_buffer(renderable_model.dots);
		VAOInfo vao_info_dots { vao_dots, vbo_dots };

		res->renderable_models.push_back(RenderableModelInfo { std::move(renderable_model), vao_info_tris, vao_info_dots });
	}

	return { res };
}

bool Graphics::window_should_close() {
	return int_to_bool(glfwWindowShouldClose(_impl->window));
}

Graphics::~Graphics() {
	delete _impl;
	//TODO: free VAO
}

void Graphics::render(Slice<DrawEntity> to_draw) {
	_impl->render(to_draw);
}
