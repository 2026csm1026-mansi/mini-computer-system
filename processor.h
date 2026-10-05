#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "memory.h"
#include <stdio.h>

#define NUM_REGISTERS 256
#define NUM_VECTOR_REGISTERS 32
#define VECTOR_ELEMENTS 8

/* One processor has its own registers, PC and flags. */
typedef struct {
    unsigned int r[NUM_REGISTERS];   //int registers
    unsigned int v[NUM_VECTOR_REGISTERS][VECTOR_ELEMENTS];
    unsigned int pc;
    int Z, N, C, V;
    int halted;
    int proc_id;
    FILE *log;
} Processor;

void processor_reset(Processor *p, int proc_id);
int processor_step(Processor *p, int page_table[]);
int processor_run_slice(Processor *p, int page_table[], int count);
void processor_close_log(Processor *p);

#endif
