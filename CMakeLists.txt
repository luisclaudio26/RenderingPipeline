cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
cmake_policy(SET CMP0015 NEW)
project(render)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -Wno-unused-result")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Headers
include_directories(include)
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(3rdparty)

#Source files
file(GLOB SOURCES "src/*.cpp"
                  "src/pipeline/*.cpp")

#Link libraries
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

set(LIBS nanogui glfw ${GLEW_LIBRARIES} ${GLFW_LIBRARIES} dl Xcursor X11 Xxf86vm Xinerama pthread Xrandr Xi GL ${OPENGL_LIBRARIES})

add_executable(render ${SOURCES})
target_link_libraries(render ${LIBS})
