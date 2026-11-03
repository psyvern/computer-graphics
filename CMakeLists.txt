cmake_minimum_required(VERSION 4.0)
project(CG)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
include_directories(${CMAKE_SOURCE_DIR}/../stb)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Please select another Build Directory")
endif()

add_executable(main main.cpp)
target_link_libraries(main
	OpenGL::GL
	GLEW
	glfw
	glm::glm
	assimp
)
