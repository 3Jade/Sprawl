#!/usr/bin/python

import csbuild

csbuild.Output("libsprawl_multitype.so")

csbuild.InstallSubdir("sprawl/multitype")
csbuild.InstallHeaders()
