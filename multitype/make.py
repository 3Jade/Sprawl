#!/usr/bin/python

import jmake

jmake.Output("libsprawl_multitype.so")

jmake.InstallSubdir("sprawl/multitype")
jmake.InstallHeaders()