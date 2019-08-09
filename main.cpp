#include <iostream>
#include "Machine.h"

// Unix specific code for terminal
struct termios original_tio;

void disable_input_buffering()
{
    tcgetattr(STDIN_FILENO, &original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        /* show usage string */
        printf("lc3 [image-file1] ...\n");
        exit(2);
    }

    // Setup
    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    Machine machine = Machine();
    for (int j = 1; j < argc; ++j)
    {
        if (machine.readImage(argv[j]))
        {
            printf("failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    machine.run();

    restore_input_buffering();
    return 0;
}