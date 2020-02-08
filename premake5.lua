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
    includedirs { "src", "vendor/glfw/include" }
    includedirs { "src", "vendor/" }

    files {"src/**.h", "src/**.c"}
    

	-- links glfw with crescent
    links {"GLFW"}

	-- specifies configuration specific options
    filter "configurations:Debug"
      defines { "DEBUG=1", "RELEASE=0" }
      symbols "On"

   filter "configurations:Release"
      defines { "DEBUG=0", "RELEASE=1" }
      optimize "On"

	-- sets platform specific includes
    filter "system:linux"
    --linkoptions {"-lvulkan"}
        defines { "PL_LINUX=1"}
        buildoptions { "-Wall", "-Wno-unused-but-set-variable", "-Wno-unused-variable", "-Wno-unused-function", "-pthread" }

        links { "m", "dl", "pthread", "X11", "GL", "vulkan" }
        postbuildcommands "cd assets/shaders && ./compile.sh"
	
	-- Specifies Windows and MSVC specific options and preprocessor definitions
	filter "system:windows"
        defines {"PL_WINDOWS=1"}
        systemversion "latest"
		links "opengl32.lib"
		-- Vulkan
		includedirs "C:/VulkanSDK/1.1.126.0/Include"
        links 		"C:/VulkanSDK/1.1.126.0/Lib/vulkan-1.lib"
        postbuildcommands "cd assets/shaders && compile.bat"

    
