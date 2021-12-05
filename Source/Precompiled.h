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

#define FT_BEGIN_NAMESPACE namespace FT {
#define FT_END_NAMESPACE }
#define FT_DEBUG !NDEBUG

#define FT_DEFAULT_WINDOW_WIDTH 1280
#define FT_DEFAULT_WINDOW_HEIGHT 720

#define FT_APPLICATION_NAME "Foton"

#define FT_DELETE_COPY_AND_MOVE(type) \
	type(type const&) = delete; \
	type& operator=(type const&) = delete; \
	type(type&&) = delete; \
	type& operator=(type&&) = delete;

#define FT_FLAG_TYPE_SETUP(flagType) \
	inline flagType operator&(flagType a, flagType b) \
	{ \
		return static_cast<flagType>(static_cast<std::underlying_type<flagType>::type>(a) & \
			static_cast<std::underlying_type<flagType>::type>(b) ); \
	} \
	\
	inline flagType operator|(flagType a, flagType b) \
	{ \
		return static_cast<flagType>( static_cast<std::underlying_type<flagType>::type>(a) | \
			static_cast<std::underlying_type<flagType>::type>(b)); \
	} \
	\
	inline bool IsFlagSet(flagType x) \
	{ \
		return (static_cast<uint32_t>(x) != 0); \
	}

#include "Utility/Assert.hpp"
#include "Utility/Log.h"
#include "Utility/FilePath.h"
