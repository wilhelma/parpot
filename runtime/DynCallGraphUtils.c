#include "DynCallGraphUtils.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <papi.h>

__inline__ uint64_t rdtsc() {
  uint32_t lo, hi;
  __asm__ __volatile__ (      // serialize
  "xorl %%eax,%%eax \n        cpuid"
  ::: "%rax", "%rbx", "%rcx", "%rdx");
  /* We cannot use "=A", since this would use %rax on x86_64 and return only the lower 32bits of the TSC */
  __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
  return (uint64_t)hi << 32 | lo;
}

/*
 * insertNode inserts a new function node at the current (pCurrentLNode)
 * function.
 */
void insertNode(fGraphT *g, char *name, unsigned num) {

	/* declarations */
	unsigned tmp, sibling;

  if (g->nextSlot == 0) {
  	assert(strcmp(name, "main") == 0
						&& "Error! Program has no main function!");

  	/* initialize graph */
  	g->currentSize = STARTSIZE;
  	g->nextSlot = STARTSLOT;
  	g->array = malloc(STARTSIZE * sizeof(fNodeT));
  	assert (g->array && "Error! Not enough memory");

    /* create start node */
    g->currentNode = g->nextSlot++;
  	g->array[g->currentNode].pName = name;
    g->array[g->currentNode].num = num;
    g->array[g->currentNode].count = 1;
    g->array[g->currentNode].exTime = rdtsc();
    g->array[g->currentNode].tmpTime = 0;
    g->array[g->currentNode].profiling = true;
    g->array[g->currentNode].parent = 0;
    g->array[g->currentNode].first_child = 0;
    g->array[g->currentNode].last_child = 0;
    g->array[g->currentNode].sibling = 0;

  } else {
  	assert(g->array[STARTSLOT].count && "Error! Inconsistent graph state");

  	/* check array size and increase dynamically */
  	if (g->nextSlot == g->currentSize) {
  		g->currentSize *= 2;
      assert(realloc(g->array, g->currentSize * sizeof(fNodeT)) != NULL &&
							"Error! Not enough memory!");
  	}

    /* check if node already exist */
    tmp = g->array[g->currentNode].first_child;
    while (tmp) {
      if (g->array[tmp].num == num) {
      	g->array[tmp].count++;
      	g->array[tmp].exTime = rdtsc();
        g->array[tmp].profiling = true;
        g->currentNode = tmp;
        return; /* prohibit more than one analysis per node */
      }
      tmp = g->array[tmp].sibling;
    }

    /* node doesn't exist => create new node (and link with parent/sibling) */
    sibling = g->array[g->currentNode].last_child;
    if (sibling)
    	g->array[sibling].sibling = g->nextSlot;
    else
			g->array[g->currentNode].first_child = g->nextSlot;
		g->array[g->currentNode].last_child = g->nextSlot;

    g->array[g->nextSlot].parent = g->currentNode;
    g->currentNode = g->nextSlot++;
    g->array[g->currentNode].pName = name;
    g->array[g->currentNode].num = num;
    g->array[g->currentNode].count = 1;
    g->array[g->currentNode].exTime = rdtsc();
    g->array[g->currentNode].tmpTime = 0;
    g->array[g->currentNode].profiling = true;
    g->array[g->currentNode].first_child = 0;
    g->array[g->currentNode].last_child = 0;
    g->array[g->currentNode].sibling = 0;
  }
}

/*
 * changeCurrentFunctionName changes the name of the current node.
 */
void changeCurrentFunctionName(fGraphT* g, char* name) {
	assert (g->array[g->currentNode].count &&
			"Error! No Function was called before!");
  g->array[g->currentNode].pName = name;
}

/*
 * leaveNode returns to the last function node.
 */
void leaveNode(fGraphT* g, unsigned num) {

  /* declarations */
  /* unsigned tmp; */

  assert(g->array[g->currentNode].count &&
  		"Error! Inconsistent call graph detected!");

  /* check if node with given number is still in list */
/*  if (g->pCurrentNode->num != num) {
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
 	}*/

  /* calculate correct time */
  g->array[g->currentNode].tmpTime +=
  		rdtsc() - g->array[g->currentNode].exTime;
  g->array[g->currentNode].exTime = g->array[g->currentNode].tmpTime;
  g->array[g->currentNode].profiling = false;
  g->currentNode = g->array[g->currentNode].parent;
}

void writeNode(FILE *outFile, fGraphT *g, unsigned nodeIndex) {

	/* declarations */
	unsigned tmp;

	fNodeT *pNode = &g->array[nodeIndex];

  /* check execution time */
  if (pNode->profiling) {
    pNode->tmpTime += rdtsc() - pNode->exTime;
    pNode->exTime = pNode->tmpTime;
    pNode->profiling = false;
  }

  /* write node entry */
  fprintf(outFile, "\tNode%u [shape=record,label=\"{%s;%u;%f}\"];\n",
      nodeIndex, pNode->pName, pNode->num, pNode->exTime);

  /* write link information */
  if (pNode->parent)
    fprintf(outFile, "\tNode%u -> Node%u [label=\"%u\"];\n",
        pNode->parent, nodeIndex, pNode->count);

  /* write child nodes */
  tmp = pNode->first_child;
  while (tmp) {
    writeNode(outFile, g, tmp);
    tmp = g->array[tmp].sibling;
  }
}

void writeGraphToFile(fGraphT *g, const char * fileName) {

	for (int i=STARTSLOT; i<STARTSIZE && g->array[i].count; ++i) {
		printf("array[%d]: %s - first_child: %d last_child: %d sibling: %d\n", i, g->array[i].pName,
				g->array[i].first_child, g->array[i].last_child, g->array[i].sibling);
	}

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
  writeNode(outFile, g, STARTSLOT);

  /* write graph footer */
  fprintf(outFile, "}");

  /* close file */
  fclose(outFile);

  printf("Dynamic callgraph written to: %s...\n", fileName);
}
