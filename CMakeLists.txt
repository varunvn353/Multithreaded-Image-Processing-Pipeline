cmake_minimum_required(VERSION 3.16)
project(ImageProcessingPipeline VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(image_pipeline
    src/main.cpp
    src/pipeline.cpp
    src/profiler.cpp
    src/stages/preprocess.cpp
    src/stages/edge_detection.cpp
    src/stages/object_detection.cpp
)

target_include_directories(image_pipeline PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(image_pipeline PRIVATE ${OpenCV_LIBS})

if(MSVC)
    target_compile_options(image_pipeline PRIVATE /W4)
else()
    target_compile_options(image_pipeline PRIVATE -Wall -Wextra -Wpedantic)
endif()
