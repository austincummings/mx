#ifndef DEBUG_H
#define DEBUG_H

#include <stdarg.h>

void debug(const char *format, ...);

void todo(const char *format, ...);

void mx_abort();

void setup_signal_handler();

#endif
