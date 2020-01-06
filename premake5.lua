workspace "crescent"
    configurations { "Debug", "Release" }

-- variable is required for glfw's premake5 file
outputdir = ""

include "vendor/glfw"

project "crescent"
    kind "ConsoleApp"
    language "C"
    targetdir "bin"

    includedirs { "src", "vendor/glfw/include" }

    files {"src/**.h", "src/**.c"}

    links "glfw"

    filter "configurations:Debug"
      defines { "DEBUG=1", "RELEASE=0" }
      symbols "On"

   filter "configurations:Release"
      defines { "DEBUG=0", "RELEASE=1" }
      optimize "On"

    filter "system:linux"
		--linkoptions {"-lvulkan"}
        defines { "PL_LINUX=1", "PL_WINDOWS=0"}
        buildoptions { "-Wall" }
        linkoptions { "-lm" }
	
	-- Specifies Windows and MSVC specific options and preprocessor definitions
	filter "system:windows"
		defines { "PL_LINUX=0", "PL_WINDOWS=1"}
