cmake_minimum_required(VERSION 3.16)

project(Foton)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Bootstrap
if (WIN32)
	set(PYTHON_EXECUTABLE "python")
else()
	set(PYTHON_EXECUTABLE "python3")
endif()

execute_process(
	COMMAND ${PYTHON_EXECUTABLE} "bootstrap.py"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/External
	RESULT_VARIABLE BOOTSTRAP_RESULT
)

if (NOT (${BOOTSTRAP_RESULT} EQUAL 0))
	message(FATAL_ERROR "Make sure that Python, CMake and Git are installed properly.")
endif()

# Project
file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} Source/*.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} Source/*.h Source/*.hpp)

include_directories(Source)
add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEADER_FILES})

target_precompile_headers(${PROJECT_NAME} PRIVATE Source/Precompiled.h)

macro(SETUP_GROUPS INPUT_FILES)
	foreach(FILE ${INPUT_FILES})
		get_filename_component(PARENT_DIR "${FILE}" PATH)

		set(GROUP "${PARENT_DIR}")
		string(REPLACE "/" "\\" GROUP "${GROUP}")

		source_group("${GROUP}" FILES "${FILE}")
	endforeach()
endmacro()

SETUP_GROUPS("${SRC_FILES}")
SETUP_GROUPS("${HEADER_FILES}")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

if (MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

add_definitions(-DFT_ROOT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/\")

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries (${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_DIR External/src/glfw)

add_subdirectory(${GLFW_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${GLFW_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
set_property(TARGET glfw PROPERTY FOLDER "External/glfw")
set_property(TARGET update_mappings PROPERTY FOLDER "External/glfw")

# GLM
set(GLM_DIR External/src/glm)
target_include_directories(${PROJECT_NAME} PRIVATE ${GLM_DIR})

# GLSLANG
set(GLSLANG_DIR External/src/glslang)
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "")
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "")
set(ENABLE_CTEST OFF CACHE BOOL "")

add_subdirectory(${GLSLANG_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE ${GLSLANG_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${GLSLANG_DIR}/src)
target_include_directories(${PROJECT_NAME} PRIVATE ${GLSLANG_DIR}/glslang/Include)
target_include_directories(${PROJECT_NAME} PRIVATE ${GLSLANG_DIR}/StandAlone)
target_link_libraries(${PROJECT_NAME} PRIVATE glslang SPIRV)

set_property(TARGET glslang PROPERTY FOLDER "External/glslang")
set_property(TARGET OGLCompiler PROPERTY FOLDER "External/glslang")
set_property(TARGET OSDependent PROPERTY FOLDER "External/glslang")
set_property(TARGET SPIRV PROPERTY FOLDER "External/glslang")
set_property(TARGET GenericCodeGen PROPERTY FOLDER "External/glslang")
set_property(TARGET MachineIndependent PROPERTY FOLDER "External/glslang")
set_property(TARGET HLSL PROPERTY FOLDER "External/glslang")

# SPIRV-Reflect
set(SPIRV_REFLECT_DIR External/src/SPIRV-Reflect)
set(SPIRV_REFLECT_SOURCES ${SPIRV_REFLECT_DIR}/spirv_reflect.c)

add_library(SPIRV-Reflect STATIC ${SPIRV_REFLECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${SPIRV_REFLECT_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE SPIRV-Reflect)

set_property(TARGET SPIRV-Reflect PROPERTY FOLDER "External")

# STB
set(STB_DIR External/src/stb)
target_include_directories(${PROJECT_NAME} PRIVATE ${STB_DIR})

# ImGui
set(IMGUI_DIR External/src/imgui)
set(IMGUI_TEXTEDIT_DIR External/src/ImGuiColorTextEdit)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_TEXTEDIT_DIR}/TextEditor.cpp)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${IMGUI_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${IMGUI_TEXTEDIT_DIR})
target_include_directories(imgui PRIVATE ${IMGUI_DIR})
target_include_directories(imgui PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(imgui PRIVATE ${GLFW_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)

set_property(TARGET imgui PROPERTY FOLDER "External")

# NFD
set(NFD_DIR External/src/nativefiledialog-extended)

add_subdirectory(${NFD_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${NFD_DIR}/src/include)
target_link_libraries(${PROJECT_NAME} PRIVATE nfd)
set_property(TARGET nfd PROPERTY FOLDER "External")
