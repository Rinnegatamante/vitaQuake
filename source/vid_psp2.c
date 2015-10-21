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
#include "quakedef.h"
#include "d_local.h"
#include "draw_psp2.h"
#define u16 uint16_t
#define u8 uint8_t
#define RGB565(r,g,b)  (((b)&0x1f)|(((g)&0x3f)<<5)|(((r)&0x1f)<<11))
#define RGB8_to_565(r,g,b)  (((b)>>3)&0x1f)|((((g)>>2)&0x3f)<<5)|((((r)>>3)&0x1f)<<11)

viddef_t	vid;				// global video state

#define	BASEWIDTH	960
#define	BASEHEIGHT	544

byte	vid_buffer[BASEWIDTH*BASEHEIGHT];
short	zbuffer[BASEWIDTH*BASEHEIGHT];
byte	surfcache[1024*1024];

u16	d_8to16table[256];
uint32_t palette_tbl[256];

void	VID_SetPalette (unsigned char *palette)
{
	int i;
	u8 *pal = palette;
	u16 *table = d_8to16table;
	unsigned r, g, b;
	for(i=0; i<256; i++){
		r = pal[0];
		g = pal[1];
		b = pal[2];
		palette_tbl[i] = r | (g << 8) | (b << 16);
		pal += 3;
	}
}

void	VID_ShiftPalette (unsigned char *palette)
{
}

void	VID_Init (unsigned char *palette)
{
	vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
	vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
	vid.aspect = 1.0;
	vid.numpages = 1;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.buffer = vid.conbuffer = vid_buffer;
	vid.rowbytes = vid.conrowbytes = BASEWIDTH;
	VID_SetPalette(palette);
	d_pzbuffer = zbuffer;
	D_InitCaches (surfcache, sizeof(surfcache));
}

void	VID_Shutdown (void)
{
}

void	VID_Update (vrect_t *rects)
{
	int x,y;
	for(x=0; x<BASEWIDTH; x++){
		for(y=0; y<BASEHEIGHT;y++){
			draw_pixel(x, y, palette_tbl[vid.buffer[y*BASEWIDTH + x]]);
		}
	}
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
