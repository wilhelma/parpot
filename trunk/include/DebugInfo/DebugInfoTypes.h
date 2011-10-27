//===------ DebugInfo/DebugInfoType.h - DebugInfoType - Enumeration -------===//
//
//                 ParPot - Parallelization Potential - Measurement
//
//===----------------------------------------------------------------------===//
//
// This file declares the DebugInfoType enumeration.
//
//===----------------------------------------------------------------------===//
#ifndef PARPOT_DEBUGINFO_DEBUGINFOTYPES_H_
#define PARPOT_DEBUGINFO_DEBUGINFOTYPES_H_

/// The DebugInfoType enumeration will be used to determine various types of
/// debug information.
enum DebugInfoType {
  TCompileUnit 	= 1,   	/* Compile unit information */
  TSubprogram	= 2,   	/* Procedure information */
  TAllocation	= 3,	/* Allocation instruction information*/
  TGlobalVar	= 4,		/* Global variable information */
  TCallInstruction = 5 /* CallInstruction information */
};

#endif
