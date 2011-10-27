#include "DebugInfo/Subprogram.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace llvm;

bool Subprogram::store(const char *filename) const {
	// open file to write
	std::ofstream file(filename, std::ios_base::app);
	if (!file) {
	    errs() << "Error: Can't open file " << filename << '\n';
		return false;
	}

	int ty = getDebugType();
	file << ty << ',';					// write type information
	file << getName() << ',';			// write name
	file << getDisplayName() << ",";	// write display name
	file << getCompileUnit() << ",";	// write compile unit
	file << getLineNo() << ";\n";		// write line number
	file.close();						// close file
	return true;
}

Subprogram::Subprogram(std::string line): DebugInfo(TSubprogram) {
    std::istringstream sLine( line );
    std::string elem;


    if (!getline( sLine, elem, ',' ))	// read debug type
    	DebugInfo::terminate(line);

    if (!getline( sLine, elem, ',' ))	// read name
    	DebugInfo::terminate(line);
    name_ = elem;

    if (!getline( sLine, elem, ',' ))	// read display name
    	DebugInfo::terminate(line);
    displayName_ = elem;

    if (!getline( sLine, elem, ',' ))	// read compile-unit
    	DebugInfo::terminate(line);
    compileUnit_ = elem;

    if (!getline( sLine, elem, ',' ))	// read line number
    	DebugInfo::terminate(line);
    std::stringstream strStream(elem);
    strStream >> lineNo_;
}
