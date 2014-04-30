#!/usr/bin/python

import csbuild

csbuild.Toolchain("gcc").Compiler().CppStandard("c++11")
csbuild.Toolchain("gcc").CppCompiler("clang++")

csbuild.Toolchain("gcc").Compiler().WarnFlags("all", "extra", "ctor-dtor-privacy", "old-style-cast", "overloaded-virtual", "init-self", "missing-include-dirs", "switch-default", "switch-enum", "undef")

csbuild.NoPrecompile()
csbuild.InstallHeaders()

@csbuild.project("multiaccess", "multiaccess")
def multiaccess():
	csbuild.Output("libsprawl_multiaccess", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/multiaccess")
	csbuild.InstallOutput()

@csbuild.project("multitype", "multitype")
def multitype():
	csbuild.Output("libsprawl_multitype", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/multitype")

	
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


@csbuild.project("memory", "memory", [ "string", "hash" ])
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
