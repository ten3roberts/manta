workspace "crescent"
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

    files {"src/**.h", "src/**.c"}

	-- links glfw with crescent
    links "glfw"

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
        defines { "PL_LINUX=1", "PL_WINDOWS=0"}
        buildoptions { "-Wall" }
        linkoptions { "-lm" }
	
	-- Specifies Windows and MSVC specific options and preprocessor definitions
	filter "system:windows"
		defines { "PL_LINUX=0", "PL_WINDOWS=1"}
