cmake_minimum_required(VERSION 3.10)
project(CohenSutherlandLineClip2D)
set(CMAKE_CXX_STANDARD 11)

# Add executable
add_executable(CohenSutherlandLineClip2D
        ch8CohenSutherlandLineClip2D.cpp
        mmn13.cpp
        ch8CohenSutherlandLineClip2D.h
)

# Use the native macOS frameworks
target_link_libraries(CohenSutherlandLineClip2D
        "-framework OpenGL"
        "-framework GLUT"
)