workspace "crescent"
    -- Only x64 architecture is supported
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "crescent"

-- variable is required for glfw's premake5 file
outputdir = ""

-- includes the glfw project for building
include "vendor/glfw"

project "crescent"
    kind "ConsoleApp"

    language "C"
    targetdir "bin"

	-- sets the compiler include directories for the source files and glfw
    includedirs { "src" }
    includedirs { "vendor/", "vendor/glfw/include" }

    files {"src/**.h", "src/**.c"}
    
	-- links glfw with crescent
    links {"GLFW"}

	-- specifies configuration specific options
    filter "configurations:Debug"
      defines { "DEBUG=1", "MP_REPLACE_STD" }
      symbols "On"

   filter "configurations:Release"
      defines { "RELEASE=1" }
	  optimize "On"
	  symbols "On"

	-- sets platform specific includes
    filter "system:linux"
    --linkoptions {"-lvulkan"}
        defines { "PL_LINUX=1"}
        buildoptions { "-Wall", "-Werror", "-Wno-unused-but-set-variable", "-Wno-unused-variable", "-Wno-unused-function", "-pthread" }

        links { "m", "dl", "pthread", "X11", "GL", "vulkan" }
        prebuildcommands "cd assets/shaders && ./compile.sh"
	
	-- Specifies Windows and MSVC specific options and preprocessor definitions
	filter "system:windows"
        defines {"PL_WINDOWS=1"}
        systemversion "latest"
		links "opengl32.lib"
		-- Vulkan
		includedirs "C:/VulkanSDK/1.1.126.0/Include"
        links 		"C:/VulkanSDK/1.1.126.0/Lib/vulkan-1.lib"
        prebuildcommands "cd assets/shaders && compile.bat"

    
