#include "./Physics.h"

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#pragma clang diagnostic ignored "-Winconsistent-missing-destructor-override"
#pragma clang diagnostic ignored "-Wshadow-field-in-constructor"
#pragma clang diagnostic ignored "-Wdeprecated"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wweak-vtables"
#include "reactphysics3d.h"
#include "collision/ContactManifold.h" // TODO: seems hacky that I have to reference this to get ContactManifoldListElement iteration to work...
#include "constraint/ContactPoint.h"
#pragma clang diagnostic pop

#include "../util/UniquePtr.h"


namespace {
	glm::vec3 vec3_from_rp3d(const rp3d::Vector3& v) {
		return glm::vec3 { v.x, v.y, v.z };
	}

	glm::quat quat_from_rp3d(const rp3d::Quaternion& q) {
		return glm::quat { q.w, q.x, q.y, q.z };
	}

	Transform transform_from_rp3d(const rp3d::Transform& t) {
		return Transform { vec3_from_rp3d(t.getPosition()), quat_from_rp3d(t.getOrientation()) };
	}

	rp3d::Vector3 vec3_to_rp3d(const glm::vec3& v) {
		return rp3d::Vector3 { v.x, v.y, v.z };
	}

	rp3d::Quaternion quat_to_rp3d(const glm::quat& q) {
		return rp3d::Quaternion { q.w, q.x, q.y, q.z };
	}

	rp3d::Transform transform_to_rp3d(const Transform& t) {
		return rp3d::Transform { vec3_to_rp3d(t.position), quat_to_rp3d(t.quat) };
	}
}

namespace {

	// Should exist one of these per model.
	struct ConcaveMesh {
		DynArray<float> vertices; //TODO:PERF share with the Model instead of copying?
		DynArray<int> indices;
		UniquePtr<rp3d::TriangleVertexArray> triangle_array;
		UniquePtr<rp3d::TriangleMesh> triangle_mesh;
		UniquePtr<rp3d::ConcaveMeshShape> shape;//TODO:own
	};

	ConcaveMesh make_concave_mesh(const Model& model) {
		Slice<glm::vec3> m_vertices = model.vertices.slice();
		DynArray<float> vertices = DynArray<float>::uninitialized(m_vertices.size() * 3);
		uint i = 0;
		for (const glm::vec3& v : m_vertices) {
			vertices[i++] = v.x; vertices[i++] = v.y; vertices[i++] = v.z;
		}
		assert(i == vertices.size());

		Slice<Face> m_faces = model.faces.slice();
		uint n_faces = m_faces.size();
		DynArray<int> indices = DynArray<int>::uninitialized(m_faces.size() * 3);
		i = 0;
		// NOTE: Vertices should be specified in counter-clockwise order, as seen from the outside of the mesh.
		for (const Face& f : m_faces) {
			indices[i++] = f.vertex_0; indices[i++] = f.vertex_1; indices[i++] = f.vertex_2;
		}
		assert(i == indices.size());

		ConcaveMesh res {
			std::move(vertices),
			std::move(indices),
			{},
			{},
			{},
		};

		res.triangle_array = UniquePtr { new rp3d::TriangleVertexArray(
			m_vertices.size(), res.vertices.begin(), 3 * sizeof(float), n_faces, res.indices.begin(), 3 * sizeof(int),
			rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE, rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE), //TODO:PERF use shorts
		};
		res.triangle_mesh = UniquePtr { new rp3d::TriangleMesh{} };
		res.triangle_mesh->addSubpart(res.triangle_array.ptr());

		float scale = 1.0;
		res.shape = UniquePtr { new rp3d::ConcaveMeshShape(res.triangle_mesh.ptr(), rp3d::Vector3 { scale, scale, scale }) };

		return res;
	}

	//NOTE: height field shapes also exist.

	//NOTE: rp3d supports collision filtering, if some objects will only collide with some other objects.
	// Note: intentionally not using 'enum class' since these should convert to uint.
	//Note: to work the collision must be specified both ways -- if player specifies hazard, hazard must specify player.
	namespace CollisionFlags {
		const uint Player = 0x01;
		const uint Hazard = 0x02;
		//const uint SomethingYetMoreElse = 0x04;
	};


	rp3d::Transform transform_identity() { return rp3d::Transform { rp3d::Vector3 { 0.0, 0.0, 0.0 }, rp3d::Quaternion::identity() }; }

	class MyCollisionCallback final : public rp3d::CollisionCallback {
		void notifyContact(const CollisionCallbackInfo& info) override {
			//info.body1;
			//info.body2;
			//info.proxyShape1;
			//info.proxyShape2;
			//this is a linked list
			for (rp3d::ContactManifoldListElement* list_ptr = info.contactManifoldElements; list_ptr != nullptr; list_ptr = list_ptr->getNext()) {
				//do useful stuff
				rp3d::ContactManifold* manifold = list_ptr->getContactManifold();
				manifold->getBody1();
				manifold->getBody2();
				manifold->getShape1();
				manifold->getShape2();
				rp3d::ContactPoint* begin_contact_points = manifold->getContactPoints();
				uint n_contact_points = int_to_uint(manifold->getNbContactPoints());
				for (uint i = 0; i != n_contact_points; ++i) {
					const rp3d::ContactPoint& point = begin_contact_points[i];
					point.getNormal();
					point.getLocalPointOnShape1();
					point.getLocalPointOnShape2();
					point.getIsRestingContact();
				}
			}
		}
	};

	/*
	void test_physics() {
		rp3d::CollisionWorld world;

		rp3d::Vector3 init_position { 0.0, 3.0, 0.0 };
		rp3d::Quaternion init_orientation = rp3d::Quaternion::identity();

		rp3d::CollisionBody* body = world.createCollisionBody(rp3d::Transform  { init_position, init_orientation });
		body->getTransform(); //get it back

		rp3d::ProxyShape* proxy_shape = body->addCollisionShape(make_concave_mesh(), transform_identity());
		proxy_shape->setCollisionCategoryBits(CollisionFlags::Player);
		proxy_shape->setCollideWithMaskBits(CollisionFlags::Player | CollisionFlags::Hazard);

		const bool do_overlap __attribute__((unused)) = world.testOverlap(body, body);

		//const rp3d::Vector3 half_extents { 2.0, 3.0, 5.0 };
		//const rp3d::BoxShape box_shape __attribute__((unused)) { half_extents };

		//const rp3d::SphereShape sphere_shape __attribute__((unused)) { 2.0 };

		// looks like (__)
		//const rp3d::CapsuleShape capsule_shape { / *radius* / 1.0, / *height* / 2.0 };

		rp3d::Ray ray { rp3d::Vector3 { 0.0, 5.0, 1.0 }, rp3d::Vector3 { 0.0, 5.0, 30.0 } };
		rp3d::RaycastInfo raycast_info;
		//Note: rp3d has methods of raycasting against every body, but don't think I need those
		bool did_hit = body->raycast(ray, raycast_info); // writes to raycat_info
		if (did_hit) {
			//int tri_idx = raycast_info.triangleIndex;
			//raycast_info.worldPoint; //where we hit
			//raycast_info.worldNormal; //TODO: maybe it would be better to interpolate from the triangle?
			//raycast_info.hitFraction; // Fraction from startPoint to endPoint -- which gives me the distance to the hit point.
			//raycast-info.body and raycast_info.proxyShape are uninteresting when testing against a single body
		}


		//getting collision with hazards...
		//elso exist methods for testing collision of single objects
		MyCollisionCallback callback {};
		world.testCollision(&callback);


		world.destroyCollisionBody(body);

		//world.testCollision();
		//world.testAABBOverlap();
	}*/
}

struct PhysicsImpl {
	rp3d::CollisionWorld world;
	DynArray<ConcaveMesh> meshes; // length is # of models
};

Physics::Physics(Slice<Model> models) {
	impl = new PhysicsImpl { rp3d::CollisionWorld {},  map<ConcaveMesh>{}(models, make_concave_mesh) };
}

Physics::~Physics() {
	delete impl;
}

Ref<rp3d::CollisionBody> Physics::add_body(ModelKind model, const Transform& transform) {
	rp3d::CollisionBody* body = impl->world.createCollisionBody(transform_to_rp3d(transform));

	rp3d::ProxyShape* proxy_shape = body->addCollisionShape(impl->meshes[model_kind_to_u32(model)].shape.ptr(), transform_identity());
	//TODO
	proxy_shape->setCollisionCategoryBits(CollisionFlags::Player);
	proxy_shape->setCollideWithMaskBits(CollisionFlags::Player | CollisionFlags::Hazard);

	return Ref { body };
}

void Physics::remove_body(Ref<rp3d::CollisionBody> body) {
	impl->world.destroyCollisionBody(body.ptr());
}

Transform Physics::get_transform(Ref<rp3d::CollisionBody> body) {
	return transform_from_rp3d(body->getTransform());
}
