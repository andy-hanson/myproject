#include <cassert>
#include <iostream> // std::cerr
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <limits>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <vector>
#include <sstream>

#include "./util/io.h"
#include "./util/Matrix.h"
#include "./util/read_png.h"
#include "./util/read_obj.h"

namespace {
	struct ShaderProgram { GLuint id; };

	GLint to_glint(unsigned long l) {
		assert(l < std::numeric_limits<GLint>::max());
		return static_cast<GLint>(l);
	}

	struct VBO {
		GLuint id;
		u32 n_vertices;

		VBO(const VBO& other) = delete;
		//VBO(VBO&& other) = default;
	};

	struct VAO { GLuint id; };


	// Note: every member must be a float, since we pass this to glVertexAttribPointer.
	struct VertexAttributes {
		glm::vec3 pos;
		glm::vec3 color;
		//glm::vec2 texture_coords;
	} __attribute__((packed));


	struct Stroke {
		glm::vec3 pos;
		glm::vec3 color;
	};

	struct Triangle {
		glm::vec3 p0;
		glm::vec3 p1;
		glm::vec3 p2;
	};

	struct Normals {
		glm::vec3 n0;
		glm::vec3 n1;
		glm::vec3 n2;
	};

	Triangle face_vertices(const Face& face, const Model& m) {
		return { m.vertices[face.vertex_0], m.vertices[face.vertex_1], m.vertices[face.vertex_2] };
	};

	__attribute__((unused))
	Normals face_normals(const Face& face, const Model& m) {
		return { m.normals[face.normal_0], m.normals[face.normal_1], m.normals[face.normal_2] };
	}

	double triangle_area(const Triangle& t) {
		glm::dvec3 va = t.p1 - t.p0;
		glm::dvec3 vb = t.p2 - t.p0;
		return glm::cross(va, vb).length() * 0.5;
	}

	__attribute__((unused))
	double compute_total_area(const Model& m) {
		double total = 0;
		for (const Face& face : m.faces)
			total += triangle_area(face_vertices(face, m));
		return total;
	}

	__attribute__((unused))
	uint round_to_uint(double d) {
		assert(d >= 0);
		return static_cast<uint>(round(d));
	}

	//TODO:MOVE
	DynArray<VertexAttributes> convert_model(const Model& m) {
		DynArray<VertexAttributes> out { m.faces.size() * 3 };
		uint i = 0;
		for (const Face& face : m.faces) {
			Triangle tri = face_vertices(face, m);
			//TODO: interpolate between vertex normals -- needs adjacent faces
			//Normals normals = face_normals(face, m);
			const Material& material = m.materials[face.material];
			//TODO: use the other attributes!
			Color color = material.kd;

			out[i++] = { tri.p0, color.vec3() };
			out[i++] = { tri.p1, color.vec3() };
			out[i++] = { tri.p2, color.vec3() };
		}
		assert(i == out.size());
		return out.copy_slice(out.slice()); //TODO: don't copy
	};

	//If changing this, must also change the call to `glVertexAttribPointer`
	VBO create_and_bind_vertex_buffer(const Model& model) {
		/*VertexAttributes Vertices[3] = {
			{ { -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },//, { 0.0f, 0.0f } },
			{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },//, { 1.0f, 0.0f } },
			{ {  0.0f,  1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f } },//, { 0.0f, 1.0f } },
		};*/
		DynArray<VertexAttributes> vertices = convert_model(model);
		GLuint vbo_id;
		glGenBuffers(1, &vbo_id);
		assert(vbo_id != 0);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexAttributes), vertices.begin(), GL_STATIC_DRAW);
		return { vbo_id, vertices.size() };
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

	struct Shaders {
		ShaderProgram program;
		GLuint vertex;
		GLuint fragment;

		Shaders(const Shaders& other) = delete;
		Shaders(Shaders&& other) = default;
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

	struct Uniform { GLint id; };
	struct Uniforms {
		//Uniform uniColor;
		Uniform transform;
	};

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

	__attribute__((unused))
	float flo(float f) {
		return static_cast<float>(floor(static_cast<double>(f)));
	}

	GLsizei u32_to_glsizei(u32 u) {
		assert(u < std::numeric_limits<GLsizei>::max());
		return static_cast<GLsizei>(u);
	}

	void render(const Uniforms& uniforms, const VBO& vbo, float time) {
		//glUniform3f(uniforms.uniColor.id, time - flo(time), 0.0f, 0.0f);
		glm::mat4 trans = total_matrix(time);
		glUniformMatrix4fv(uniforms.transform.id, 1, /*transpose*/ GL_FALSE, glm::value_ptr(trans));

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.1f, 0.0f);

		glDrawArrays(GL_TRIANGLES, 0, u32_to_glsizei(vbo.n_vertices));
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

	Texture bind_texture(const Matrix<u32>& mat) {
		// Generate the OpenGL texture object
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, to_glsizei(mat.width()), to_glsizei(mat.height()), 0, GL_RGBA, GL_UNSIGNED_BYTE, mat.raw());
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

	VAO create_and_bind_vao() {
		GLuint vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		return VAO { vao };
	}

	u32 safe_div(u32 a, u32 b) {
		assert(a % b == 0);
		return a / b;
	}

	Shaders init_shaders() {
		Shaders shaders = compile_shaders();

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

	Uniform get_uniform(const Shaders& shaders, const char* name) {
		GLint id = glGetUniformLocation(shaders.program.id, name);
		assert(id != -1); // NOTE: if this fails, perhaps the uniform was unused
		return Uniform { id };
	}


	void play_game(const Model& model) {
		GLFWwindow* window = init_glfw();
		init_glew();

		VAO vao __attribute__((unused)) = create_and_bind_vao();

		VBO vbo = create_and_bind_vertex_buffer(model);

		Shaders shaders = init_shaders();

		glEnable(GL_DEPTH_TEST); //TODO: do this closer to where it's needed, and pair with glDIsable

		//load_texture();
		//glUniform1i(get_uniform(shaders, "tex").id, 0);

		Uniforms uniforms { /*get_uniform(shaders, "u_triangleColor"),*/ get_uniform(shaders, "u_transform") };

		float time = 0;
		while (!glfwWindowShouldClose(window)) {
			time += 0.01;
			render(uniforms, vbo, time);
			glfwSwapBuffers(window);
			glfwPollEvents();
		}

		free_shaders(shaders);

		//TODO: destructor
		glDeleteBuffers(1, &vbo.id);

		//TODO: free VAO

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	//todo:move
	/*void gen_strokes(const Model& m) {
		double total_area = compute_total_area(m);

		double density = m.vertices.size() / total_area;

		double vertices_owed = 0;

		for (const Face& face : m.faces) {
			Triangle tri = face_vertices(face, m);
			//TODO: interpolate between vertex normals -- needs adjacent faces
			Normals normals = face_normals(face, m);
			const Material& material = m.materials[face.material];

			double area = triangle_area(tri);

			vertices_owed += area * density;
			uint n_face_points = round_to_uint(vertices_owed);
			vertices_owed -= n_face_points;

			for (uint i = n_face_points; i != 0; --i) {
				//Get the material here

				//random_point_normal_uv_in_triangle(tri, normals, textures);
			}
		}
	}*/

}

int main() {
	std::string cwd = get_current_directory();
	std::string mtl_source = read_file(cwd + "/models/cube2.mtl");
	std::string obj_source = read_file(cwd + "/models/cube2.obj");
	Model m = parse_model(mtl_source.c_str(), obj_source.c_str());
	//gen_strokes(m);

	play_game(m);
}
