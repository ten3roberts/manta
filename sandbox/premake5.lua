workspace "sandbox"

-- Includes manta from source
-- manta is one dir up
include ("../")

project "sandbox"
	kind		"ConsoleApp"
	language	"C"
	targetdir	"../bin/%{cfg.buildcfg}"
	objdir		"../obj"

	-- Add C and H files
	files { "src/**.c", "src/**.h" }

	-- Add shaders to compilation
	files { "../assets/shaders/**.vert", "../assets/shaders/**.frag" }	

	-- Link manta and GLFW
	-- Vulkan is linked by manta
	links { "manta", "GLFW" }
