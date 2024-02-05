cmake_minimum_required(VERSION 3.27)
project(Chip_8_TFG C)

set(CMAKE_C_STANDARD 11)


find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)


include_directories(.)

add_executable(Chip_8_TFG
        main.c
        play.c
        play.c
)


target_link_libraries(Chip_8_TFG ${OPENGL_LIBRARIES})
target_link_libraries(Chip_8_TFG ${GLUT_LIBRARIES})
target_link_libraries(Chip_8_TFG -lopengl32 -g -lfreeglut)