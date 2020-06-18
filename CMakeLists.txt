cmake_minimum_required(VERSION 3.1)

project(manta C)

# Check if cmake was run standalone or as subdirectory
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    set(MANTA_STANDALONE TRUE)
endif()

option(MANTA_BUILD_PROJECTS "Build the sandbox program" ${MANTA_STANDALONE})

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
	message("No build type specified, using debug")
	set(CMAKE_BUILD_TYPE "Debug")
endif()

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})

# Find all c and h files, recursively
file(GLOB_RECURSE SOURCES src/*c src/*.h)

# Build the manta library from the sources
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Locate and bind to vulkan and glfw
find_package(Vulkan REQUIRED FATAL_ERROR)
target_link_libraries(${PROJECT_NAME} vulkan)
add_subdirectory(vendor/glfw)

target_link_libraries(${PROJECT_NAME} glfw)

# Set include directories for PRIVATE and API use
target_include_directories(${PROJECT_NAME} PRIVATE src)

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PUBLIC vendor)
target_include_directories(${PROJECT_NAME} PUBLIC vendor/headerlibs)

# Enable warnings for different compilers
if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
	target_compile_definitions(${PROJECT_NAME} PUBLIC PL_WINDOWS=1)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wno-unused-function -Werror)
	target_compile_definitions(${PROJECT_NAME} PUBLIC PL_LINUX=1)
endif()

# Build sandbox application if manta cmake was run standalone, I.e; not with add_subdirectory()
if(MANTA_BUILD_PROJECTS)
	add_subdirectory(sandbox)
endif()