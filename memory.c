#include "memory.h"
#include <string.h>

unsigned char memory[MEMSIZE];
unsigned char instruction_memory[MAX_PROC][INSTR_SIZE];
unsigned char data_memory[MAX_PROC][DATA_SIZE];

void memory_reset(void)
{
    memset(memory, 0, sizeof(memory));
    memset(instruction_memory, 0, sizeof(instruction_memory));
    memset(data_memory, 0, sizeof(data_memory));
}

int memory_load_file(unsigned char *area, int max_size, const char *file)
{
    FILE *fp = fopen(file, "r");
    unsigned int value;
    int count = 0;

    if (fp == NULL) return -1;

    while (count < max_size && fscanf(fp, "%2x", &value) == 1)
        area[count++] = (unsigned char)value;

    fclose(fp);
    return count;
}

int memory_save_file(const unsigned char *area, int size, const char *file)
{
    FILE *fp = fopen(file, "w");
    int i, j;

    if (fp == NULL) return -1;

    for (i = 0; i < size; i += 4) {
        for (j = 0; j < 4; j++) {
            unsigned char value = (i + j < size) ? area[i + j] : 0;
            fprintf(fp, "%02X%c", value, (j == 3) ? '\n' : ' ');
        }
    }

    fclose(fp);
    return 0;
}
