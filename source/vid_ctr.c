/*
Copyright (C) 2015 Felipe Izzo
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

#include "quakedef.h"
#include "d_local.h"

#include <3ds.h>
#include "ctr.h"

viddef_t	vid;				// global video state

int	basewidth, baseheight, offset;

byte	*vid_buffer;
short	*zbuffer;
byte	surfcache[256*1024];
u16* fb;

u16	d_8to16table[256];

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
		table[0] = RGB8_to_565(r,g,b);
		table++;
		pal += 3;
	}
}

void	VID_ShiftPalette (unsigned char *palette)
{
}

void	VID_Init (unsigned char *palette)
{
	baseheight = 240;

	if(isN3DS){
		basewidth = 400;
		offset = 0;
	}

	else{
		basewidth = 320;
		offset = 40;
	}

	vid_buffer = malloc(sizeof(byte) * basewidth * baseheight);
	zbuffer = malloc(sizeof(short) * basewidth * baseheight);

	vid.maxwarpwidth = vid.width = vid.conwidth = basewidth;
	vid.maxwarpheight = vid.height = vid.conheight = baseheight;
	vid.aspect = 1.0;
	vid.numpages = 1;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.buffer = vid.conbuffer = vid_buffer;
	vid.rowbytes = vid.conrowbytes = basewidth;
	VID_SetPalette(palette);
	d_pzbuffer = zbuffer;
	D_InitCaches (surfcache, sizeof(surfcache));
	fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
}

void	VID_Shutdown (void)
{
}

void	VID_Update (vrect_t *rects)
{
	int x,y;
	for(x=0; x<basewidth; x++){
		for(y=0; y<baseheight;y++){
			fb[((offset + x)*240 + (239 -y))] = d_8to16table[vid.buffer[y*basewidth + x]];
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
