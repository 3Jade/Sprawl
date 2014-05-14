#!/usr/bin/python

import subprocess
import os
import csbuild
from csbuild import log

csbuild.Toolchain("gcc").Compiler().CppStandard("c++11")
csbuild.Toolchain("gcc").CppCompiler("clang++")

csbuild.Toolchain("msvc").SetMsvcVersion(110)

csbuild.Toolchain("gcc").Compiler().WarnFlags("all", "extra", "ctor-dtor-privacy", "old-style-cast", "overloaded-virtual", "init-self", "missing-include-dirs", "switch-default", "switch-enum", "undef")

csbuild.NoPrecompile()

csbuild.OutDir("lib/{project.activeToolchainName}/{project.outputArchitecture}/{project.targetName}")
csbuild.ObjDir("Intermediate/{project.activeToolchainName}/{project.outputArchitecture}/{project.targetName}/{project.name}")

csbuild.add_option("--with-mongo", action="store", help="Path to mongo include directory. If not specified, mongo will not be built.")
csbuild.add_option("--with-boost", action="store", help="Path to boost include directory. If not specified, mongo will not be built.")

csbuild.InstallSubdir("sprawl/{project.name}")

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

MongoDir = os.path.abspath(csbuild.get_option("with_mongo"))
BoostDir = os.path.abspath(csbuild.get_option("with_boost"))
if (not MongoDir) ^ (not BoostDir):
	log.LOG_ERROR("Both mongo and boost directories must be specified to build MongoSerializer.");
	csbuild.Exit(1)
	
if MongoDir and BoostDir:
	@csbuild.project("serialization-mongo", "serialization/mongo")
	def serialization():
		csbuild.Output("libsprawl_serialization-mongo", csbuild.ProjectType.StaticLibrary)
		
		csbuild.IncludeDirs("./serialization", os.path.join(MongoDir, "include"), os.path.join(BoostDir, "include"))
		csbuild.LibDirs(os.path.join(MongoDir, "lib"), os.path.join(BoostDir, "lib"))
	
		csbuild.InstallSubdir("sprawl/serialization")
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

@csbuild.project("UnitTests", "UnitTests", ["collections", "network", "serialization", "memory", "string", "hash", "common"])
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

	@csbuild.postMakeStep
	def postMake(project):
		log.LOG_BUILD("Running unit tests...")
		ret = subprocess.call(["bin/{project.activeToolchainName}/{project.outputArchitecture}/{project.targetName}/SprawlUnitTest".format(project=project), "--all"]);
		if ret != 0:
			log.LOG_ERROR("Unit tests failed! Errors reported: {}".format(ret));
