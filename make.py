#!/usr/bin/python

import csbuild

csbuild.Toolchain("gcc").CppStandard("c++11")
csbuild.Toolchain("gcc").CppCompiler("clang++")

csbuild.Toolchain("gcc").WarnFlags("all", "extra", "ctor-dtor-privacy", "old-style-cast", "overloaded-virtual", "init-self", "missing-include-dirs", "switch-default", "switch-enum", "undef")

csbuild.NoPrecompile()

@csbuild.project("multiaccess", "multiaccess/src")
def multiaccess():
	csbuild.Output("libsprawl_multiaccess", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallHeaders()
	csbuild.InstallSubdir("sprawl/multiaccess")
	csbuild.InstallOutput()

@csbuild.project("multitype", "multitype/src")
def multitype():
	csbuild.Output("libsprawl_multitype", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallSubdir("sprawl/multitype")
	csbuild.InstallHeaders()
	
@csbuild.project("format", "format/src")
def format():
	csbuild.Output("libsprawl_format", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallHeaders()
	csbuild.InstallSubdir("sprawl/format")
	csbuild.InstallOutput()
	
@csbuild.project("network", "network/src")
def network():
	csbuild.Output("libsprawl_network", csbuild.ProjectType.SharedLibrary)
	
	#csbuild.NoPrecompile("network.hpp")
	
	csbuild.InstallHeaders()
	csbuild.InstallSubdir("sprawl/network")
	csbuild.InstallOutput()
	
@csbuild.project("serialization", "serialization/src")
def serialization():
	csbuild.Output("libsprawl_serialization", csbuild.ProjectType.SharedLibrary)

	csbuild.InstallHeaders()
	csbuild.InstallSubdir("sprawl/serialization")
	csbuild.InstallOutput()
