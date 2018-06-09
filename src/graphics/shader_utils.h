#pragma once

#include <string>
#include "./gl_types.h"

enum class ShadersKind { Tri, Dot };
Shaders compile_shaders(const std::string& name, const std::string& cwd);
Shaders set_attrib_pointers(const Shaders& shaders, ShadersKind kind);
