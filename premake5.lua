workspace "crescent"
    configurations { "Debug", "Release" }
project "crescent"
    kind "ConsoleApp"
    language "C"
    targetdir "bin"
    files {"src/**.h", "src/**.c"}

    filter "configurations:Debug"
      defines { "DEBUG=1", "RELEASE=0" }
      symbols "On"

   filter "configurations:Release"
      defines { "DEBUG=0", "RELEASE=1" }
      optimize "On"

    filter "system:linux"
		--linkoptions {"-lvulkan"}
        defines { "PL_LINUX=1", "PL_WINDOWS=0"}
        linkoptions { "-lm" }
	
	-- Specifies Windows and MSVC specific options and preprocessor definitions
	filter "system:windows"
		defines { "PL_LINUX=0", "PL_WINDOWS=1"}
