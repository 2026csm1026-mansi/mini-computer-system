#ifndef MEMORY_H
#define MEMORY_H

#include <stdio.h>

#define MEMSIZE 8192
#define PAGESIZE 512
#define MAX_PROC 4
#define INT_REGS 256
#define VECTOR_REGS 32
#define VECTOR_ELEMENTS 8
#define INSTR_SIZE 1024
#define DATA_SIZE 4096
#define LOGICAL_PAGES ((INSTR_SIZE + DATA_SIZE) / PAGESIZE)
#define PHYSICAL_PAGES (MEMSIZE / PAGESIZE)

extern unsigned char memory[MEMSIZE];
extern unsigned char instruction_memory[MAX_PROC][INSTR_SIZE];
extern unsigned char data_memory[MAX_PROC][DATA_SIZE];

void memory_reset(void);
int memory_load_file(unsigned char *memory_area, int max_size, const char *file);
int memory_save_file(const unsigned char *memory_area, int size, const char *file);

#endif
