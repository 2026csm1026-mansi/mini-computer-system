#include "compiler.h"
#include "os.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    os_init();

    /* If a source program is supplied, compile and run it. */
    if (argc >= 2) {
        const char *data = (argc >= 3) ? argv[2] : "data.byte";

        printf("Compiling %s...\n", argv[1]);
        if (compile_file(argv[1], "program.byte", data) != 0) {
            printf("Compilation failed.\n");
            return 1;
        }

        if (os_submit("program.byte", data) < 0) {
            printf("Could not load program.\n");
            return 1;
        }
        os_run();
    }

    printf("\nCS527 Lab 5 Mini Computer\n");
    printf("Type: program.txt data.byte   or   exit\n");
    os_shell();
    os_shutdown();
    return 0;
}
