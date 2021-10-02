cmake_minimum_required(VERSION 3.12)

project(foton CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Bootstrap
if(WIN32)
	set(PYTHON_EXECUTABLE "python")
else()
	set(PYTHON_EXECUTABLE "python3")
endif()

execute_process(
	COMMAND ${PYTHON_EXECUTABLE} "bootstrap.py"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
	RESULT_VARIABLE BOOTSTRAP_RESULT
)

if (NOT (${BOOTSTRAP_RESULT} EQUAL 0))
	message(FATAL_ERROR "Make sure that Python, CMake and Git are installed properly.")
endif()

# Project
file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.c??)
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.h)

include_directories(src)
add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEADER_FILES})

macro(SETUP_GROUPS src_files)
	foreach(FILE ${src_files})
		get_filename_component(PARENT_DIR "${FILE}" PATH)

		# skip src or include and changes /'s to \\'s
		set(GROUP "${PARENT_DIR}")
		string(REPLACE "/" "\\" GROUP "${GROUP}")

		source_group("${GROUP}" FILES "${FILE}")
	endforeach()
endmacro()

SETUP_GROUPS("${SRC_FILES}")
SETUP_GROUPS("${HEADER_FILES}")

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

if(MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT foton)
endif()

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries (${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_DIR 3rdparty/src/glfw)

add_subdirectory(${GLFW_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${GLFW_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
set_property(TARGET glfw PROPERTY FOLDER "3rdParty")

# GLM
set(GLM_DIR 3rdparty/src/glm)
target_include_directories(${PROJECT_NAME} PRIVATE ${GLM_DIR})

# STB
set(STB_DIR 3rdparty/src/stb)
target_include_directories(${PROJECT_NAME} PRIVATE ${STB_DIR})

# Dear ImGui
set(IMGUI_DIR 3rdparty/src/imgui)
set(IMGUI_TEXTEDIT_DIR 3rdparty/src/ImGuiColorTextEdit)
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
set_property(TARGET imgui PROPERTY FOLDER "3rdParty")
