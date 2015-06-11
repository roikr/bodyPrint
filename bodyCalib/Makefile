# Attempt to load a config.make file.
# If none is found, project defaults in config.project.make will be used.
ifneq ($(wildcard config.make),)
	include config.make
endif

# make sure the the OF_ROOT location is defined
ifndef OF_ROOT
    OF_ROOT=../../..
endif

# call the project makefile!
include $(OF_ROOT)/libs/openFrameworksCompiled/project/makefileCommon/compile.project.mk

override CFLAGS += -I/usr/include/openni2
override LDFLAGS += -L/usr/lib/i386-linux-gnu -lusb-1.0
