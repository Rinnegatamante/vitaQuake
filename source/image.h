/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers

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

#ifndef IMAGE_H
#define IMAGE_H

//image.h -- image reading / writing

//be sure to free the hunk after using these loading functions
byte *Image_LoadTGA (FILE *f, int *width, int *height);
byte *Image_LoadPCX (FILE *f, int *width, int *height);
byte *Image_LoadImage (const char *name, int *width, int *height);

#endif	/* IMAGE_H */


