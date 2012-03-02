/*===- DynCallGraphUtils - Graph utils to build a dynamic callgraph - C -*-===*\
\*===----------------------------------------------------------------------===*/

#ifndef DYNCALLGRAPHUTILS_H
#define DYNCALLGRAPHUTILS_H

#define STARTSIZE 5000
#define STARTSLOT 1

#include <stdbool.h>

/*
 *  a function node with its edges
 */
typedef struct fNode {
	char *pName;
  unsigned num;
  unsigned count;
  double exTime;
  double tmpTime;
  bool profiling;
  unsigned parent;
  unsigned first_child;
  unsigned last_child;
  unsigned sibling;
} fNodeT;

/*
 * a graph structure
 */
typedef struct fGraph {
	fNodeT *array;
	unsigned currentSize;
	unsigned currentNode;
	unsigned nextSlot;
} fGraphT;



/*
 * insertNode inserts a new function node at the current (pCurrentLNode)
 * function.
 */
void insertNode(fGraphT *g, char *name, unsigned num);

void changeCurrentFunctionName(fGraphT* g, char* name);

void writeGraphToFile(fGraphT *g, const char*);

void leaveNode(fGraphT* g, unsigned num);

#endif
