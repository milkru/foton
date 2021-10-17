#pragma once

// TODO: If some of these includes are used in a single .cpp file, move it over there!!!

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <TextEditor.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glslang_c_interface.h>
#include <StandAlone/ResourceLimits.h>

#include <spirv_reflect.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define NFD_THROWS_EXCEPTIONS
#include <nfd.hpp>

// TODO: Strip down some of these includes.
#include <algorithm>
#include <chrono>
#include <vector>
#include <array>
#include <set>

#include "utils/assert.hpp"
#include "utils/log.h"
#include "utils/profile.hpp"
