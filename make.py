#!/usr/bin/python

import csbuild
import subprocess
from csbuild import log

csbuild.Toolchain("gcc").Compiler().CppStandard("c++11")
csbuild.Toolchain("gcc").CppCompiler("clang++")

csbuild.Toolchain("gcc").Compiler().WarnFlags("all", "extra", "ctor-dtor-privacy", "old-style-cast", "overloaded-virtual", "init-self", "missing-include-dirs", "switch-default", "switch-enum", "undef")

csbuild.NoPrecompile()
csbuild.InstallHeaders()

csbuild.OutDir("lib")
csbuild.ObjDir("Intermediate/{project.name}")

@csbuild.project("collections", "collections")
def collections():
	csbuild.Output("libsprawl_collections", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/collections")
	
@csbuild.project("network", "network")
def network():
	csbuild.Output("libsprawl_network", csbuild.ProjectType.SharedLibrary)
	
	csbuild.InstallSubdir("sprawl/network")
	csbuild.InstallOutput()

	
@csbuild.project("serialization", "serialization")
def serialization():
	csbuild.Output("libsprawl_serialization", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/serialization")
	csbuild.InstallOutput()


@csbuild.project("memory", "memory")
def memory():
	csbuild.Output("libsprawl_memory", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/memory")


@csbuild.project("string", "string")
def string():
	csbuild.Output("libsprawl_string", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/string")
	csbuild.InstallOutput()


@csbuild.project("hash", "hash")
def string():
	csbuild.Output("libsprawl_hash", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/hash")
	csbuild.InstallOutput()


@csbuild.project("common", "common")
def string():
	csbuild.Output("libsprawl_common", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/common")

@csbuild.project("UnitTests", "UnitTests", ["collections", "network", "serialization", "memory", "string", "hash", "common"])
def UnitTests():
	csbuild.Output("SprawlUnitTest")
	csbuild.OutDir("bin")

	csbuild.LibDirs( "lib" )

	csbuild.Libraries(
		# Header only "sprawl_collections",
		"sprawl_network",
		# No tests yet, needs to be split up to remove Mongo requirement "sprawl_serialization",
		"sprawl_memory",
		"sprawl_string",
		"sprawl_hash",
		# Header only "sprawl_common",
	)

	@csbuild.postMakeStep
	def postMake(project):
		log.LOG_BUILD("Running unit tests...")
		ret = subprocess.call(["bin/SprawlUnitTest", "--all"]);
		if ret != 0:
			log.LOG_ERROR("Unit tests failed! Errors reported: {}".format(ret));
