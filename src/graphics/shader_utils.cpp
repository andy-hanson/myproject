#include "./shader_utils.h"

#include <GL/glew.h>
#include <iostream> // cerr

#include "../util/io.h"
#include "../util/Slice.h"
#include "../util/string.h"

#include "./RenderableModel.h"

namespace {
	GLuint add_shader(ShaderProgram shader_program, Slice<char> shader_source, GLenum shader_type) {
		GLuint shader_id = glCreateShader(shader_type);
		check(shader_id != 0);

		GLint len = to_glint(shader_source.size());
		const char* begin = shader_source.begin();
		glShaderSource(shader_id, 1, &begin, &len);
		glCompileShader(shader_id);

		GLint success;
		glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
		if (success == 0) {
			GLchar InfoLog[1024];
			glGetShaderInfoLog(shader_id, 1024, nullptr, InfoLog);
			std::cerr << "Error compiling shader:" << std::endl << InfoLog << std::endl;
			todo();
		}

		glAttachShader(shader_program.id, shader_id);
		//TODO: glBindFragDataLocation(shader_program.id, 0, "outColor"); (it's the default though)

		return shader_id;
	}

	Shaders compile_shaders(const std::string& name, const std::string& cwd) {
		ShaderProgram shader_program { gluint_to_u32(glCreateProgram()) };
		check(shader_program.id != 0);

		std::string vs = read_file(cwd + "/shaders/" + name + ".vert");
		std::string fs = read_file(cwd + "/shaders/" + name + ".frag");

		GLuint vertex_shader_id = add_shader(shader_program, to_slice(vs), GL_VERTEX_SHADER);
		GLuint fragment_shader_id = add_shader(shader_program, to_slice(fs), GL_FRAGMENT_SHADER);

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
}

//TODO:NEATER
Shaders init_shaders(const std::string& name, const std::string& cwd, ShadersKind kind) {
	Shaders shaders = compile_shaders(name, cwd);

	uint stride = kind == ShadersKind::Tri ? sizeof(VertexAttributesTri) : sizeof(VertexAttributesDot);

	auto get_attrib = [&](const char* attr_name, uint expected_index) {
		GLuint attrib = glint_to_gluint(glGetAttribLocation(shaders.program.id, attr_name));
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

	return shaders;
}
