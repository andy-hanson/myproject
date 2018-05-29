#include "./process_model.h"

#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp> // cross

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
		return { m.vertices[face.vertex_0], m.vertices[face.vertex_1], m.vertices[face.vertex_2] };
	};

	Normals face_normals(const Face& face, const Model& m) {
		return { m.normals[face.normal_0], m.normals[face.normal_1], m.normals[face.normal_2] };
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

	struct PointNormal { glm::vec3 point; glm::vec3 normal; };
	// Uses barycentric coordinates
	PointNormal random_point_normal_in_triangle(const Triangle& tri, const Normals& normals) {
		float rand0 = random(); //TODO:BETTER random
		float rand1 = random();
		float alpha = 1 - float_sqrt(rand0);
		float beta = rand1 * (1 - alpha);
		float gamma = 1 - alpha - beta;

		glm::vec3 point = tri.p0 * alpha + tri.p1 * beta + tri.p2 * gamma;
		glm::vec3 normal = normals.n0 * alpha + normals.n1 * beta + normals.n2 * gamma;
		return { point, normal };
	}

	__attribute__((unused))
	DynArray<VertexAttributes> gen_strokes(const Model& m) {
		double total_area = compute_total_area(m);

		u32 n_strokes = m.vertices.size();

		double density = n_strokes / total_area;

		double vertices_owed = 0;

		uint out_i = 0;
		DynArray<VertexAttributes> out { n_strokes };

		for (const Face& face : m.faces) {
			Triangle tri = face_vertices(face, m);
			//TODO: interpolate between vertex normals -- needs adjacent faces
			Normals normals = face_normals(face, m);
			const Material& material = m.materials[face.material];

			double area = triangle_area(tri);

			vertices_owed += area * density;
			uint n_face_points = round_to_u32(vertices_owed);
			vertices_owed -= n_face_points;

			for (uint i = n_face_points; i != 0; --i) {
				PointNormal pn = random_point_normal_in_triangle(tri, normals);
				//todo: normal should be in attributes
				out[out_i++] = VertexAttributes { pn.point, material.kd.vec3() };
			}
		}

		assert(out_i == out.size());
		return out;
	}

//TODO:MOVE
	DynArray<VertexAttributes> get_triangles(const Model& m) {
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
		return out;
	};
}

//TODO:MOVE
RenderableModel convert_model(const Model& m) {
	//TODO: second call should be gen_strokes (for now, we'll just draw dots on the vertices to debug)
	return { get_triangles(m), get_triangles(m) };
};
