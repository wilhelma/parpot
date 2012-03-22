#include "DynCallGraphUtils.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <papi.h>

static const int MSIZE = 1000;
static double CallOverhead = 0;
static double LoopOverhead = 0;

double get_time() {
  return PAPI_get_real_cyc();
}

/*
 * insertNode inserts a new function node at the current (pCurrentLNode)
 * function.
 */
void insertNode(fGraphT *g, char *name, unsigned num) {

	/* get timestamp for overhead compensation */
	double start = get_time();

	/* declarations */
	unsigned tmp, sibling;

  if (g->nextSlot == 0) {
  	if (strcmp(name, "main") != 0)
  		return;
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
    g->array[g->currentNode].exTime = get_time();
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
      g->array = (fNodeT*)realloc(g->array, g->currentSize * sizeof(fNodeT));
  	}

    /* check if node already exist */
    tmp = g->array[g->currentNode].first_child;
    while (tmp) {
      if (g->array[tmp].num == num) {
      	g->array[tmp].count++;
      	g->array[tmp].exTime = get_time();
        g->array[tmp].profiling = true;
        g->currentNode = tmp;

			  /* increase overhead time for computation */
  			g->array[tmp].ovTime += get_time() - start;

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
    g->array[g->currentNode].exTime = get_time();
    g->array[g->currentNode].tmpTime = 0;
    g->array[g->currentNode].profiling = true;
    g->array[g->currentNode].first_child = 0;
    g->array[g->currentNode].last_child = 0;
    g->array[g->currentNode].sibling = 0;
  }

  /* increase overhead time for computation */
  g->array[g->currentNode].ovTime += get_time() - start;
}

/*
 * changeCurrentFunctionName changes the name of the current node.
 */
void changeCurrentFunctionName(fGraphT* g, char* name) {

  if (g->nextSlot == 0)
  	return;

	/* get timestamp for overhead compensation */
	double start = get_time();

	assert (g->array[g->currentNode].count &&
			"Error! No Function was called before!");
  g->array[g->currentNode].pName = name;

  /* increase overhead time for computation */
  g->array[g->currentNode].ovTime += get_time() - start;
}

/*
 * leaveNode returns to the last function node.
 */
void leaveNode(fGraphT* g, unsigned num) {
  if (g->nextSlot == 0)
  	return;

	/* get timestamp for overhead compensation */
	double start = get_time();

  /* declarations */
	unsigned node = g->currentNode;

	assert(g->array[g->currentNode].count &&
  		"Error! Inconsistent call graph detected!");

  /* calculate correct time */
  g->array[g->currentNode].tmpTime +=
  		get_time() - g->array[g->currentNode].exTime;
  g->array[g->currentNode].exTime = g->array[g->currentNode].tmpTime;
  g->array[g->currentNode].profiling = false;
  g->currentNode = g->array[g->currentNode].parent;

  /* increase overhead time for computation */
  g->array[node].ovTime += get_time() - start;
}

double calcOverhead(fGraphT *g, unsigned nodeIndex,
										double *fnOvhds) {
	unsigned tmp;
	fNodeT *pNode = &g->array[nodeIndex];

	/* increase overhead per called function */
	if (fnOvhds[nodeIndex] == 0) {

		 // 2 methods per procedure * call number times * (get_time + call ovhd)
		//fnOvhds[nodeIndex] += 3 * pNode->count * CallOverhead;
		// accumulated overhead for all calls of the given procedure
		if (pNode->ovTime >= 0)
			fnOvhds[nodeIndex] += pNode->ovTime;

		// consider inner calls recursively
	  tmp = pNode->first_child;
	  while (tmp) {
			fnOvhds[nodeIndex] += calcOverhead(g, tmp, fnOvhds);
	    tmp = g->array[tmp].sibling;
	  }
	}

	return fnOvhds[nodeIndex];
}

void writeNode(FILE *outFile, fGraphT *g, unsigned nodeIndex,
							 double *fnOvhds) {

	/* declarations */
	unsigned tmp;

	fNodeT *pNode = &g->array[nodeIndex];

  /* check execution time */
  if (pNode->profiling) {
    pNode->tmpTime += get_time() - pNode->exTime;
    pNode->exTime = pNode->tmpTime;
    pNode->profiling = false;
  }

  /* subtract overhead for measuring */
  pNode->exTime -= calcOverhead(g, nodeIndex, fnOvhds);
  pNode->exTime = (pNode->exTime < 0) ? 0 : pNode->exTime;

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
    writeNode(outFile, g, tmp, fnOvhds);
    tmp = g->array[tmp].sibling;
  }
}

void writeGraphToFile(fGraphT *g, const char * fileName) {
	if (g->nextSlot == 0)
  	return;

	double *fnOvhds = (double *) malloc(g->nextSlot * sizeof(double));
	memset(fnOvhds,'\0',g->nextSlot);

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
  writeNode(outFile, g, STARTSLOT, fnOvhds);

  /* write graph footer */
  fprintf(outFile, "}");

  /* close file */
  fclose(outFile);

  printf("Dynamic callgraph written to: %s...\n", fileName);
  printf("And the result is: %f\n", CallOverhead);
}

void doNothing(int i1, int i2, int i3) {
	get_time();
}

void startOvhdMeasure(void) {
	CallOverhead = get_time();
}

void stopOvhdMeasure(unsigned loopSize) {
	CallOverhead = (get_time() - CallOverhead - LoopOverhead) / loopSize;
	assert(LoopOverhead > 0 && "Measurement for loop overhead hasn't performed!");
}

void startLoopMeasure(void) {
	LoopOverhead = get_time();
}

void stopLoopMeasure(void) {
	LoopOverhead = get_time() - LoopOverhead;
}
