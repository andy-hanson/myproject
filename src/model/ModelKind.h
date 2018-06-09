#pragma once

enum class ModelKind {
	Player,

	Cylinder,


	// Last entry -- this is the # of models.
	COUNT,
};

const u32 N_MODELS = u32(ModelKind::COUNT);

inline u32 model_kind_to_u32(ModelKind m) {
	return u32(m);
}

