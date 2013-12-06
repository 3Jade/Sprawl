#!/usr/bin/python

import csbuild

csbuild.Output("libsprawl_network.so")

csbuild.NoPrecompile("src/network.hpp")

csbuild.Standard("c++11")

csbuild.Compiler("clang++")

csbuild.Shared()

csbuild.WarnFlags("all", "extra", "ctor-dtor-privacy", "old-style-cast", "overloaded-virtual", "init-self", "missing-include-dirs", "switch-default", "switch-enum", "undef")

csbuild.InstallHeaders()
csbuild.InstallSubdir("sprawl/network")
csbuild.InstallOutput()