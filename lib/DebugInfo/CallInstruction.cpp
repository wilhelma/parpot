#include "DebugInfo/CallInstruction.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace llvm;

bool CallInstruction::store(const char *filename) const {
	// open file to write
	std::ofstream file(filename, std::ios_base::app);
	if (!file) {
	    errs() << "Error: Can't open file " << filename << '\n';
		return false;
	}

	int ty = getDebugType();
	file << ty << ',';					// write debugtype information
	file << getInstNo() << ',';			// write callinstruction number
	file << getLineNo() << ',';		// write line number
	file << getFile() << ";\n"; // write corresponding file name
	file.close();						// close file
	return true;
}


CallInstruction::CallInstruction(std::string line, LineInstMapTy* liMap):
		DebugInfo(TAllocation) {
    std::istringstream sLine( line );
    std::string elem;

    if (!getline( sLine, elem, ',' )) // read debug type
      DebugInfo::terminate(line);

    if (!getline( sLine, elem, ',' )) // read instruction number
      DebugInfo::terminate(line);
    int tmp;
    std::stringstream strStream(elem);
    strStream >> tmp;
    inst_ = (*liMap)[tmp];

    if (!getline( sLine, elem, ',' )) // read line number
      DebugInfo::terminate(line);
    std::stringstream strStream2(elem);
    strStream2 >> lineNo_;

    if (!getline( sLine, elem, ';' )) // read corresponding file name
      DebugInfo::terminate(line);
    fileName_ = elem;
}
