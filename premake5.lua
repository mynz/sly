workspace "sly"
	configurations { "Debug", "Release" }

project "sly"
	kind "ConsoleApp"
	language "C++"
	targetdir "bin/${conf.buidlcfg}"
	files { "src/**.cpp", "src/**.h" }

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "on"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "on"
