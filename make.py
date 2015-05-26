#!/usr/bin/python

import subprocess
import os
import time
import platform

import csbuild
from csbuild import log
from csbuild.toolchain_msvc import VisualStudioPackage

csbuild.Toolchain("gcc").Compiler().SetCppStandard("c++11")
csbuild.Toolchain("gcc").SetCxxCommand("clang++")

csbuild.Toolchain("msvc").SetMsvcVersion(VisualStudioPackage.Vs2013)

csbuild.Toolchain("gcc").Compiler().AddWarnFlags("all", "extra", "ctor-dtor-privacy", "old-style-cast", "overloaded-virtual", "init-self", "missing-include-dirs", "switch-default", "switch-enum", "undef")

csbuild.DisablePrecompile()

csbuild.AddOption("--with-mongo", action="store", help="Path to mongo include directory. If not specified, mongo will not be built.", nargs="?", default=None, const="/usr")
csbuild.AddOption("--with-boost", action="store", help="Path to boost include directory. If not specified, mongo will not be built.", nargs="?", default=None, const="/usr")
csbuild.AddOption("--vs-ver", action="store", default="vs2012", help="Visual studio version", choices=["vs2012", "vs2013"])
csbuild.AddOption("--no-threads", action="store_true", help="Build without thread support")
csbuild.SetHeaderInstallSubdirectory("sprawl/{project.name}")

if platform.system() == "Windows":
	csbuild.SetUserData("subdir", csbuild.GetOption("vs_ver"))
else:
	csbuild.SetUserData("subdir", platform.system())

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
	csbuild.Toolchain("msvc").AddExcludeFiles("time/*_linux.cpp")

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

	@csbuild.scope(csbuild.ScopeDef.Final)
	def finalScope():
		csbuild.Toolchain("gcc").Linker().AddLinkerFlags("-pthread")
		csbuild.Toolchain("gcc").AddLibraries("pthread")

	csbuild.Toolchain("gcc").AddExcludeFiles("threading/*_windows.cpp")
	csbuild.Toolchain("msvc").AddExcludeFiles("threading/*_linux.cpp")

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


@csbuild.project("common", "common")
def common():
	csbuild.SetOutput("libsprawl_common", csbuild.ProjectType.StaticLibrary)

	csbuild.EnableHeaderInstall()

UnitTestDepends = ["serialization", "string", "hash", "time", "threading"]
if MongoDir:
	UnitTestDepends.append("serialization-mongo")

@csbuild.project("UnitTests", "UnitTests", UnitTestDepends)
def UnitTests():
	csbuild.SetOutput("SprawlUnitTest")
	csbuild.SetOutputDirectory("bin/{project.userData.subdir}/{project.activeToolchainName}/{project.outputArchitecture}/{project.targetName}")
	
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
