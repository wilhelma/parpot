#include "DynCallGraphUtils.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <papi.h>

static unsigned nodeNo = 0;

/*
 * insertNode inserts a new function node at the current (pCurrentLNode)
 * function.
 */
void insertNode(fGraphT *g, char *name, unsigned num) {

  /* declarations */
  fEdgeT *pEdge, *pTmpEdge;
  fNodeT *pNode;
  if (!g->pStartNode) {
  	assert(strcmp(name, "main") == 0
						&& "Error! Program has no main function!");

    /* create start node */
    g->pStartNode = (fNodeT *) malloc(sizeof(fNodeT));
    g->pStartNode->pName = name;
    g->pStartNode->num = num;
    g->pStartNode->id = nodeNo++;
    g->pStartNode->count = 1;
    g->pStartNode->pEdges = NULL;
    g->pStartNode->exTime = PAPI_get_real_cyc();
    g->pStartNode->tmpTime = 0;
    g->pStartNode->profiling = true;
    g->pStartNode->pParent = NULL;

    /* create first list node */
    g->pCurrentNode = g->pStartNode;
  } else {
  	assert(g->pCurrentNode && g->pStartNode
						&& "Error! Inconsistent graph state");

    /* check if node already exist */
    pTmpEdge = g->pCurrentNode->pEdges;
    while (pTmpEdge != NULL) {
      if (pTmpEdge->pNodeTo->num == num) {
        pTmpEdge->pNodeTo->count++;
        pTmpEdge->pNodeTo->exTime = PAPI_get_real_cyc();
        pTmpEdge->pNodeTo->profiling = true;
        g->pCurrentNode = pTmpEdge->pNodeTo;
        return; /* prohibit more than one analysis per node */
      }
      pTmpEdge = pTmpEdge->pNext;
    }

    /* node doesn't exist => create new node */
    pNode = (fNodeT *) malloc(sizeof(fNodeT));
    pNode->pName = name;
    pNode->num = num;
    pNode->id = nodeNo++;
    pNode->count = 1;
    pNode->exTime = PAPI_get_real_cyc();
    pNode->tmpTime = 0;
    pNode->profiling = true;
    pNode->pEdges = NULL;
    pNode->pParent = g->pCurrentNode;

    /* add new edge */
    pEdge = (fEdgeT *) malloc(sizeof(fEdgeT));
    pEdge->pNodeTo = pNode;
    pEdge->pNext = NULL;

    pTmpEdge = g->pCurrentNode->pEdges;
    /* insert edge into the correct position of parent's edge-list  */
    if (pTmpEdge == NULL) {
      g->pCurrentNode->pEdges = pEdge;
    } else {
      while(pTmpEdge->pNext != NULL)
        pTmpEdge = pTmpEdge->pNext;
      pTmpEdge->pNext = pEdge;
    }

    /* set current node further */
    g->pCurrentNode = pNode;
  }
}

/*
 * changeCurrentFunctionName changes the name of the current node.
 */
void changeCurrentFunctionName(fGraphT* g, char* name) {
	assert (g->pCurrentNode && "Error! No Function was called before!");
  g->pCurrentNode->pName = name;
}

/*
 * leaveNode returns to the last function node.
 */
void leaveNode(fGraphT* g, unsigned num) {

  /* declarations */
  fEdgeT* tmpEdge;

  assert(g->pStartNode && "Error! Inconsistent call graph detected!");

  /* check if node with given number is still in list */
  if (g->pCurrentNode->num != num) {
		tmpEdge = g->pCurrentNode->pParent->pEdges;
 		while (tmpEdge->pNodeTo->num != num) {
 			if (tmpEdge->pNext)
 				tmpEdge = tmpEdge->pNext;
 			else {
 				assert (tmpEdge->pNodeTo != g->pStartNode
 								&& "Error! Node was already deleted => Graph inconsistent!");
 				tmpEdge = tmpEdge->pNodeTo->pParent->pEdges;
 			}
 		}
 	}

  /* calculate correct time */
  g->pCurrentNode->tmpTime += PAPI_get_real_cyc() - g->pCurrentNode->exTime;
  g->pCurrentNode->exTime = g->pCurrentNode->tmpTime;
  g->pCurrentNode->profiling = false;

  g->pCurrentNode = g->pCurrentNode->pParent;

//  /* free node with corresponding edges */
//  while (tmpEdge) {
//  	tmpEdge2Free = tmpEdge;
//  	tmpEdge = tmpEdge->pNext;
//  	free(tmpEdge2Free);
//  }
//  free (tmpNode);
}

void writeNode(FILE *outFile, fNodeT* pNode, fNodeT* pParent) {
  /* declarations */
  fEdgeT *pEdge;

  /* check execution time */
  if (pNode->profiling) {
    pNode->tmpTime += PAPI_get_real_cyc() - pNode->exTime;
    pNode->exTime = pNode->tmpTime;
    pNode->profiling = false;
  }

  /* write node entry */
  fprintf(outFile, "\tNode%u [shape=record,label=\"{%s;%u;%f}\"];\n",
      pNode->id, pNode->pName, pNode->num, pNode->exTime);

  /* write link information */
  if (pParent != NULL)
    fprintf(outFile, "\tNode%u -> Node%u [label=\"%u\"];\n",
        pParent->id, pNode->id, pNode->count);

  /* write child nodes */
  pEdge = pNode->pEdges;
  while (pEdge != NULL) {
    writeNode(outFile, pEdge->pNodeTo, pNode);
    pEdge = pEdge->pNext;
  }
}

void writeGraphToFile(fGraphT *g, char * fileName) {
  /* open file for writing */
  FILE *outFile = fopen(fileName, "w");
  if (!outFile) {
    fprintf(stderr, "LLVM profiling runtime: while opening '%s': ",
            fileName);
    perror("");
    return;
  }

  /* write graph header */
  fprintf(outFile, "digraph \"Dynamic Call Graph\" {\n");
  fprintf(outFile, "\tlabel=\"Dynamic Call Graph\";\n\n");

  /* write nodes recursively */
  writeNode(outFile, g->pStartNode, NULL);

  /* write graph footer */
  fprintf(outFile, "}");

  /* close file */
  fclose(outFile);

  printf("Dynamic callgraph written to: %s...\n", fileName);
}
