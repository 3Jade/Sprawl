#!/usr/bin/python

import jmake

jmake.Standard("c++11")

jmake.Compiler("clang++")

jmake.Shared()
jmake.Output("libsprawl_multiaccess.so")

#For testing purposes, uncomment this and comment out the above two lines, then run the resulting executable in Release/jhash.
#This will put code through a suite of tests that should catch all the potential pitfalls and edge cases I've found so far.
#If you find new edge cases, please add them to the test suite.

jmake.WarnFlags("all", "extra", "ctor-dtor-privacy", "old-style-cast", "overloaded-virtual", "init-self", "missing-include-dirs", "switch-default", "switch-enum", "undef")

jmake.InstallHeaders()
jmake.InstallSubdir("sprawl/multiaccess")
jmake.InstallOutput()
