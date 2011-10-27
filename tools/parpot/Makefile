##===- projects/parpot/tools/parpot/Makefile ---------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of the tool.
#
TOOLNAME=parpot

#
# List libraries that we'll need
# We use LIBS because parpot is a dynamic library.
#
USEDLIBS = parpot_instrumentation.a

LINK_COMPONENTS := 	jit interpreter nativecodegen bitreader bitwriter \
										selectiondag

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

