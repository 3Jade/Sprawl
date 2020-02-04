#!/usr/bin/python

import subprocess
import os
import time
import platform
import glob
import shutil

import csbuild
from csbuild import log

csbuild.ToolchainGroup("gnu").AddCompilerCxxFlags(
	"-std=c++17",
	"-Wall",
	"-Wextra",
	"-Wctor-dtor-privacy",
	"-Woverloaded-virtual",
	"-Winit-self",
	"-Wmissing-include-dirs",
	"-Wswitch-default", 
	"-Wno-switch-enum", 
	"-Wundef", 
	"-Wno-old-style-cast"
)

#csbuild.AddOption("--with-mongo", action="store", help="Path to mongo include directory. If not specified, mongo will not be built.", nargs="?", default=None, const="/usr")
#csbuild.AddOption("--with-boost", action="store", help="Path to boost include directory. If not specified, mongo will not be built.", nargs="?", default=None, const="/usr")
#csbuild.AddOption("--no-threads", action="store_true", help="Build without thread support")
#csbuild.AddOption("--no-exceptions", action="store_true", help="Build without exception support")
#csbuild.AddOption("--no-unit-tests", action="store_true", help="Don't automatically run unit tests as part of build")
#csbuild.SetHeaderInstallSubdirectory("sprawl/{name}")

csbuild.SetUserData("subdir", platform.system())

if platform.system() == "Darwin":
	csbuild.ToolchainGroup("gnu").AddCompilerCxxFlags(
		"-D_XOPEN_SOURCE",
		"-stdlib=libc++"
	)
	csbuild.Toolchain("gcc").AddDefines("_XOPEN_SOURCE");
	csbuild.Toolchain("gcc").SetCppStandardLibrary("libc++")

csbuild.SetOutputDirectory("lib/{userData.subdir}/{toolchainName}/{architectureName}/{targetName}")
csbuild.SetIntermediateDirectory("Intermediate/{userData.subdir}/{toolchainName}/{architectureName}/{targetName}/{name}")


csbuild.Toolchain("msvc").AddCompilerCxxFlags(
	"/fp:fast",
	"/wd\"4530\"",
	"/wd\"4067\"",
	"/wd\"4351\"",
	"/constexpr:steps1000000",
    "/std:c++17",
    "/Zc:__cplusplus"
)

#if not csbuild.GetOption("no_threads"):
csbuild.ToolchainGroup("gnu").AddCompilerFlags("-pthread")

#if csbuild.GetOption("no_exceptions"):
#	csbuild.Toolchain("gcc", "ios", "android").AddCompilerCxxFlags("-fno-exceptions")
#else:
csbuild.Toolchain("msvc").AddCompilerCxxFlags("/EHsc")

	
with csbuild.Project("collections", "collections"):
	csbuild.SetOutput("libsprawl_collections", csbuild.ProjectType.StaticLibrary)


	
with csbuild.Project("tag", "tag"):
	csbuild.SetOutput("libsprawl_tag", csbuild.ProjectType.StaticLibrary)

with csbuild.Project("if", "if"):
	csbuild.SetOutput("libsprawl_if", csbuild.ProjectType.StaticLibrary)

with csbuild.Project("network", "network"):
	csbuild.SetOutput("libsprawl_network", csbuild.ProjectType.StaticLibrary)
	
with csbuild.Project("serialization", "serialization"):
	csbuild.SetOutput("libsprawl_serialization", csbuild.ProjectType.StaticLibrary)
	csbuild.AddExcludeDirectories("serialization/mongo")
	
with csbuild.Project("time", "time"):
	csbuild.SetOutput("libsprawl_time", csbuild.ProjectType.StaticLibrary)

	csbuild.ToolchainGroup("gnu").AddExcludeFiles("time/*_windows.cpp")
	if platform.system() == "Darwin":
		csbuild.ToolchainGroup("gnu").AddExcludeFiles("time/*_linux.cpp")
	else:
		csbuild.ToolchainGroup("gnu").AddExcludeFiles("time/*_osx.cpp")

	csbuild.Toolchain("msvc").AddExcludeFiles("time/*_linux.cpp", "time/*_osx.cpp")

	
with csbuild.Project("filesystem", "filesystem"):
	csbuild.SetOutput("libsprawl_filesystem", csbuild.ProjectType.StaticLibrary)

	csbuild.ToolchainGroup("gnu").AddExcludeFiles("filesystem/*_windows.cpp")
	csbuild.Toolchain("msvc").AddExcludeFiles("filesystem/*_linux.cpp")

	
with csbuild.Project("threading", "threading"):
	csbuild.SetOutput("libsprawl_threading", csbuild.ProjectType.StaticLibrary)

	if platform.system() != "Darwin":
		with csbuild.Scope(csbuild.ScopeDef.Final):
			csbuild.ToolchainGroup("gnu").AddLinkerFlags("-pthread")

	csbuild.ToolchainGroup("gnu").AddExcludeFiles("threading/*_windows.cpp")
	if platform.system() == "Darwin":
		csbuild.ToolchainGroup("gnu").AddExcludeFiles("threading/event_linux.cpp")
	else:
		csbuild.ToolchainGroup("gnu").AddExcludeFiles("threading/event_osx.cpp")

	csbuild.Toolchain("msvc").AddExcludeFiles(
		"threading/*_linux.cpp",
		"threading/*_osx.cpp"
	)

#MongoDir = csbuild.GetOption("with_mongo")
#BoostDir = csbuild.GetOption("with_boost")
#if (not MongoDir) ^ (not BoostDir):
#	log.LOG_ERROR("Both mongo and boost directories must be specified to build MongoSerializer.");
#	csbuild.Exit(1)
#	
#if MongoDir and BoostDir:
#	MongoDir = os.path.abspath(MongoDir)
#	BoostDir = os.path.abspath(BoostDir)
#	@csbuild.project("serialization-mongo", "serialization/mongo")
#	def serialization():
#		csbuild.SetOutput("libsprawl_serialization-mongo", csbuild.ProjectType.StaticLibrary)
#		csbuild.AddDefines("BOOST_ALL_NO_LIB")
#		
#		csbuild.AddIncludeDirectories(
#			"./serialization",
#			os.path.join(MongoDir, "include"),
#			os.path.join(BoostDir, "include")
#		)
#
#		csbuild.AddLibraryDirectories(
#			os.path.join(MongoDir, "lib"),
#			os.path.join(BoostDir, "lib")
#		)
#	
#		csbuild.SetHeaderInstallSubdirectory("sprawl/serialization")
#		csbuild.EnableOutputInstall()
#		csbuild.EnableHeaderInstall()

with csbuild.Project("memory", "memory"):
	csbuild.SetOutput("libsprawl_memory", csbuild.ProjectType.StaticLibrary)

with csbuild.Project("string", "string"):
	csbuild.SetOutput("libsprawl_string", csbuild.ProjectType.StaticLibrary)

with csbuild.Project("hash", "hash"):
	csbuild.SetOutput("libsprawl_hash", csbuild.ProjectType.StaticLibrary)

with csbuild.Project("logging", "logging"):
	csbuild.SetOutput("libsprawl_logging", csbuild.ProjectType.StaticLibrary)

	with csbuild.Scope(csbuild.ScopeDef.Final):
		if platform.system() != "Darwin":
			csbuild.ToolchainGroup("gnu").AddLibraries(
				"bfd",
			)
		csbuild.Toolchain("msvc").AddLibraries(
			"DbgHelp"
		)

	csbuild.ToolchainGroup("gnu").AddExcludeFiles("logging/*_windows.cpp")
	if platform.system() == "Darwin":
		csbuild.ToolchainGroup("gnu").AddExcludeFiles("logging/*_linux.cpp")
	else:
		csbuild.ToolchainGroup("gnu").AddExcludeFiles("logging/*_osx.cpp")
	csbuild.Toolchain("msvc").AddExcludeFiles(
		"logging/*_linux.cpp",
		"logging/*_osx.cpp"
	)

with csbuild.Project("common", "common"):
	csbuild.SetOutput("libsprawl_common", csbuild.ProjectType.StaticLibrary)


UnitTestDepends = ["serialization", "string", "hash", "time", "threading", "filesystem", "logging"]
#if MongoDir:
#	UnitTestDepends.append("serialization-mongo")

with csbuild.Project("UnitTests", "UnitTests", UnitTestDepends):
	#csbuild.DisableChunkedBuild()
	csbuild.SetOutput("SprawlUnitTest")
	csbuild.SetOutputDirectory("bin/{userData.subdir}/{toolchainName}/{architectureName}/{targetName}")
	
	#csbuild.EnableOutputInstall()
	
	csbuild.AddIncludeDirectories(
		"UnitTests/gtest",
		"UnitTests/gtest/include",
	)

	csbuild.ToolchainGroup("gnu").AddCompilerCxxFlags(
		"-Wno-undef", 
		"-Wno-switch-enum", 
		"-Wno-missing-field-initializers"
	)

	csbuild.AddExcludeFiles(
		"UnitTests/gtest/src/gtest-death-test.cc",
		"UnitTests/gtest/src/gtest-filepath.cc",
		"UnitTests/gtest/src/gtest-internal-inl.h",
		"UnitTests/gtest/src/gtest-port.cc",
		"UnitTests/gtest/src/gtest-printers.cc",
		"UnitTests/gtest/src/gtest-test-part.cc",
		"UnitTests/gtest/src/gtest-typed-test.cc",
		"UnitTests/gtest/src/gtest.cc",
	)

	#if MongoDir:
	#	csbuild.AddIncludeDirectories(
	#		"./serialization",
	#		os.path.join(MongoDir, "include"),
	#		os.path.join(BoostDir, "include")
	#	)
	#
	#	csbuild.AddLibraryDirectories(
	#		os.path.join(MongoDir, "lib"),
	#		os.path.join(BoostDir, "lib")
	#	)
	#	
	#	csbuild.AddLibraries(
	#		"mongoclient",
	#		"boost_filesystem",
	#		"boost_system",
	#		"boost_thread",
	#		"boost_program_options",
	#		"ssl",
	#		"crypto",
	#	)
	#	csbuild.Toolchain("gcc").AddLibraries("pthread")
	#	csbuild.Toolchain("gcc").AddCompilerFlags("-pthread")
	#	csbuild.AddDefines("WITH_MONGO")
	#else:
	csbuild.AddExcludeFiles(
		"UnitTests/UnitTests_MongoReplicable.cpp",
	)
		
		
with csbuild.Project("QueueTests", "QueueTests", ["time", "threading"]):
	#csbuild.DisableChunkedBuild()
	csbuild.SetOutput("QueueTests")
	csbuild.SetOutputDirectory("bin/{userData.subdir}/{toolchainName}/{architectureName}/{targetName}")
	
	#csbuild.EnableOutputInstall()
	
	csbuild.ToolchainGroup("gnu").AddCompilerCxxFlags(
		"-Wno-undef", 
		"-Wno-switch-enum", 
		"-Wno-missing-field-initializers"
	)

	csbuild.AddIncludeDirectories("QueueTests/ext/include")
	csbuild.AddLibraryDirectories("QueueTests/ext/lib/{userData.subdir}-{architectureName}")
	csbuild.AddExcludeDirectories("QueueTests/ext")
	csbuild.AddLibraries("tbb")
	
	if platform.system() == "Windows":
		@csbuild.OnBuildFinished
		def postMake(projects):
			for project in projects:
				for f in glob.glob("QueueTests/ext/lib/{project.userData.subdir}-{project.architectureName}/*".format(project=project)):
					basename = os.path.basename(f)
					dest = os.path.join(project.outputDir, basename)
					if not os.path.exists(dest):
						print("Copying {} to {}".format(f, dest))
						shutil.copyfile(f, dest)