/* ***** BEGIN LICENSE BLOCK *****
 * queue.c
 * 
 * Implementation of a FIFO queue abstract data type.
 * 
 * by: Steven Skiena
 * begun: March 27, 2002
 * 
 * 
 *
 * Copyright 2003 by Steven S. Skiena; all rights reserved. 
 * 
 * Permission is granted for use in non-commerical applications
 * provided this copyright notice remains intact and unchanged.
 * 
 * This program appears in my book:
 * 
 " Programming Challenges: The Programming Contest Training Manual"
 * by Steven Skiena and Miguel Revilla, Springer-Verlag, New York 2003.
 * 
 * See our website www.programming-challenges.com for additional information.
 * 
 * This book can be ordered from Amazon.com at
 * 
 * http://www.amazon.com/exec/obidos/ASIN/0387001638/thealgorithmrepo/
 * 
 * ***** END LICENSE BLOCK ***** */
#define QUEUESIZE       200

typedef struct {
        void* items[QUEUESIZE+1];		/* body of queue */
        int first;                      /* position of first element */
        int last;                       /* position of last element */
        int count;                      /* number of queue elements */
} queue;

void init_queue(queue* q, int size_of_element);

int enqueue(queue* q, void* data, int size);

int dequeue(queue* q, void* data, int size);
