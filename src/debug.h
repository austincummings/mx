#ifndef _MX_DEBUG_H
#define _MX_DEBUG_H

#include <stdarg.h>

void debug(const char *format, ...);

void todo(const char *format, ...);

void setup_signal_handler();

#endif // _MX_DEBUG_H
