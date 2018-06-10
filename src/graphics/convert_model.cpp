#include "convert_model.h"

#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp> // cross
#include <random>

namespace {
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
		return Triangle { m.vertices[face.vertex_0], m.vertices[face.vertex_1], m.vertices[face.vertex_2] };
	};

	Normals face_normals(const Face& face, const Model& m) {
		return Normals { m.normals[face.normal_0], m.normals[face.normal_1], m.normals[face.normal_2] };
	}

	double triangle_area(const Triangle& t) {
		glm::dvec3 va = t.p1 - t.p0;
		glm::dvec3 vb = t.p2 - t.p0;
		return glm::cross(va, vb).length() * 0.5;
	}

	double compute_total_area(const Model& m) {
		double total = 0;
		for (const Face& face : m.faces)
			total += triangle_area(face_vertices(face, m));
		return total;
	}

	u32 round_to_u32(double d) {
		assert(d >= 0 && d < std::numeric_limits<u32>::max());
		return static_cast<u32>(round(d));
	}

	float float_sqrt(float f) {
		return static_cast<float>(sqrt(static_cast<double>(f)));
	}

	using Random = std::mt19937_64;

	bool about_equal(float a, float b) {
		return (a - 0.01f) < b && (a + 0.01f) > b;
	}

	struct PointNormal { glm::vec3 point; glm::vec3 normal; };
	// Uses barycentric coordinates
	PointNormal random_point_normal_in_triangle(const Triangle& tri, const Normals& normals, Random& rand) {
		std::uniform_real_distribution<> dis(0.0, 1.0);
		float rand0 = static_cast<float>(dis(rand)); //TODO:BETTER random
		float rand1 = static_cast<float>(dis(rand));
		float alpha = 1 - float_sqrt(rand0);
		float beta = rand1 * (1 - alpha);
		float gamma = 1 - alpha - beta;

		check(about_equal(alpha + beta + gamma, 1.0f));

		glm::vec3 point = tri.p0 * alpha + tri.p1 * beta + tri.p2 * gamma;
		glm::vec3 normal = normals.n0 * alpha + normals.n1 * beta + normals.n2 * gamma;
		return { point, normal };
	}

	float rand_range(float min, float max, Random& rand) {
		std::uniform_real_distribution<> dis(static_cast<double>(min), static_cast<double>(max));
		return static_cast<float>(dis(rand));
	}

	float max(float a, float b) { return a > b ? a : b; }
	float min(float a, float b) { return a < b ? a : b; }

	//TODO: use hsv
	__attribute__((unused))
	Color random_color_near(Color c, Random& rand) {
		auto fluctuate = [&](float f) { return rand_range(max(f - 0.1f, 0), min(f + 0.1f, 1.0), rand); };
		return Color { fluctuate(c.r), fluctuate(c.g), fluctuate(c.b) };
	}

	DynArray<VertexAttributesDotOrDebug> gen_dots(const Model& m, Random& rand) {
		double total_area = compute_total_area(m);

		u32 n_strokes = 500; // TODO: configurable

		double density = n_strokes / total_area;

		double vertices_owed = 0;

		uint out_i = 0;
		DynArray<VertexAttributesDotOrDebug> out = DynArray<VertexAttributesDotOrDebug>::uninitialized(n_strokes);

		for (const Face& face : m.faces) {
			Triangle tri = face_vertices(face, m);
			//TODO: interpolate between vertex normals -- needs adjacent faces
			Normals normals = face_normals(face, m);
			const ParsedMaterial& material = m.materials[face.material];

			double area = triangle_area(tri);

			vertices_owed += area * density;
			uint n_face_points = round_to_u32(vertices_owed);
			vertices_owed -= n_face_points;

			for (uint i = n_face_points; i != 0; --i) {
				PointNormal pn = random_point_normal_in_triangle(tri, normals, rand);
				//Color color = material.kd;//random_color_near(material.kd, rand);
				out[out_i++] = VertexAttributesDotOrDebug { pn.point, pn.normal, material.id };
			}
		}

		assert(out_i == out.size());
		return out;
	}

	DynArray<VertexAttributesTri> get_triangles(const Model& m) {
		DynArray<VertexAttributesTri> out = DynArray<VertexAttributesTri>::uninitialized(m.faces.size() * 3);
		uint i = 0;
		for (const Face& face : m.faces) {
			Triangle tri = face_vertices(face, m);
			const ParsedMaterial& material = m.materials[face.material];
			out[i++] = VertexAttributesTri { tri.p0, material.id };
			out[i++] = VertexAttributesTri { tri.p1, material.id };
			out[i++] = VertexAttributesTri { tri.p2, material.id };
		}
		assert(i == out.size());
		return out;
	};

	DynArray<VertexAttributesDotOrDebug> get_debug(const Model& m) {
		DynArray<VertexAttributesDotOrDebug> out = DynArray<VertexAttributesDotOrDebug>::uninitialized(m.faces.size() * 3);
		uint i = 0;
		for (const Face& face : m.faces) {
			Triangle tri = face_vertices(face, m);
			Normals normals = face_normals(face, m);
			const ParsedMaterial& material = m.materials[face.material];
			out[i++] = VertexAttributesDotOrDebug { tri.p0, normals.n0, material.id };
			out[i++] = VertexAttributesDotOrDebug { tri.p1, normals.n1, material.id };
			out[i++] = VertexAttributesDotOrDebug { tri.p2, normals.n2, material.id };
		}
		assert(i == out.size());
		return out;
	}
}

RenderableModel convert_model(const Model& m) {
	Random rand;
	return { get_triangles(m), gen_dots(m, rand), get_debug(m) };
};
