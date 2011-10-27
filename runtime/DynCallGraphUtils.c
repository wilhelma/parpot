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
  fListNodeT *pLNode;
  fNodeT *pNode;
  if (!g->pStartNode) {

    if (strcmp(name, "main") != 0)
      return;

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

    /* create first list node */
    g->pCurrentLNode = (fListNodeT *) malloc(sizeof(fListNodeT));
    g->pCurrentLNode->pNode = g->pStartNode;
    g->pCurrentLNode->pParent = NULL;
  } else {
    if (g->pCurrentLNode)
    if (g->pStartNode)
    /* check if node already exist */
    pTmpEdge = g->pStartNode->pEdges;
    while (pTmpEdge != NULL) {
      if (pTmpEdge->pNodeTo->num == num) {
        pTmpEdge->pNodeTo->count++;
        pTmpEdge->pNodeTo->exTime = PAPI_get_real_cyc();
        pTmpEdge->pNodeTo->profiling = true;
        pLNode = (fListNodeT *) malloc(sizeof(fListNodeT));
        pLNode->pNode = pTmpEdge->pNodeTo;
        pLNode->pParent = g->pCurrentLNode;
        g->pCurrentLNode = pLNode;
        return;
      }
      pTmpEdge = pTmpEdge->pNext;
    }

    /* create new node */
    pNode = (fNodeT *) malloc(sizeof(fNodeT));
    pNode->pName = name;
    pNode->num = num;
    pNode->id = nodeNo++;
    pNode->count = 1;
    pNode->exTime = PAPI_get_real_cyc();
    pNode->tmpTime = 0;
    pNode->profiling = true;
    pNode->pEdges = NULL;

    /* add new edge */
    pEdge = (fEdgeT *) malloc(sizeof(fEdgeT));
    /*pEdge->pNodeFrom = g->pCurrentLNode->pNode;*/
    pEdge->pNodeTo = pNode;
    pEdge->pNext = NULL;
    pTmpEdge = g->pCurrentLNode->pNode->pEdges;
    if (pTmpEdge == NULL) {
      g->pCurrentLNode->pNode->pEdges = pEdge;
    } else {
      while(pTmpEdge->pNext != NULL)
        pTmpEdge = pTmpEdge->pNext;
      pTmpEdge->pNext = pEdge;
    }

    /* add new list node */
    pLNode = (fListNodeT *) malloc(sizeof(fListNodeT));
    pLNode->pNode = pNode;
    pLNode->pParent = g->pCurrentLNode;
    g->pCurrentLNode = pLNode;
  }
}

void changeCurrentFunctionName(fGraphT* g, char* name) {
  if (g->pCurrentLNode)
    g->pCurrentLNode->pNode->pName = name;
}

/*
 * leaveNode returns to the last function node.
 */
void leaveNode(fGraphT* g, unsigned num) {

  /* declarations */
  fListNodeT* tmp;

  if (!g->pStartNode) return;
  assert(g->pCurrentLNode->pNode->num == num
      && "Wrong function no in stack!");

  /* calculate correct time */
  g->pCurrentLNode->pNode->tmpTime += PAPI_get_real_cyc() -
                                    g->pCurrentLNode->pNode->exTime;
  g->pCurrentLNode->pNode->exTime = g->pCurrentLNode->pNode->tmpTime;
  g->pCurrentLNode->pNode->profiling = false;

  tmp = g->pCurrentLNode;
  g->pCurrentLNode = g->pCurrentLNode->pParent;
  free (tmp);
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
