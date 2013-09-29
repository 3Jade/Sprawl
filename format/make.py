#!/usr/bin/python

import jmake

jmake.Output("libsprawl_format.so")

jmake.Standard("c++11")

jmake.Compiler("clang++")

jmake.Shared()

jmake.WarnFlags("all", "extra", "ctor-dtor-privacy", "old-style-cast", "overloaded-virtual", "init-self", "missing-include-dirs", "switch-default", "switch-enum", "undef")

jmake.InstallHeaders()
jmake.InstallSubdir("sprawl/format")
jmake.InstallOutput()