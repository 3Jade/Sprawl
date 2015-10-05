#!/usr/bin/python

import subprocess
import os
import time
import platform

import csbuild
from csbuild import log

csbuild.Toolchain("gcc").Compiler().SetCppStandard("c++11")
csbuild.Toolchain("gcc").SetCxxCommand("clang++")

csbuild.Toolchain("gcc").Compiler().AddWarnFlags("all", "extra", "ctor-dtor-privacy", "overloaded-virtual", "init-self", "missing-include-dirs", "switch-default", "no-switch-enum", "undef", "no-old-style-cast")

csbuild.DisablePrecompile()

csbuild.AddOption("--with-mongo", action="store", help="Path to mongo include directory. If not specified, mongo will not be built.", nargs="?", default=None, const="/usr")
csbuild.AddOption("--with-boost", action="store", help="Path to boost include directory. If not specified, mongo will not be built.", nargs="?", default=None, const="/usr")
csbuild.AddOption("--no-threads", action="store_true", help="Build without thread support")
csbuild.AddOption("--no-unit-tests", action="store_true", help="Don't automatically run unit tests as part of build")
csbuild.SetHeaderInstallSubdirectory("sprawl/{project.name}")

csbuild.SetUserData("subdir", platform.system())

if platform.system() == "Darwin":
	csbuild.Toolchain("gcc").AddDefines("_XOPEN_SOURCE");
	csbuild.Toolchain("gcc").SetCppStandardLibrary("libc++")

csbuild.SetOutputDirectory("lib/{project.userData.subdir}/{project.activeToolchainName}/{project.outputArchitecture}/{project.targetName}")
csbuild.SetIntermediateDirectory("Intermediate/{project.userData.subdir}/{project.activeToolchainName}/{project.outputArchitecture}/{project.targetName}/{project.name}")


csbuild.Toolchain("msvc").AddCompilerFlags(
	"/fp:fast",
	"/wd\"4530\"",
	"/wd\"4067\"",
	"/wd\"4351\""
)

if not csbuild.GetOption("no_threads"):
	csbuild.Toolchain("gcc", "ios", "android").AddCompilerFlags("-pthread")

@csbuild.target("debug")
def debug():
	csbuild.Toolchain("msvc").AddCompilerFlags(
		"/EHsc"
	)

@csbuild.project("collections", "collections")
def collections():
	csbuild.SetOutput("libsprawl_collections", csbuild.ProjectType.StaticLibrary)

	csbuild.EnableHeaderInstall()


@csbuild.project("network", "network")
def network():
	csbuild.SetOutput("libsprawl_network", csbuild.ProjectType.StaticLibrary)
	
	csbuild.EnableOutputInstall()
	csbuild.EnableHeaderInstall()

	
@csbuild.project("serialization", "serialization")
def serialization():
	csbuild.SetOutput("libsprawl_serialization", csbuild.ProjectType.StaticLibrary)
	csbuild.AddExcludeDirectories("serialization/mongo")

	csbuild.EnableOutputInstall()
	csbuild.EnableHeaderInstall()

	
@csbuild.project("time", "time")
def timeProject():
	csbuild.SetOutput("libsprawl_time", csbuild.ProjectType.StaticLibrary)

	csbuild.Toolchain("gcc").AddExcludeFiles("time/*_windows.cpp")
	if platform.system() == "Darwin":
		csbuild.Toolchain("gcc").AddExcludeFiles("time/*_linux.cpp")
	else:
		csbuild.Toolchain("gcc").AddExcludeFiles("time/*_osx.cpp")

	csbuild.Toolchain("msvc").AddExcludeFiles("time/*_linux.cpp", "time/*_osx.cpp")

	csbuild.EnableOutputInstall()
	csbuild.EnableHeaderInstall()

	
@csbuild.project("filesystem", "filesystem")
def filesystem():
	csbuild.SetOutput("libsprawl_filesystem", csbuild.ProjectType.StaticLibrary)

	csbuild.Toolchain("gcc").AddExcludeFiles("filesystem/*_windows.cpp")
	csbuild.Toolchain("msvc").AddExcludeFiles("filesystem/*_linux.cpp")

	csbuild.EnableOutputInstall()
	csbuild.EnableHeaderInstall()

	
@csbuild.project("threading", "threading")
def threading():
	csbuild.SetOutput("libsprawl_threading", csbuild.ProjectType.StaticLibrary)

	if platform.system() != "Darwin":
		@csbuild.scope(csbuild.ScopeDef.Final)
		def finalScope():
			csbuild.Toolchain("gcc").Linker().AddLinkerFlags("-pthread")

	csbuild.Toolchain("gcc").AddExcludeFiles("threading/*_windows.cpp")
	if platform.system() == "Darwin":
		csbuild.Toolchain("gcc").AddExcludeFiles("threading/event_linux.cpp")
	else:
		csbuild.Toolchain("gcc").AddExcludeFiles("threading/event_osx.cpp")

	csbuild.Toolchain("msvc").AddExcludeFiles(
		"threading/*_linux.cpp",
		"threading/*_osx.cpp"
	)

	csbuild.EnableOutputInstall()
	csbuild.EnableHeaderInstall()

MongoDir = csbuild.GetOption("with_mongo")
BoostDir = csbuild.GetOption("with_boost")
if (not MongoDir) ^ (not BoostDir):
	log.LOG_ERROR("Both mongo and boost directories must be specified to build MongoSerializer.");
	csbuild.Exit(1)
	
if MongoDir and BoostDir:
	MongoDir = os.path.abspath(MongoDir)
	BoostDir = os.path.abspath(BoostDir)
	@csbuild.project("serialization-mongo", "serialization/mongo")
	def serialization():
		csbuild.SetOutput("libsprawl_serialization-mongo", csbuild.ProjectType.StaticLibrary)
		csbuild.AddDefines("BOOST_ALL_NO_LIB")
		
		csbuild.AddIncludeDirectories(
			"./serialization",
			os.path.join(MongoDir, "include"),
			os.path.join(BoostDir, "include")
		)

		csbuild.AddLibraryDirectories(
			os.path.join(MongoDir, "lib"),
			os.path.join(BoostDir, "lib")
		)
	
		csbuild.SetHeaderInstallSubdirectory("sprawl/serialization")
		csbuild.EnableOutputInstall()
		csbuild.EnableHeaderInstall()

@csbuild.project("memory", "memory")
def memory():
	csbuild.SetOutput("libsprawl_memory", csbuild.ProjectType.StaticLibrary)

	csbuild.EnableHeaderInstall()


@csbuild.project("string", "string")
def string():
	csbuild.SetOutput("libsprawl_string", csbuild.ProjectType.StaticLibrary)

	csbuild.EnableOutputInstall()
	csbuild.EnableHeaderInstall()


@csbuild.project("hash", "hash")
def hash():
	csbuild.SetOutput("libsprawl_hash", csbuild.ProjectType.StaticLibrary)

	csbuild.EnableOutputInstall()
	csbuild.EnableHeaderInstall()

@csbuild.project("logging", "logging")
def logging():
	csbuild.SetOutput("libsprawl_logging", csbuild.ProjectType.StaticLibrary)

	@csbuild.scope(csbuild.ScopeDef.Final)
	def finalScope():
		if platform.system() != "Darwin":
			csbuild.Toolchain("gcc").AddLibraries(
				"bfd",
				"unwind",
				"unwind-x86_64",
			)
		csbuild.Toolchain("msvc").AddLibraries(
			"DbgHelp"
		)

	csbuild.Toolchain("gcc").AddExcludeFiles("logging/*_windows.cpp")
	if platform.system() == "Darwin":
		csbuild.Toolchain("gcc").AddExcludeFiles("logging/*_linux.cpp")
	else:
		csbuild.Toolchain("gcc").AddExcludeFiles("logging/*_osx.cpp")
	csbuild.Toolchain("msvc").AddExcludeFiles(
		"logging/*_linux.cpp",
		"logging/*_osx.cpp"
	)

	csbuild.EnableOutputInstall()
	csbuild.EnableHeaderInstall()

@csbuild.project("common", "common")
def common():
	csbuild.SetOutput("libsprawl_common", csbuild.ProjectType.StaticLibrary)

	csbuild.EnableHeaderInstall()

UnitTestDepends = ["serialization", "string", "hash", "time", "threading", "filesystem", "logging"]
if MongoDir:
	UnitTestDepends.append("serialization-mongo")

@csbuild.project("UnitTests", "UnitTests", UnitTestDepends)
def UnitTests():
	csbuild.DisableChunkedBuild()
	csbuild.SetOutput("SprawlUnitTest")
	csbuild.SetOutputDirectory("bin/{project.userData.subdir}/{project.activeToolchainName}/{project.outputArchitecture}/{project.targetName}")
	
	csbuild.EnableOutputInstall()
	
	csbuild.AddIncludeDirectories(
		"UnitTests/gtest",
		"UnitTests/gtest/include",
	)

	csbuild.Toolchain("gcc").Compiler().AddWarnFlags("no-undef", "no-switch-enum", "no-missing-field-initializers")

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

	if MongoDir:
		csbuild.AddIncludeDirectories(
			"./serialization",
			os.path.join(MongoDir, "include"),
			os.path.join(BoostDir, "include")
		)

		csbuild.AddLibraryDirectories(
			os.path.join(MongoDir, "lib"),
			os.path.join(BoostDir, "lib")
		)
		
		csbuild.AddLibraries(
			"mongoclient",
			"boost_filesystem",
			"boost_system",
			"boost_thread",
			"boost_program_options",
			"ssl",
			"crypto",
		)
		csbuild.Toolchain("gcc").AddLibraries("pthread")
		csbuild.Toolchain("gcc").AddCompilerFlags("-pthread")
		csbuild.AddDefines("WITH_MONGO")
	else:
		csbuild.AddExcludeFiles(
			"UnitTests/UnitTests_MongoReplicable.cpp",
		)

	if not csbuild.GetOption("no_unit_tests"):
		@csbuild.postMakeStep
		def postMake(project):
			unitTestExe = "bin/{project.userData.subdir}/{project.activeToolchainName}/{project.outputArchitecture}/{project.targetName}/SprawlUnitTest".format(project=project)
			if platform.system() == "Windows":
				time.sleep(2)
			else:
				while not os.access(unitTestExe, os.X_OK):
					time.sleep(1)

			log.LOG_BUILD("Running unit tests...")
			ret = subprocess.call([unitTestExe, "--all"]);
			if ret != 0:
				log.LOG_ERROR("Unit tests failed! Errors reported: {}".format(ret));
