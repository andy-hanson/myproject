#pragma once

#include <string>
#include "./gl_types.h"

enum class ShadersKind { Tri, Dot };
Shaders init_shaders(const std::string& name, const std::string& cwd, ShadersKind kind);
