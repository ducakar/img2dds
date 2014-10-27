cmake_minimum_required(VERSION 2.8)

project(img2dds)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wconversion")

find_library(FREEIMAGE_LIBRARY NAMES freeimage FreeImage)
find_library(SQUISH_LIBRARY NAMES squish)

add_executable(img2dds main.cc ImageBuilder.hh ImageBuilder.cc)
target_link_libraries(img2dds ${FREEIMAGE_LIBRARY} ${SQUISH_LIBRARY})

install(TARGETS img2dds RUNTIME DESTINATION bin)
