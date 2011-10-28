/*===- DynCallGraphUtils - Graph utils to build a dynamic callgraph - C -*-===*\
\*===----------------------------------------------------------------------===*/

#ifndef DYNCALLGRAPHUTILS_H
#define DYNCALLGRAPHUTILS_H

#include <stdbool.h>

/*
 *  a function node with its edges
 */
typedef struct fNode {
  char *pName;
  unsigned num;
  unsigned id;
  unsigned count;
  double exTime;
  double tmpTime;
  bool profiling;
  struct fEdge *pEdges;
} fNodeT;

/*
 * an edge between two function nodes (linked list)
 */
typedef struct fEdge {
  struct fNode *pNodeTo;
  struct fEdge *pNext;
} fEdgeT;

/*
 * a linked list (bottom-up)
 */
typedef struct fListNode {
  fNodeT *pNode;
  struct fListNode *pParent;
} fListNodeT;

/*
 * a graph structure
 */
typedef struct fGraph {
  fNodeT *pStartNode;
  fListNodeT *pCurrentLNode;
} fGraphT;

/*
 * insertNode inserts a new function node at the current (pCurrentLNode)
 * function.
 */
void insertNode(fGraphT *g, char *name, unsigned num);

void changeCurrentFunctionName(fGraphT* g, char* name);

void writeGraphToFile(fGraphT *g, char*);

#endif