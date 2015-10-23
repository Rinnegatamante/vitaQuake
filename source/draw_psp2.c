/*
 * Copyright (c) 2015 Sergi Granell (xerpi)
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <psp2/display.h>
#include <psp2/gxm.h>
#include <psp2/kernel/sysmem.h>
#include "draw_psp2.h"
#include "utils.h"
#include <jpeglib.h>
#include <setjmp.h>

extern const unsigned char msx_font[];

SceDisplayFrameBuf fb;
static SceUID fb_memuid;

static void *alloc_gpu_mem(uint32_t type, uint32_t size, uint32_t attribs, SceUID *uid)
{
	int ret;
	void *mem = NULL;

	if (type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW)
		size = align_mem(size, 256 * 1024);
	else
		size = align_mem(size, 4 * 1024);

	*uid = sceKernelAllocMemBlock("gxm", type, size, NULL);

	printf("MemBlock uid: 0x%08X\n", *uid);

	ret = sceKernelGetMemBlockBase(*uid, &mem);
	printf("sceKernelGetMemBlockBase(): 0x%08X\n", ret);
	printf("MemBlockBase addr: %p\n", mem);
	if (ret != 0) {
		return NULL;
	}

	ret = sceGxmMapMemory(mem, size, attribs);
	printf("sceGxmMapMemory(): 0x%08X\n", ret);
	if (ret != 0) {
		return NULL;
	}

	return mem;
}

void init_video()
{
	int ret;

	SceGxmInitializeParams params;

	params.flags                        = 0x0;
	params.displayQueueMaxPendingCount  = 0x1;
	params.displayQueueCallback         = 0x0;
	params.displayQueueCallbackDataSize = 0x0;
	params.parameterBufferSize          = (16 * 1024 * 1024);

	/* Initialize the GXM */
	ret = sceGxmInitialize(&params);
	printf("sceGxmInitialize(): 0x%08X\n", ret);

	/* Setup framebuffers */
	fb.size        = sizeof(fb);
	fb.pitch       = SCREEN_W;
	fb.pixelformat = PSP2_DISPLAY_PIXELFORMAT_A8B8G8R8;
	fb.width       = SCREEN_W;
	fb.height      = SCREEN_H;

	/* Allocate memory for the framebuffers */
	fb.base = alloc_gpu_mem(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		SCREEN_W * SCREEN_H * 4, SCE_GXM_MEMORY_ATTRIB_RW, &fb_memuid);

	if (fb.base == NULL) {
		printf("Could not allocate memory for fb. %p", fb.base);
		return;
	}

	sceDisplaySetFrameBuf(&fb, PSP2_DISPLAY_SETBUF_IMMEDIATE);

	printf(
		"\nframebuffer 0:\n"
		"\tsize:           0x%08X\n"
		"\tbase:           0x%08X\n"
		"\tpitch:          0x%08X\n"
		"\tpixelformat:    0x%08X\n"
		"\twidth:          0x%08X\n"
		"\theight          0x%08X\n",
		fb.size, (uintptr_t)fb.base,
		fb.pitch, fb.pixelformat, fb.width, fb.height);

}

void end_video()
{
	sceGxmUnmapMemory(fb.base);
	sceKernelFreeMemBlock(fb_memuid);
	sceGxmTerminate();
}

void clear_screen()
{
	memset(fb.base, 0x00, SCREEN_W*SCREEN_H*4);
}

void draw_pixel(uint32_t x, uint32_t y, uint32_t color)
{
	((uint32_t *)fb.base)[x + y*fb.pitch] = color;
}

void draw_rectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
	int i, j;
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			((uint32_t *)fb.base)[(x + j) + (y + i)*fb.pitch] = color;
		}
	}
}

void font_draw_char(int x, int y, uint32_t color, char c)
{
	unsigned char *font = (unsigned char *)(msx_font + (c - (uint32_t)' ') * 8);
	int i, j, pos_x, pos_y;
	for (i = 0; i < 8; ++i) {
		pos_y = y + i*2;
		for (j = 0; j < 8; ++j) {
			pos_x = x + j*2;
			if ((*font & (128 >> j))) {
				draw_pixel(pos_x + 0, pos_y + 0, color);
				draw_pixel(pos_x + 1, pos_y + 0, color);
				draw_pixel(pos_x + 0, pos_y + 1, color);
				draw_pixel(pos_x + 1, pos_y + 1, color);
			}
		}
		++font;
	}
}

struct my_error_mgr {
struct jpeg_error_mgr pub;
jmp_buf setjmp_buffer;
};
typedef struct my_error_mgr * my_error_ptr;
METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
my_error_ptr myerr = (my_error_ptr) cinfo->err;
(*cinfo->err->output_message) (cinfo);
longjmp(myerr->setjmp_buffer, 1);
}

uint8_t* decodeJpg(unsigned char* in,uint64_t size)
{
    struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, in, size);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);
    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int row_bytes = width * cinfo.num_components;
    uint8_t* bgr_buffer = (uint8_t*) malloc(width*height*cinfo.num_components);
    while (cinfo.output_scanline < cinfo.output_height) {
        uint8_t* buffer_array[1];
        buffer_array[0] = bgr_buffer + (cinfo.output_scanline) * row_bytes;
        jpeg_read_scanlines(&cinfo, buffer_array, 1);
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return bgr_buffer;
}

void font_draw_string(int x, int y, uint32_t color, const char *string)
{
	if (string == NULL) return;

	int startx = x;
	const char *s = string;

	while (*s) {
		if (*s == '\n') {
			x = startx;
			y += 16;
		} else if (*s == ' ') {
			x += 16;
		} else if(*s == '\t') {
			x += 16*4;
		} else {
			font_draw_char(x, y, color, *s);
			x += 16;
		}
		++s;
	}
}

void font_draw_stringf(int x, int y, uint32_t color, const char *s, ...)
{
	char buf[256];
	va_list argptr;
	va_start(argptr, s);
	vsnprintf(buf, sizeof(buf), s, argptr);
	va_end(argptr);
	font_draw_string(x, y, color, buf);
}
