#pragma once

// TODO: If some of these includes are used in a single .cpp file, move it over there!!!

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <TextEditor.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define NFD_THROWS_EXCEPTIONS
#include <nfd.hpp>

#include <algorithm> // TODO: Needed?
#include <chrono>
#include <vector>
#include <array> // TOOD: Use it more if the vector size is set.
#include <set> // TODO: Needed?

#include "utils/assert.hpp"
#include "utils/log.h"
#include "utils/profile.hpp"
