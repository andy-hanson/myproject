#pragma once

enum class ModelKind {
	Player,

	Stage0,
};
//We can easily map a ModelKind to the renderable model by an array lookup.
//graphics will need to map ModelKind to the list of positions at that location.
//So when we move an entity,
