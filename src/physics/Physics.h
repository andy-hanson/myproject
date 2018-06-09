#pragma once

#include "../util/Transform.h"
#include "../util/Ref.h"

#include "../model/Model.h"
#include "../model/ModelKind.h"

namespace reactphysics3d { class CollisionBody; }

using PhysicsBody = reactphysics3d::CollisionBody;

struct PhysicsImpl;

class Physics {
	PhysicsImpl* impl;

public:
	Physics(Slice<Model> models);
	Physics(const Physics& other) = delete;
	~Physics();

	Ref<PhysicsBody> add_body(ModelKind model, const Transform& transform);
	void remove_body(Ref<PhysicsBody> body);
	Transform get_transform(Ref<PhysicsBody> body);
};
