#!/usr/bin/python

import csbuild

csbuild.Toolchain("gcc").CppStandard("c++11")
csbuild.Toolchain("gcc").CppCompiler("clang++")

csbuild.Toolchain("gcc").WarnFlags("all", "extra", "ctor-dtor-privacy", "old-style-cast", "overloaded-virtual", "init-self", "missing-include-dirs", "switch-default", "switch-enum", "undef")

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
	
@csbuild.project("format", "format")
def format():
	csbuild.Output("libsprawl_format", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/format")
	csbuild.InstallOutput()
	
@csbuild.project("network", "network")
def network():
	csbuild.Output("libsprawl_network", csbuild.ProjectType.SharedLibrary)
	
	csbuild.InstallSubdir("sprawl/network")
	csbuild.InstallOutput()
	
@csbuild.project("serialization", "serialization")
def serialization():
	csbuild.Output("libsprawl_serialization", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/serialization")

@csbuild.project("memory", "memory")
def memory():
	csbuild.Output("libsprawl_memory", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/memory")

@csbuild.project("string", "string", [ "hash" ])
def string():
	csbuild.Output("libsprawl_string", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/string")
	csbuild.InstallOutput()

@csbuild.project("hash", "hash")
def string():
	csbuild.Output("libsprawl_hash", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/hash")
	csbuild.InstallOutput()
