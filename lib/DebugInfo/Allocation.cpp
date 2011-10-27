#include "DebugInfo/Allocation.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace llvm;

bool Allocation::store(const char *filename) const {
	// open file to write
	std::ofstream file(filename, std::ios_base::app);
	if (!file) {
	    errs() << "Error: Can't open file " << filename << '\n';
		return false;
	}

	int ty = getDebugType();
	file << ty << ',';					// write debugtype information
	file << getName() << ',';			// write name
	file << getDisplayName() << ",";	// write display name
	file << getType() << ",";			// write type information
	file << getSubprogram() << ",";		// write subprogram
	file << getCompileUnit() << ",";	// write compile unit (dir + file)
	file << getLineNo() << ";\n";		// write line number
	file.close();						// close file
	return true;
}

Allocation::Allocation(std::string line): DebugInfo(TAllocation) {
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

    if (!getline( sLine, elem, ',' ))	// read type information
    	DebugInfo::terminate(line);
    type_ = elem;

    if (!getline( sLine, elem, ',' ))	// read sub-program
    	DebugInfo::terminate(line);
    subprogram_ = elem;

    if (!getline( sLine, elem, ',' ))	// read compile-unit
    	DebugInfo::terminate(line);
    compileUnit_ = elem;

    if (!getline( sLine, elem, ',' ))	// read line number
    	DebugInfo::terminate(line);
    std::stringstream strStream(elem);
    strStream >> lineNo_;
}
