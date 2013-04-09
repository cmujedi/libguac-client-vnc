/* ***** BEGIN LICENSE BLOCK *****
 * queue.c
 * 
 * Implementation of a FIFO queue abstract data type.
 * 
 * by: Steven Skiena
 * begun: March 27, 2002
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
 
#include <stdlib.h>
#include <string.h>
#include "queue.h"

void init_queue(queue* q, int size_of_element) {   
    q->first = 0;
    q->last = QUEUESIZE-1;
    q->count = 0;
    
    int i;
    for(i = 0; i < QUEUESIZE; i++)
        q->items[i] = malloc(size_of_element);
}

int enqueue(queue* q, void* data, int size) {
    if (q->count >= QUEUESIZE)
        return -1;
        
    q->last = (q->last+1) % QUEUESIZE;
    // q->items[ q->last ] = data; 
    memcpy(q->items[ q->last ], data, size);   
    q->count = q->count + 1;
    
    return 0;
}

int dequeue(queue* q, void* data, int size) {
    if (q->count <= 0) 
        return -1;
        
    // data = q->items[ q->first ];
    memcpy(data, q->items[ q->first ], size);  
    q->first = (q->first+1) % QUEUESIZE;
    q->count = q->count - 1;

    return 0;
}