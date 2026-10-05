#include "os.h"
#include "compiler.h"
#include <stdio.h>
#include <string.h>

#define NP 4
#define TIME_SLICE 10
#define MAX_TASKS 32

typedef struct {
    int pid;
    int proc_id;  //processor_id -> which processor is running for a particular process.
    int active;
    char program[260];   //Stores the program filename.
    char data[260];   //Stores the data filename.
    Processor cpu;  //Each task gets its own processor state. This is important for multitasking.
    int page_table[LOGICAL_PAGES];
} Task;

static Task tasks[MAX_TASKS];
static int next_pid = 1;
static unsigned char frame_used[PHYSICAL_PAGES];

void os_init(void)
{
    memset(tasks, 0, sizeof(tasks));
    memset(frame_used, 0, sizeof(frame_used));
    frame_used[0] = 1; /* Frame 0 is reserved. */
    memory_reset();
}

static int free_frame(void)
{
    for (int i = 1; i < PHYSICAL_PAGES; i++) {
        if (!frame_used[i]) {
            frame_used[i] = 1;
            return i;
        }
    }
    return -1;
}

static void release_frames(Task *t)
{
    for (int i = 0; i < LOGICAL_PAGES; i++) {
        if (t->page_table[i] >= 0) {
            frame_used[t->page_table[i]] = 0;
            t->page_table[i] = -1;
        }
    }
}

static int load_task(Task *t)
{
    int instruction_pages = INSTR_SIZE / PAGESIZE;  //2
    int data_pages = DATA_SIZE / PAGESIZE;  //8
    int i;

    for (i = 0; i < LOGICAL_PAGES; i++) 
        t->page_table[i] = -1;

    if (memory_load_file(instruction_memory[t->proc_id], INSTR_SIZE, t->program) < 0)
        return -1;
    if (memory_load_file(data_memory[t->proc_id], DATA_SIZE, t->data) < 0)
        return -1;

    /* Give the task 2 instruction pages followed by 8 data pages. */
    for (i = 0; i < instruction_pages + data_pages; i++) {
        t->page_table[i] = free_frame();
        if (t->page_table[i] < 0) {
            release_frames(t);
            return -1;
        }
    }

    /* Copy logical instruction/data bytes into physical memory. */
    for (i = 0; i < INSTR_SIZE; i++) {
        int logical_page = i / PAGESIZE;
        int physical = t->page_table[logical_page] * PAGESIZE + i % PAGESIZE;
        memory[physical] = instruction_memory[t->proc_id][i];
    }

    //copy data into physical memory
    for (i = 0; i < DATA_SIZE; i++) {
        int logical_page = 2 + i / PAGESIZE;   //Because logical pages 0 and 1 are instructions.
        int physical = t->page_table[logical_page] * PAGESIZE + i % PAGESIZE;
        memory[physical] = data_memory[t->proc_id][i];
    }

    processor_reset(&t->cpu, t->proc_id);
    return 0;
}

static int find_free_processor(void)
{
    int used[NP] = {0};
    for (int i = 0; i < MAX_TASKS; i++)
        if (tasks[i].active && tasks[i].proc_id >= 0)
            used[tasks[i].proc_id] = 1;

    for (int i = 0; i < NP; i++)
        if (!used[i]) return i;
    return -1;
}

int os_submit(const char *program_file, const char *data_file)
{
    int slot = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!tasks[i].active) 
        { slot = i; 
            break; }
    }
    if (slot < 0) 
        return -1;

    Task *t = &tasks[slot];
    t->pid = next_pid++;
    t->proc_id = find_free_processor();
    t->active = 1;
    strncpy(t->program, program_file, sizeof(t->program) - 1);   //Store the program filename.
    strncpy(t->data, data_file ? data_file : "data.byte", sizeof(t->data) - 1);   //If a data filename was provided, use it.

    if (t->proc_id >= 0) {
        if (load_task(t) != 0) {
            t->active = 0;
            return -1;
        }
    }

    printf("OS: PID %d submitted on processor %d\n", t->pid, t->proc_id);
    return t->pid;
}

static int active_tasks(void)
{
    int count = 0;
    for (int i = 0; i < MAX_TASKS; i++) count += tasks[i].active;
    return count;
}

static void start_waiting_task(void)
{
    int proc = find_free_processor();
    if (proc < 0) return;

    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].active && tasks[i].proc_id < 0) {
            tasks[i].proc_id = proc;
            load_task(&tasks[i]);
            printf("OS: PID %d moved from waiting to processor %d\n", tasks[i].pid, proc);
            return;
        }
    }
}

void os_run(void)
{
    while (active_tasks() > 0) {
        int ran_something = 0;

        for (int i = 0; i < MAX_TASKS; i++) {
            Task *t = &tasks[i];
            if (!t->active || t->proc_id < 0) continue;

            ran_something = 1;
            processor_run_slice(&t->cpu, t->page_table, TIME_SLICE);

            if (t->cpu.halted) {
                /* Copy the task's physical data back to its logical data area. */
                for (int b = 0; b < DATA_SIZE; b++) {
                    int page = 2 + b / PAGESIZE;  //2 because first two pages are occuppied by instructions 
                    int physical = t->page_table[page] * PAGESIZE + b % PAGESIZE;
                    data_memory[t->proc_id][b] = memory[physical];
                }
                memory_save_file(data_memory[t->proc_id], DATA_SIZE, t->data);
                processor_close_log(&t->cpu);
                release_frames(t);
                printf("OS: PID %d finished\n", t->pid);
                t->active = 0;
                t->proc_id = -1;
                start_waiting_task();
            }
        }

        if (!ran_something) break;
    }
}

void os_shell(void)
{
    char line[512];
    printf("$ ");

    while (fgets(line, sizeof(line), stdin)) {
        char *program;
        char *data;

        line[strcspn(line, "\r\n")] = '\0';
        if (!strcmp(line, "exit")) break;
        if (!line[0]) { printf("$ "); continue; }

        program = strtok(line, " \t");  //gets first file name
        data = strtok(NULL, " \t");     //gets second file name
        if (!data) data = "data.byte";

        if (strstr(program, ".txt") != NULL) {
            if (compile_file(program, "program.byte", data) != 0) {
                printf("Compilation failed\n$ ");
                continue;
            }
            os_submit("program.byte", data);
        } 
        
        else {
            os_submit(program, data);
        }
        os_run();
        printf("$ ");
    }
}

void os_shutdown(void)
{
    for (int i = 0; i < MAX_TASKS; i++)
        if (tasks[i].active) processor_close_log(&tasks[i].cpu);
}
