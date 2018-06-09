#include <GL/glew.h>
#include <iostream>
#include <thread>

#include "./util/io.h"
#include "./util/float.h"
#include "./util/Matrix.h"

#include "graphics/convert_model.h"
#include "graphics/Graphics.h"

#include "./vendor/readerwriterqueue/readerwriterqueue.h"

#include "./audio/audio.h"
#include "./audio/audio_file.h"
#include "./audio/read_wav.h"
#include "./audio/read_ogg.h"
#include "./control/Controller.h"

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

#include "./util/Ref.h"
#include "./game.h"

namespace {
	template <typename Cb>
	auto print_time(const char* desc, Cb cb) {
		std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
		auto res = cb();
		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

		std::cout << desc << " took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
		return res;
	}

	void test_sound() {
		DecodedAudioFile wavvy = print_time("wav", []() { return read_wav(); });
		DecodedAudioFile vorby = print_time("ogg", []() { return read_ogg("/home/andy/CLionProjects/myproject/audio/awe.ogg"); });

		std::cout << "size: " << wavvy.floats.size() << "   " << vorby.floats.size() << std::endl;

		bool prefer_wav = false;

		Audio audio = Audio::start();
		audio.play(prefer_wav ? wavvy.floats.slice() : vorby.floats.slice());
		std::this_thread::sleep_for(std::chrono::seconds{5});
	}
}

namespace {

	void test_input() {
		Controller controller = Controller::start();
		for (uint i = 0; i != 100; ++i) {
			ControllerGet g = controller.get();
			std::cout << g.button_is_down << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}

namespace {
	// Note: this is slower, use only for environment.
	//TODO:THIS CAUSES INVALID POINTERZZZ
	rp3d::ConcaveMeshShape* make_concave_mesh() {
		//Note: rp3d just points into this data, doesn't copy it. So must keep alive!
		//Can also share with the graphics system.

		constexpr uint n_vertices = 3;
		const uint n_triangles = 1;
		float vertices[3 * n_vertices] = {
			-3, -3, 3,
			3, -3, 3,
			3, -3, -3,
		};
		// NOTE: Vertices should be specified in counter-clockwise order, as seen from the outside of the mesh.
		int indices[3 * n_triangles] = { 0, 1, 2 };

		rp3d::TriangleVertexArray* triangle_array = new rp3d::TriangleVertexArray(
			n_vertices, vertices, 3 * sizeof(float), n_triangles, indices, 3 * sizeof(int),
			rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE, rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE);

		// need to keep this around!
		rp3d::TriangleMesh* triangle_mesh = new rp3d::TriangleMesh{};
		triangle_mesh->addSubpart(triangle_array);
		float scale = 1.0;
		rp3d::ConcaveMeshShape* concave_mesh __attribute__((unused)) = new rp3d::ConcaveMeshShape(triangle_mesh, rp3d::Vector3 { scale, scale, scale });

		todo();
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

	// Note: the mesh must be convex.
	__attribute__((unused))
	void make_convex_mesh() {
		// NOTE: these are *not* copied, I need to keep them alive!
		float vertices[24] = {
			-3, -3, 3,
			3, -3, 3,
			3, -3, -3,
			-3, -3, -3,
			-3, 3, 3,
			3, 3, 3,
			3, 3, -3,
			-3, 3, -3,
		};

		// NOTE: these are *not* copied, I need to keep them alive!
		int indices[24] = {
			0, 3, 2, 1,
			4, 5, 6, 7,
			0, 1, 5, 4,
			1, 2, 6, 5,
			2, 3, 7, 6,
			0, 4, 7, 3,
		};

		rp3d::PolygonVertexArray::PolygonFace polygonFaces[6];
		// NOTE: these are *not* copied, I need to keep them alive!
		rp3d::PolygonVertexArray::PolygonFace* face = &polygonFaces[0];
		for (uint f = 0; f != 6; ++f) {
			// First vertex  of the  face in the  indices  array
			face->indexBase = f * 4;
			// Number  of  vertices  in the  face
			face->nbVertices = 4;
			++face;
		}

		// NOTE: these are *not* copied, I need to keep them alive!
		rp3d::PolygonVertexArray* polygonVertexArray = new rp3d::PolygonVertexArray(
			8, vertices, 3 * sizeof(float), indices, sizeof(int), 6, polygonFaces,
			rp3d::PolygonVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
			rp3d::PolygonVertexArray::IndexDataType::INDEX_INTEGER_TYPE);

		// NOTE: these are *not* copied, I need to keep them alive!
		rp3d::PolyhedronMesh* polyhedronMesh = new rp3d::PolyhedronMesh(polygonVertexArray);

		rp3d::ConvexMeshShape* convexMeshShape __attribute__((unused)) = new rp3d::ConvexMeshShape(polyhedronMesh);

		todo(); //todo: don't make invalid pointers
	}

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

	void test_physics() {
		rp3d::CollisionWorld world;

		rp3d::Vector3 init_position { 0.0, 3.0, 0.0 };
		rp3d::Quaternion init_orientation = rp3d::Quaternion::identity();
		rp3d::Transform transform { init_position, init_orientation };

		rp3d::CollisionBody* body = world.createCollisionBody(transform);

		rp3d::ProxyShape* proxy_shape = body->addCollisionShape(make_concave_mesh(), transform_identity());
		proxy_shape->setCollisionCategoryBits(CollisionFlags::Player);
		proxy_shape->setCollideWithMaskBits(CollisionFlags::Player | CollisionFlags::Hazard);

		const bool do_overlap __attribute__((unused)) = world.testOverlap(body, body);

		//const rp3d::Vector3 half_extents { 2.0, 3.0, 5.0 };
		//const rp3d::BoxShape box_shape __attribute__((unused)) { half_extents };

		//const rp3d::SphereShape sphere_shape __attribute__((unused)) { 2.0 };

		// looks like (__)
		//const rp3d::CapsuleShape capsule_shape { /*radius*/ 1.0, /*height*/ 2.0 };

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
	}
}

int main() {
	if ((false)) test_sound();
	if ((false)) test_input();
	if ((false)) test_physics();


	std::string cwd = get_current_directory();
	game(cwd);
}
