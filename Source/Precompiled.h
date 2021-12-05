#pragma once

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <TextEditor.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <spirv_reflect.h>

#include <chrono>
#include <vector>

#include "Utility/Assert.hpp"
#include "Utility/Log.h"
#include "Utility/FilePath.h"
