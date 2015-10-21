/*
 * Copyright (c) 2015 Sergi Granell (xerpi)
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <psp2/types.h>
#include "draw_psp2.h"

void console_init();
void console_fini();
void console_reset();
void console_putc(char c);
void console_print(const char *s);
void console_printf(const char *s, ...);
void console_set_color(uint32_t color);
int console_get_y();
void console_set_top_margin(int new_top_margin);

#define INFO(...) console_printf(__VA_ARGS__)

#if defined(SHOW_DEBUG)
#  define DEBUG(...) console_printf(__VA_ARGS__);
#else
#  define DEBUG(...)
#endif


#endif
