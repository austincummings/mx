#define _GNU_SOURCE // Use GNU extensions (backtrace, backtrace_symbols_fd)
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Add this for strsignal()
#include <unistd.h>

#include "debug.h"

void debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

void todo(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    abort();
}

void print_stack_trace(void) {
    void *buffer[100];
    int nptrs = backtrace(buffer, 100);
    char **symbols = backtrace_symbols(buffer, nptrs);

    if (symbols == NULL) {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Stack trace:\n");
    for (int i = 0; i < nptrs; i++) {
        fprintf(stderr, "stack[%d] %s\n", i, symbols[i]);

        // Resolve function name using addr2line
        // char cmd[256];
        // snprintf(cmd, sizeof(cmd), "addr2line -f -e mx %p", buffer[i]);
        // system(cmd);
    }

    free(symbols);
}

void crash_handler(int sig) {
    fprintf(stderr, "\nCaught signal %d (%s)\n", sig, strsignal(sig));
    print_stack_trace();
    exit(EXIT_FAILURE); // Exit after printing the stack trace
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = crash_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NODEFER;

    sigaction(SIGSEGV, &sa, NULL); // Segmentation fault
    sigaction(SIGABRT, &sa, NULL); // Abort (e.g., from assert())
    sigaction(SIGFPE, &sa, NULL);  // Floating point exception
    sigaction(SIGILL, &sa, NULL);  // Illegal instruction
    sigaction(SIGBUS, &sa, NULL);  // Bus error (bad memory access)
}
