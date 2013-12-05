#!/usr/bin/python

import csbuild

csbuild.Standard("c++11")

csbuild.Compiler("clang++")

csbuild.Shared()
csbuild.Output("libsprawl_multiaccess.so")

#For testing purposes, uncomment this and comment out the above two lines, then run the resulting executable in Release/jhash.
#This will put code through a suite of tests that should catch all the potential pitfalls and edge cases I've found so far.
#If you find new edge cases, please add them to the test suite.

csbuild.WarnFlags("all", "extra", "ctor-dtor-privacy", "old-style-cast", "overloaded-virtual", "init-self", "missing-include-dirs", "switch-default", "switch-enum", "undef")

csbuild.InstallHeaders()
csbuild.InstallSubdir("sprawl/multiaccess")
csbuild.InstallOutput()
