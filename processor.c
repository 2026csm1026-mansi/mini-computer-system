#include "processor.h"
#include <stdint.h>
#include <string.h>

static uint32_t read32(const unsigned char *m)
{
    return (uint32_t)m[0] |
           ((uint32_t)m[1] << 8) |
           ((uint32_t)m[2] << 16) |
           ((uint32_t)m[3] << 24);
}

static void write32(unsigned char *m, uint32_t value)
{
    m[0] = (unsigned char)value;
    m[1] = (unsigned char)(value >> 8);
    m[2] = (unsigned char)(value >> 16);
    m[3] = (unsigned char)(value >> 24);
}

static void set_add_flags(Processor *p, uint32_t a, uint32_t b, uint32_t r)
{
    p->Z = (r == 0);
    p->N = (r >> 31) & 1;
    p->C = (r < a || r < b);
    p->V = ((a >> 31) == (b >> 31)) && ((r >> 31) != (a >> 31));
}

static void set_sub_flags(Processor *p, uint32_t a, uint32_t b, uint32_t r)
{
    p->Z = (r == 0);
    p->N = (r >> 31) & 1;
    p->C = (a > b);
    p->V = ((a >> 31) != (b >> 31)) && ((r >> 31) == (b >> 31));
}

/* Convert a logical address into a physical address using the task page table. */
static int physical_address(int page_table[], int address)
{
    int page, offset;

    if (address < 0 || address + 4 > DATA_SIZE) return -1;
    page = address / PAGESIZE;
    offset = address % PAGESIZE;

    if (page_table[2 + page] < 0) return -1;
    return page_table[2 + page] * PAGESIZE + offset;
}

static int branch_taken(Processor *p, int condition)
{
    switch (condition) {
        case 0: return p->Z;                    /* BEQ */
        case 1: return !p->Z;                   /* BNE */
        case 2: return !p->Z && p->N == p->V;  /* BGT */
        case 3: return p->N != p->V;            /* BLT */
        case 4: return p->Z || p->N == p->V;   /* BGE */
        case 5: return p->Z || p->N != p->V;   /* BLE */
        case 6: return 1;                       /* BAL */
        default: return 0;
    }
}

void processor_reset(Processor *p, int proc_id)
{
    char name[64];
    memset(p, 0, sizeof(*p));
    p->proc_id = proc_id;

    snprintf(name, sizeof(name), "processor_%d.log", proc_id);  //creates log file
    p->log = fopen(name, "a");
}

void processor_close_log(Processor *p)
{
    if (p->log != NULL) {
        fclose(p->log);
        p->log = NULL;
    }
}

int processor_step(Processor *p, int page_table[])
{
    unsigned char op, a, b, c;
    unsigned char *im = instruction_memory[p->proc_id];
    int physical;

    if (p->halted) return 0;
    if (p->pc + 3 >= INSTR_SIZE) {
        p->halted = 1;
        return 0;
    }

    op = im[p->pc];
    a  = im[p->pc + 1];
    b  = im[p->pc + 2];
    c  = im[p->pc + 3];
    p->pc += 4;

    switch (op) {
        case 0x00: /* End */
            p->halted = 1;
            break;

        case 0x01: { /* x[a] = x[b] + x[c] */
            uint32_t r = p->r[b] + p->r[c];
            set_add_flags(p, p->r[b], p->r[c], r);
            p->r[a] = r;
            break;
        }
        case 0x02: { /* subtraction */
            uint32_t r = p->r[b] - p->r[c];
            set_sub_flags(p, p->r[b], p->r[c], r);
            p->r[a] = r;
            break;
        }
        case 0x03: p->r[a] = p->r[b] * p->r[c]; break;
        case 0x04: p->r[a] = p->r[c] ? p->r[b] / p->r[c] : 0; break;

        case 0x09: { /* add constant */
            uint32_t r = p->r[b] + c;
            set_add_flags(p, p->r[b], c, r);
            p->r[a] = r;
            break;
        }
        case 0x0A: {
            uint32_t r = p->r[b] - c;
            set_sub_flags(p, p->r[b], c, r);
            p->r[a] = r;
            break;
        }
        case 0x0B: p->r[a] = p->r[b] * c; break;
        case 0x0C: p->r[a] = c ? p->r[b] / c : 0; break;

        case 0x07: /* register copy */
            p->r[a] = p->r[c];
            break;
        case 0x0F: /* x[a] = constant */
            p->r[a] = c;
            break;

        case 0x05: /* x[a] = [x[c]] */
            physical = physical_address(page_table, (int)p->r[c]);
            if (physical < 0) { p->halted = 1; break; }
            p->r[a] = read32(memory + physical);
            break;

        case 0x06: /* [x[c]] = x[a] */
            physical = physical_address(page_table, (int)p->r[c]);
            if (physical < 0) { p->halted = 1; break; }
            write32(memory + physical, p->r[a]);
            break;

        case 0x0D: /* x[a] = [constant] */
            physical = physical_address(page_table, c);
            if (physical < 0) { p->halted = 1; break; }
            p->r[a] = read32(memory + physical);
            break;

        case 0x0E: /* [constant] = x[a] */
            physical = physical_address(page_table, c);
            if (physical < 0) { p->halted = 1; break; }
            write32(memory + physical, p->r[a]);
            break;

        case 0x08: /* Print */
            if (p->log)
                fprintf(p->log, "Process id: x%u : %08X\n", c, p->r[c]);
            break;

        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x14: case 0x15: case 0x16:
            if (branch_taken(p, op & 0x0F))
                p->pc = (unsigned int)((int)p->pc + (int8_t)c);
            break;

        case 0x21: case 0x22: case 0x23: /* vector-vector */
            for (int i = 0; i < VECTOR_ELEMENTS; i++) {
                if (op == 0x21) p->v[a][i] = p->v[b][i] + p->v[c][i];
                if (op == 0x22) p->v[a][i] = p->v[b][i] - p->v[c][i];
                if (op == 0x23) p->v[a][i] = p->v[b][i] * p->v[c][i];
            }
            break;

        case 0x29: case 0x2A: case 0x2B: /* vector-constant */
            for (int i = 0; i < VECTOR_ELEMENTS; i++) {
                if (op == 0x29) p->v[a][i] = p->v[b][i] + c;
                if (op == 0x2A) p->v[a][i] = p->v[b][i] - c;
                if (op == 0x2B) p->v[a][i] = p->v[b][i] * c;
            }
            break;

        case 0x25: { /* vector load from x address */
            int address = (int)p->r[c];
            for (int i = 0; i < VECTOR_ELEMENTS; i++) {
                physical = physical_address(page_table, address + i * 4);
                if (physical < 0) { p->halted = 1; break; }
                p->v[a][i] = read32(memory + physical);
            }
            break;
        }

        case 0x2D: { /* vector load from constant address */
            int address = c;
            for (int i = 0; i < VECTOR_ELEMENTS; i++) {
                physical = physical_address(page_table, address + i * 4);
                if (physical < 0) { p->halted = 1; break; }
                p->v[a][i] = read32(memory + physical);
            }
            break;
        }

        case 0x26: { /* vector store to x address */
            int address = (int)p->r[c];
            for (int i = 0; i < VECTOR_ELEMENTS; i++) {
                physical = physical_address(page_table, address + i * 4);
                if (physical < 0) { p->halted = 1; break; }
                write32(memory + physical, p->v[a][i]);
            }
            break;
        }

        case 0x2E: { /* vector store to constant address */
            int address = c;
            for (int i = 0; i < VECTOR_ELEMENTS; i++) {
                physical = physical_address(page_table, address + i * 4);
                if (physical < 0) { p->halted = 1; break; }
                write32(memory + physical, p->v[a][i]);
            }
            break;
        }

        case 0x30: { /* x[a] = sum(v[b][0..7]) */
            uint32_t sum = 0;
            for (int i = 0; i < VECTOR_ELEMENTS; i++)
                sum += p->v[b][i];
            p->r[a] = sum;
            set_add_flags(p, sum, 0, sum);
            break;
        }

        default:
            p->halted = 1;
            break;
    }

    return 1;
}

int processor_run_slice(Processor *p, int page_table[], int count)
{
    int done = 0;
    while (done < count && !p->halted) {
        if (!processor_step(p, page_table)) break;
        done++;
    }
    return done;
}
