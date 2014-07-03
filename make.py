#!/usr/bin/python

import subprocess
import os
import time
import platform
import csbuild
from csbuild import log

csbuild.Toolchain("gcc").Compiler().CppStandard("c++11")
csbuild.Toolchain("gcc").CppCompiler("clang++")

csbuild.Toolchain("msvc").SetMsvcVersion(110)

csbuild.Toolchain("gcc").Compiler().WarnFlags("all", "extra", "ctor-dtor-privacy", "old-style-cast", "overloaded-virtual", "init-self", "missing-include-dirs", "switch-default", "switch-enum", "undef")

csbuild.NoPrecompile()

csbuild.OutDir("lib/{project.activeToolchainName}/{project.outputArchitecture}/{project.targetName}")
csbuild.ObjDir("Intermediate/{project.activeToolchainName}/{project.outputArchitecture}/{project.targetName}/{project.name}")

csbuild.add_option("--with-mongo", action="store", help="Path to mongo include directory. If not specified, mongo will not be built.", nargs="?", default=None, const="/usr")
csbuild.add_option("--with-boost", action="store", help="Path to boost include directory. If not specified, mongo will not be built.", nargs="?", default=None, const="/usr")

csbuild.InstallSubdir("sprawl/{project.name}")


csbuild.Toolchain("msvc").CompilerFlags(
	"/fp:fast",
	"/wd\"4530\"",
	"/wd\"4067\"",
	"/wd\"4351\""
)

@csbuild.project("collections", "collections")
def collections():
	csbuild.Output("libsprawl_collections", csbuild.ProjectType.StaticLibrary)

	csbuild.InstallHeaders()
	
@csbuild.project("network", "network")
def network():
	csbuild.Output("libsprawl_network", csbuild.ProjectType.StaticLibrary)
	
	csbuild.InstallOutput()
	csbuild.InstallHeaders()

	
@csbuild.project("serialization", "serialization")
def serialization():
	csbuild.Output("libsprawl_serialization", csbuild.ProjectType.StaticLibrary)
	csbuild.ExcludeDirs("serialization/mongo")

	csbuild.InstallOutput()
	csbuild.InstallHeaders()

MongoDir = csbuild.get_option("with_mongo")
BoostDir = csbuild.get_option("with_boost")
if (not MongoDir) ^ (not BoostDir):
	log.LOG_ERROR("Both mongo and boost directories must be specified to build MongoSerializer.");
	csbuild.Exit(1)
	
if MongoDir and BoostDir:
	MongoDir = os.path.abspath(MongoDir)
	BoostDir = os.path.abspath(BoostDir)
	@csbuild.project("serialization-mongo", "serialization/mongo")
	def serialization():
		csbuild.Output("libsprawl_serialization-mongo", csbuild.ProjectType.StaticLibrary)
		csbuild.Define("BOOST_ALL_NO_LIB")
		
		csbuild.IncludeDirs(
			"./serialization",
			os.path.join(MongoDir, "include"),
			os.path.join(BoostDir, "include")
		)

		csbuild.LibDirs(
			os.path.join(MongoDir, "lib"),
			os.path.join(BoostDir, "lib")
		)
	
		csbuild.InstallSubdir("sprawl/serialization")
		csbuild.InstallHeaders()
		csbuild.InstallOutput()

@csbuild.project("memory", "memory")
def memory():
	csbuild.Output("libsprawl_memory", csbuild.ProjectType.StaticLibrary)

	csbuild.InstallHeaders()


@csbuild.project("string", "string")
def string():
	csbuild.Output("libsprawl_string", csbuild.ProjectType.StaticLibrary)

	csbuild.InstallOutput()
	csbuild.InstallHeaders()


@csbuild.project("hash", "hash")
def string():
	csbuild.Output("libsprawl_hash", csbuild.ProjectType.StaticLibrary)

	csbuild.InstallOutput()
	csbuild.InstallHeaders()


@csbuild.project("common", "common")
def string():
	csbuild.Output("libsprawl_common", csbuild.ProjectType.StaticLibrary)

	csbuild.InstallHeaders()

UnitTestDepends = ["collections", "network", "serialization", "memory", "string", "hash", "common"]
if MongoDir:
	UnitTestDepends.append("serialization-mongo")

@csbuild.project("UnitTests", "UnitTests", UnitTestDepends)
def UnitTests():
	csbuild.Output("SprawlUnitTest")
	csbuild.OutDir("bin/{project.activeToolchainName}/{project.outputArchitecture}/{project.targetName}")

	csbuild.LibDirs( "lib/{project.activeToolchainName}/{project.outputArchitecture}/{project.targetName}" )
	
	csbuild.Libraries(
		# Header only "sprawl_collections",
		"sprawl_network",
		"sprawl_serialization",
		"sprawl_string",
		"sprawl_hash",
		# Header only "sprawl_common",
	)

	if MongoDir:
		csbuild.IncludeDirs(
			"./serialization",
			os.path.join(MongoDir, "include"),
			os.path.join(BoostDir, "include")
		)

		csbuild.LibDirs(
			os.path.join(MongoDir, "lib"),
			os.path.join(BoostDir, "lib")
		)

		csbuild.Libraries(
			"sprawl_serialization-mongo",
			"mongoclient",
			"boost_filesystem",
			"boost_system",
			"boost_thread",
			"boost_program_options",
			"ssl",
			"crypto",
		)
		csbuild.Toolchain("gcc").Libraries("pthread")
		csbuild.Toolchain("gcc").CompilerFlags("-pthread")
		csbuild.Define("WITH_MONGO")
	else:
		csbuild.ExcludeFiles(
			"UnitTests/UnitTests_MongoReplicable.cpp",
		)

	@csbuild.postMakeStep
	def postMake(project):
		unitTestExe = "bin/{project.activeToolchainName}/{project.outputArchitecture}/{project.targetName}/SprawlUnitTest".format(project=project)
		if platform.system() == "Windows":
			time.sleep(2)
		else:
			while not os.access(unitTestExe, os.X_OK):
				time.sleep(1)

		log.LOG_BUILD("Running unit tests...")
		ret = subprocess.call([unitTestExe, "--all"]);
		if ret != 0:
			log.LOG_ERROR("Unit tests failed! Errors reported: {}".format(ret));
