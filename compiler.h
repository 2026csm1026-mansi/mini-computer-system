#ifndef COMPILER_H
#define COMPILER_H

/* Convert a program.txt file into program.byte. */
int compile_file(const char *source, const char *program_byte, const char *data_byte);

#endif
