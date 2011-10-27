#include "DebugInfo/CompileUnit.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace llvm;

bool CompileUnit::store(const char *filename) const {
	// open file to write
	std::ofstream file(filename, std::ios_base::app);
	if (!file) {
	    errs() << "Error: Can't open file " << filename << '\n';
		return false;
	}

	int ty = getDebugType();
	file << ty << ',';					// write type information
	file << getFilename() << ',';		// write filename
	file << getDirectory() << ";\n";	// write directory
	file.close();						// close file
	return true;
}

CompileUnit::CompileUnit(std::string line): DebugInfo(TCompileUnit) {
    std::istringstream sLine( line );
    std::string elem;


    if (!getline( sLine, elem, ',' ))	// read debug type
    	DebugInfo::terminate(line);

    if (!getline( sLine, elem, ',' ))	// read filename
    	DebugInfo::terminate(line);
    filename_ = elem;

    if (!getline( sLine, elem, ',' ))	// read directory
    	DebugInfo::terminate(line);
    directory_ = elem;
}
