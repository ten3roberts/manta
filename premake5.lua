-- variable is required for glfw's premake5 file
outputdir = ""
include ("vendor/glfw")

libraries = {};

-- Inserts one or more libraries into the libraries table
function addlib(libname)
	if type(libname) == "table" then
		for k, v in pairs(libname) do
			table.insert(libraries, v)
			printf("Adding library '%s'", v)
		end
	elseif type(libname) == "string" then
		table.insert(libraries, libname)
		printf("Adding library '%s'", v)

	else
		premake.error("Invalid library type: ", libname)
end
end

project "*"
	-- Only x64 architecture is supported
	architecture "x64"
	configurations { "Debug", "Release" }

	-- sets the compiler include directories for the source files and glfw
	includedirs { "src",  "vendor", "vendor/glfw/include", "vendor/headerlibs"  }

	-- Unix and Linux links and configurations
	filter "system:linux or bsd or hurd or aix or solaris or haiku"
		defines { "PL_LINUX=1"}
		buildoptions { "-Wall", "-Wno-unused-function", "-pthread" }
		links { "m", "dl", "pthread", "X11", "GL", "vulkan" }

	-- Specifies Windows and MSVC specific options and preprocessor definitions
	filter "system:windows"
		defines {"PL_WINDOWS=1"}
		systemversion "latest"
		links "opengl32.lib"
		-- Vulkan
		includedirs "C:/VulkanSDK/1.1.126.0/Include"
		links ("C:/VulkanSDK/1.1.126.0/Lib/vulkan-1.lib")
		prebuildcommands "cd assets/shaders && compile.bat"

	-- specifies configuration specific options
	filter "configurations:Debug"
		defines { "DEBUG=1", "MP_REPLACE_STD" }
		optimize "Off"
    	symbols "On"

   filter "configurations:Release"
    	defines { "RELEASE=1" }
		optimize "On"
		symbols "On"


project "crescent"
    kind "StaticLib"

    language "C"
    targetdir "bin"
	files {"src/**.h", "src/**.c"}
	includedirs { "src", "vendor", "vendor/glfw/include", "vendor/headerlibs"  }
	


    

