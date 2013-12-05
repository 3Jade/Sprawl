#!/usr/bin/python

import csbuild

csbuild.call("multiaccess/make.py")
csbuild.call("multitype/make.py")
csbuild.call("format/make.py")
csbuild.call("network/make.py")
csbuild.call("serialization/make.py")
