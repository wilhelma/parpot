##===- projects/parpot/lib/parpot/Analyis/Makefile ---------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..
DSA_INCLUDE=/home/wilhelma/tools/llvm30/projects/poolalloc/include/dsa

#
# Give the name of a library.  This will build a dynamic version.
#
LIBRARYNAME=parpot_analysis
SHARED_LIBRARY = 1
LINK_LIBS_IN_SHARED = 1
CFLAGS += -I$(DSA_INCLUDE)
CXXFLAGS += -I$(DSA_INCLUDE)
CPPFLAGS += -I$(DSA_INCLUDE)

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

#LIBS += -lLLVMDataStructure