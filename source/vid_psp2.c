/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <psp2/display.h>
#include <vita2d.h>
#include "quakedef.h"
#include "d_local.h"
#include "danzeff.h"
#define u16 uint16_t
#define u8 uint8_t

viddef_t	vid;				// global video state
int isDanzeff = false;
int old_char = 0;
extern native_resolution;
int old_resolution = 1;
int firststart = 1;

#define	BASEWIDTH	960
#define	BASEHEIGHT	544
#define SURFCACHE_SIZE 4194304

short	zbuffer[BASEWIDTH*BASEHEIGHT];
byte*	surfcache;
vita2d_texture* tex_buffer;
u16	d_8to16table[256];

void	VID_SetPalette (unsigned char *palette)
{
	int i;
	uint32_t* palette_tbl = vita2d_texture_get_palette(tex_buffer);
	u8* pal = palette;
	unsigned r, g, b;
	
	for(i=0; i<256; i++){
		r = pal[0];
		g = pal[1];
		b = pal[2];
		palette_tbl[i] = r | (g << 8) | (b << 16) | (0xFF << 24);
		pal += 3;
	}
}

void	VID_ShiftPalette (unsigned char *palette)
{
	VID_SetPalette(palette);
}

void	VID_Init (unsigned char *palette)
{
	
	if (firststart){
	
		// Init GPU
		vita2d_init();
		vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));
		vita2d_set_vblank_wait(0);
		
	}
	
	if (native_resolution){
		tex_buffer = vita2d_create_empty_texture_format(BASEWIDTH, BASEHEIGHT, SCE_GXM_TEXTURE_BASE_FORMAT_P8);
		vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
		vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
		vid.rowbytes = vid.conrowbytes = BASEWIDTH;
	}else{
		tex_buffer = vita2d_create_empty_texture_format(BASEWIDTH/2, BASEHEIGHT/2, SCE_GXM_TEXTURE_BASE_FORMAT_P8);
		vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH/2;
		vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT/2;
		vid.rowbytes = vid.conrowbytes = BASEWIDTH/2;	
	}
	
	vid.aspect = 1.0;
	vid.numpages = 1;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.buffer = vid.conbuffer = vita2d_texture_get_datap(tex_buffer);
	
	if (firststart){
	
		// Set correct palette for the texture
		VID_SetPalette(palette);
		
		// Init Quake Cache
		d_pzbuffer = zbuffer;
		surfcache = malloc(SURFCACHE_SIZE);
		D_InitCaches (surfcache, SURFCACHE_SIZE);
		firststart = 0;
		
	}else Cvar_SetValue ("gamma", v_gamma.value); // calls a palette reloading
	
}

void	VID_Shutdown (void)
{
	vita2d_free_texture(tex_buffer);
	vita2d_fini();
	free(surfcache);
}

void	VID_Update (vrect_t *rects)
{
	
	if (native_resolution != old_resolution) VID_Init(NULL);
	old_resolution = native_resolution;
	vita2d_start_drawing();
	if (native_resolution) vita2d_draw_texture(tex_buffer, 0, 0);
	else vita2d_draw_texture_scale(tex_buffer, 0, 0, 2.0, 2.0);
	if (isDanzeff) danzeff_render();
	vita2d_end_drawing();
	vita2d_wait_rendering_done();
	vita2d_swap_buffers();
	sceDisplayWaitVblankStart();
	
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}


/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect (int x, int y, int width, int height)
{
}
