#include "compiler.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 2048
#define MAX_LABELS 256
#define MAX_LINE 512

typedef struct {
    char name[64];
    int pc;
} Label;

static Label labels[MAX_LABELS];
static int label_count;

static char *trim(char *s)
{
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return s;
}

static void remove_comment(char *s)
{
    char *p = strchr(s, '%');
    if (p) *p = '\0';
}

static int register_number(const char *s, char type)
{
    char *end;
    long n;

    if (s == NULL || s[0] != type) 
        return -1;

    n = strtol(s + 1, &end, 10);
    if (*end != '\0') 
        return -1;
    if (type == 'x' && (n < 0 || n >= 256)) 
        return -1;
    if (type == 'v' && (n < 0 || n >= 32)) 
        return -1;
    return (int)n;
}

static int get_number(const char *s, int *ok)
{
    char *end;
    long n = strtol(s, &end, 0);
    *ok = (*s != '\0' && *end == '\0' && n >= 0 && n <= 255);
    return (int)n;
}

static int split(char *line, char *t[], int max)
{
    int n = 0;
    char *p = strtok(line, " \t,;\r\n");
    while (p && n < max) {
        t[n++] = p;
        p = strtok(NULL, " \t,;\r\n");
    }
    return n;
}

static void emit(FILE *fp, int a, int b, int c, int d)
{
    fprintf(fp, "%02X %02X %02X %02X\n", a & 255, b & 255, c & 255, d & 255);   //emit() writes one 4-byte machine instruction to program.byte in hexadecimal format. The & 255 ensures each field is limited to 8 bits.
}

static int find_label(const char *name)
{
    for (int i = 0; i < label_count; i++)
        if (strcmp(labels[i].name, name) == 0) 
            return labels[i].pc;
    return -1;
}

static int branch_opcode(const char *s)
{
    if (!strcmp(s, "BEQ")) return 0x10;
    if (!strcmp(s, "BNE")) return 0x11;
    if (!strcmp(s, "BGT")) return 0x12;
    if (!strcmp(s, "BLT")) return 0x13;
    if (!strcmp(s, "BGE")) return 0x14;
    if (!strcmp(s, "BLE")) return 0x15;
    if (!strcmp(s, "BAL")) return 0x16;
    return -1;
}

static int arithmetic_opcode(const char *op, int vector, int constant)
{
    if (!vector) {    //If vector = 0 → scalar operation using x registers.    //If vector = 1 → vector operation using v registers.
        if (!strcmp(op, "+")) return constant ? 0x09 : 0x01;
        if (!strcmp(op, "-")) return constant ? 0x0A : 0x02;
        if (!strcmp(op, "*")) return constant ? 0x0B : 0x03;
        if (!strcmp(op, "/")) return constant ? 0x0C : 0x04;
    } else {
        if (!strcmp(op, "+")) return constant ? 0x29 : 0x21;
        if (!strcmp(op, "-")) return constant ? 0x2A : 0x22;
        if (!strcmp(op, "*")) return constant ? 0x2B : 0x23;
    }
    return -1;
}

static int memory_address(const char *text, int *reg, int *constant)
{
    char temp[64];  //a temp string for copying char after [ for const address
    int ok = 0;

    *reg = -1;
    *constant = -1;
    if (text[0] != '[') return 0;

    strncpy(temp, text + 1, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    char *right = strchr(temp, ']');   //strchr() searches for the ] character.
    if (!right) return 0;
    *right = '\0';

    *reg = register_number(temp, 'x');
    if (*reg >= 0) return 1;

    *constant = get_number(temp, &ok);
    return ok;
}

int compile_file(const char *source, const char *program_byte, const char *data_byte)
{
    FILE *in = fopen(source, "r");
    FILE *out;
    char *lines[MAX_LINES];
    char buffer[MAX_LINE];
    int line_count = 0;
    int pc = 0;

    if (!in) {
        fprintf(stderr, "Compiler: cannot open %s\n", source);
        return -1;
    }

    label_count = 0;

    /* Pass 1: keep useful lines and calculate real instruction addresses. */
    while (fgets(buffer, sizeof(buffer), in) && line_count < MAX_LINES) {
        char clean[MAX_LINE];
        char *tokens[8];
        int n;

        strcpy(clean, buffer);
        remove_comment(clean);
        char *s = trim(clean);
        if (!*s) continue;

        lines[line_count] = malloc(strlen(s) + 1);
        strcpy(lines[line_count], s);

        strcpy(clean, s);
        n = split(clean, tokens, 8);
        if (n == 1 && tokens[0][0] == '.') {
            if (label_count < MAX_LABELS) {
                strcpy(labels[label_count].name, tokens[0]);
                labels[label_count].pc = pc;
                label_count++;
            }
        } else {
            pc += 4;
        }
        line_count++;
    }
    fclose(in);

    out = fopen(program_byte, "w");
    if (!out) return -1;

    int success = 1;

    for (int line_no = 0; line_no < line_count; line_no++) {
        char line[MAX_LINE];
        char *t[8];
        int n;
        int current_pc;

        strcpy(line, lines[line_no]);
        n = split(line, t, 8);
        if (n == 0) continue;
        if (n == 1 && t[0][0] == '.') continue;

        current_pc = 0;
        for (int k = 0; k < line_no; k++) {
            char temp[MAX_LINE];
            char *u[8];
            strcpy(temp, lines[k]);
            if (split(temp, u, 8) == 1 && u[0][0] == '.') continue;
            current_pc += 4;
        }

        if (n == 2) {
            int bop = branch_opcode(t[0]);
            if (bop >= 0) {
                int target = find_label(t[1]);
                if (target < 0) { success = 0; continue; }
                emit(out, bop, 0, 0, (target - current_pc - 4));
                continue;
            }
        }

        if (!strcmp(t[0], "Print") && n == 2) {
            int r = register_number(t[1], 'x');
            if (r >= 0) emit(out, 0x08, 0, 0, r);
            else success = 0;
            continue;
        }

        /* VSum xN = vM: horizontal sum of all 8 vector elements. */
        if (!strcmp(t[0], "VSum") && n == 4 && !strcmp(t[2], "=")) {
            int dest = register_number(t[1], 'x');
            int src = register_number(t[3], 'v');
            if (dest >= 0 && src >= 0) emit(out, 0x30, dest, src, 0);
            else success = 0;
            continue;
        }

        if ((!strcmp(t[0], "Read") || !strcmp(t[0], "Write")) && n == 3) {
            int r = register_number(t[1], 'x');
            int ok;
            int address = get_number(t[2], &ok);
            if (r >= 0 && ok) emit(out, !strcmp(t[0], "Read") ? 0x0D : 0x0E, r, 0, address);
            else success = 0;
            continue;
        }

        if (n < 3 || strcmp(t[1], "=") != 0) {
            success = 0;
            continue;
        }

        int dx = register_number(t[0], 'x');
        int dv = register_number(t[0], 'v');

        /* dest = [x2] or dest = [32] */
        if (n == 3 && t[2][0] == '[') {
            int ar, address;
            if (!memory_address(t[2], &ar, &address)) { success = 0; continue; }
            if (dx >= 0) emit(out, ar >= 0 ? 0x05 : 0x0D, dx, 0, ar >= 0 ? ar : address);
            else if (dv >= 0) emit(out, ar >= 0 ? 0x25 : 0x2D, dv, 0, ar >= 0 ? ar : address);
            else success = 0;
            continue;
        }

        /* [x2] = x1 or [32] = v1 */
        if (t[0][0] == '[' && n == 3) {
            int ar, address;
            int rx = register_number(t[2], 'x');
            int rv = register_number(t[2], 'v');
            if (!memory_address(t[0], &ar, &address)) { success = 0; continue; }
            if (rx >= 0) emit(out, ar >= 0 ? 0x06 : 0x0E, rx, 0, ar >= 0 ? ar : address);
            else if (rv >= 0) emit(out, ar >= 0 ? 0x26 : 0x2E, rv, 0, ar >= 0 ? ar : address);
            else success = 0;
            continue;
        }

        /* dest = x1 or dest = 10 */
        if (n == 3) {
            int ok;
            int value = get_number(t[2], &ok);
            int sx = register_number(t[2], 'x');
            if (dx >= 0 && ok) emit(out, 0x0F, dx, 0, value);
            else if (dx >= 0 && sx >= 0) emit(out, 0x07, dx, 0, sx);
            else success = 0;
            continue;
        }

        /* dest = src1 op src2 */
        if (n == 5) {
            int s1x = register_number(t[2], 'x');
            int s1v = register_number(t[2], 'v');
            int s2x = register_number(t[4], 'x');
            int s2v = register_number(t[4], 'v');
            int ok;
            int constant = get_number(t[4], &ok);
            int op;

            if (dv >= 0 && s1v >= 0) {
                if (s2v >= 0) op = arithmetic_opcode(t[3], 1, 0);
                else if (s2x >= 0) op = arithmetic_opcode(t[3], 1, 0);
                else if (ok) op = arithmetic_opcode(t[3], 1, 1);
                else op = -1;
                if (op >= 0) emit(out, op, dv, s1v, s2v >= 0 ? s2v : (s2x >= 0 ? s2x : constant));
                else success = 0;
                continue;
            }

            if (dx >= 0 && s1x >= 0) {
                if (s2x >= 0) op = arithmetic_opcode(t[3], 0, 0);
                else if (ok) op = arithmetic_opcode(t[3], 0, 1);
                else op = -1;
                if (op >= 0) emit(out, op, dx, s1x, s2x >= 0 ? s2x : constant);
                else success = 0;
                continue;
            }
        }

        success = 0;
    }

    /* Zero opcode is the end instruction. */
    emit(out, 0, 0, 0, 0);
    fclose(out);

    for (int i = 0; i < line_count; i++) free(lines[i]);

    /* Create an empty data file only when the named file does not exist. */
    FILE *df = fopen(data_byte, "a");
    if (df) fclose(df);

    if (!success) {
        fprintf(stderr, "Compiler: one or more lines were invalid.\n");
        return -1;
    }
    return 0;
}
