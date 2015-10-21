/*
 * Copyright (c) 2015 Sergi Granell (xerpi)
 */

#include <stdio.h>
#include <stdarg.h>

#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/error.h>

#include "console_psp2.h"

static int top_margin = 10;
static int cns_x = 0, cns_y = 0;
static uint32_t cns_color = WHITE;
static int console_initialzed = 0;
static SceUID console_mtx;

void console_init()
{
	if (console_initialzed) {
		return;
	}

	console_reset();

	console_mtx = sceKernelCreateMutex("console_mutex", 0, 0, NULL);
	if (console_mtx > 0) {
		DEBUG("Console Mutex UID: 0x%08X\n", console_mtx);
	}

	console_initialzed = 1;
}

void console_fini()
{
	if (console_initialzed) {
		sceKernelDeleteMutex(console_mtx);
	}
}

void console_reset()
{
	cns_x = 10;
	cns_y = top_margin;
}

void console_putc(char c)
{
	int mtx_err = sceKernelTryLockMutex(console_mtx, 1);

	int last_y = cns_y;
	if (c == '\n') {
		cns_y += 20;
		cns_x = 10;
	} else if (c == '\t') {
		cns_x += 16*4;
	} else if (c >= ' ' && c <= 126) {
		font_draw_char(cns_x, cns_y, cns_color, c);
		cns_x += 16;
	}
	if (cns_x >= (SCREEN_W-16)) {
		cns_y += 20;
		cns_x = 10;
	}
	if (cns_y >= (SCREEN_H-16)) {
		cns_y = top_margin;
	}
	if (cns_y != last_y) {
		draw_rectangle(0, cns_y, SCREEN_W, 20, BLACK);
		if ((cns_y+20+16) < SCREEN_H) {
			draw_rectangle(0, cns_y+20, SCREEN_W, 20, BLACK);
			if ((cns_y+20+20+16) < SCREEN_H) {
				draw_rectangle(0, cns_y+40, SCREEN_W, 20, BLACK);
			}
		}
	}

	if (mtx_err == SCE_KERNEL_OK) {
		sceKernelUnlockMutex(console_mtx, 1);
	}
}

void console_print(const char *s)
{
	int mtx_err = sceKernelTryLockMutex(console_mtx, 1);

	while (*s) {
		console_putc(*s);
		s++;
	}

	if (mtx_err == SCE_KERNEL_OK) {
		sceKernelUnlockMutex(console_mtx, 1);
	}
}

void console_printf(const char *s, ...)
{
	unsigned int mtx_timeout = 0xFFFFFFFF;
	sceKernelLockMutex(console_mtx, 1, &mtx_timeout);

	char buf[256];
	va_list argptr;
	va_start(argptr, s);
	vsnprintf(buf, sizeof(buf), s, argptr);
	va_end(argptr);
	console_print(buf);

	sceKernelUnlockMutex(console_mtx, 1);
}

void console_set_color(uint32_t color)
{
	cns_color = color;
}

int console_get_y()
{
	return cns_y;
}

void console_set_top_margin(int new_top_margin)
{
	top_margin = new_top_margin;
}
