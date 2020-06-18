project(sandbox C)

# Find all c and h files, recursively and build them
file(GLOB_RECURSE SOURCES src/*c src/*.h)

add_executable(${PROJECT_NAME} ${SOURCES})

# Find and compile the shaders to SPIR-V
file(GLOB_RECURSE SHADERS ${CMAKE_BINARY_DIR}/assets/*.vert ${CMAKE_BINARY_DIR}/assets/*.frag)

foreach(shader IN LISTS SHADERS)
	message("Found shader ${shader}")
	add_custom_command(OUTPUT "${shader}.spv" COMMAND glslc "${shader}" -o "${shader}.spv" COMMENT "Compiling shader ${shader}" DEPENDS "${shader}")

	list(APPEND SHADER_OUTPUTS "${shader}.spv")
	# add_custom_command(TARGET ${PROJECT_NAME} COMMAND glslc "${shader}" -o "${shader}.spv" COMMENT "Compiling shader ${shader}" DEPENDS "${shader}")
endforeach()

# Add a fake shader_target for shader compilatio and add as dependency to sandbox
add_custom_target(shader_target DEPENDS ${SHADER_OUTPUTS})

add_dependencies(${PROJECT_NAME} shader_target)

target_link_libraries(${PROJECT_NAME} manta )