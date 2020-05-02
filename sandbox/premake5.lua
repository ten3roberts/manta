workspace "sandbox"

-- Includes cuttle from source
-- cuttle is one dir up
include ("../")

project "sandbox"
	kind		"ConsoleApp"
	language	"C"
	targetdir	"../bin"
	objdir		"../obj"

	-- Add C and H files
	files { "src/**.c", "src/**.h" }

	-- Add shaders to compilation
	files { "../assets/shaders/**.vert", "../assets/shaders/**.frag" }	

	-- Link cuttle and GLFW
	-- Vulkan is linked by cuttle
	links { "cuttle", "GLFW" }
