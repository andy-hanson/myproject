#include <sstream>
#include <vector>

#include "./assert.h"
#include "./read_obj.h"

namespace {

	std::string read_str(std::istringstream& s) {
		std::string res;
		s >> res;
		return res;
	}

	void expect_str(std::istringstream& s, const char* expected) {
		std::string str = read_str(s);
		check(str == expected);
	};

	u8 read_u8(std::istringstream& s) {
		u64 u;
		s >> u;
		return u64_to_u8(u);
	}

	// Some indices are 1-based in the .obj file, but we want 0-based.
	u8 read_u8_minus_one(std::istringstream& s) {
		u8 u = read_u8(s);
		check(u != 0);
		return static_cast<u8>(u - 1);
	}

	float read_float(std::istringstream& s) {
		float f;
		s >> f;
		return f;
	}

	glm::vec3 read_vec3(std::istringstream& s) {
		float x = read_float(s);
		float y = read_float(s);
		float z = read_float(s);
		return { x, y, z };
	}

	Color read_color(std::istringstream& s) {
		return Color { read_vec3(s) };
	}

	char read_char(std::istringstream& s) {
		int i = s.get();
		check(i >= std::numeric_limits<char>::min() && i <= std::numeric_limits<char>::max());
		return static_cast<char>(i);
	}

	void expect_char(std::istringstream& s, char expected) {
		char c = read_char(s);
		check(c == expected);
	}

	struct FacePart { u8 vertex; u8 normal; };
	FacePart read_face_part(std::istringstream& s) {
		u8 vertex = read_u8_minus_one(s);
		expect_char(s, '/');
		expect_char(s, '/');
		u8 normal = read_u8_minus_one(s);
		return { vertex, normal };
	}

	void read_face(std::istringstream& s, u8 material, std::vector<Face>& faces) {
		FacePart a = read_face_part(s);
		FacePart b = read_face_part(s);
		FacePart c = read_face_part(s);
		faces.push_back(Face { material, a.vertex, b.vertex, c.vertex, a.normal, b.normal, c.normal });
		if (read_char(s) == '\n') return;

		FacePart d = read_face_part(s);
		faces.push_back(Face { material, c.vertex, d.vertex, a.vertex, c.normal, d.normal, a.normal });
	}

	void skip_line(std::istringstream& s) {
		std::string str;
		getline(s, str);
	}

	void skip_comments(std::istringstream& s) {
		char c = read_char(s);
		check(c == '#');
		skip_line(s);
		c = read_char(s);
		check(c == '#');
		skip_line(s);
	}

	void parse_materials(const std::string& file_content, std::vector<std::string>& material_names, std::vector<Material>& materials) {
		std::istringstream s { file_content };
		skip_comments(s);

		while (!s.eof()) {
			std::string newmtl = read_str(s);
			if (newmtl != "newmtl") {
				check(newmtl == "");
				break;
			}

			std::string name = read_str(s);

			expect_str(s, "Ns");
			float ns = read_float(s);
			expect_str(s, "Ka");
			Color ka = read_color(s);
			expect_str(s, "Kd");
			Color kd = read_color(s);
			expect_str(s, "Ks");
			Color ks = read_color(s);
			expect_str(s, "Ke");
			Color ke = read_color(s);
			expect_str(s, "Ni");
			float ni = read_float(s);
			expect_str(s, "d");
			float d = read_float(s);
			expect_str(s, "illum");
			u8 illum = read_u8(s);

			material_names.push_back(name);
			// First id is 1
			uint id = u64_to_u32(materials.size() + 1);
			materials.push_back(Material { id, ns, ka, kd, ks, ke, ni, d, illum });
		}
	}
}

Model parse_model(const char* mtl_source, const char* obj_source) {
	std::vector<std::string> material_names;
	std::vector<Material> materials;
	parse_materials(mtl_source, material_names, materials);

	std::istringstream s { obj_source };

	skip_comments(s);

	expect_str(s, "mtllib");
	expect_str(s, "cube2.mtl");
	expect_str(s, "o");
	expect_str(s, "Sphere_Sphere.001");

	// Read vertices
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<Face> faces;

	u8 current_material;

	auto set_current_material = [&]() {
		std::string str = read_str(s);
		current_material = index_of(vec_to_slice(material_names), str);
	};

	while (true) {
		std::string str = read_str(s);
		if (str == "v") {
			vertices.push_back(read_vec3(s));
		} else {
			check(str == "vn");
			normals.push_back(read_vec3(s));
			break;
		}
	}
	while (true) {
		std::string str = read_str(s);
		if (str == "vn") {
			normals.push_back(read_vec3(s));
		} else {
			check(str == "usemtl");
			set_current_material();
			break;
		}
	}

	expect_str(s, "s");
	expect_str(s, "1"); //TODO: wtf is this?

	while (true) {
		std::string str = read_str(s);
		if (str == "f")
			read_face(s, current_material, faces);
		else if (str == "usemtl")
			set_current_material();
		else if (str.empty())
			break;
		else
			todo();
	}

	return { vec_to_dyn_array(materials), vec_to_dyn_array(vertices), vec_to_dyn_array(normals), vec_to_dyn_array(faces) };
}
