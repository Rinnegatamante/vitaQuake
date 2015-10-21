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
#include "errno.h"
#include <psp2/ctrl.h>
#include <psp2/types.h>
#include <psp2/rtc.h>
#include "draw_psp2.h"
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#define TICKS_PER_SEC 268123480.0

qboolean		isDedicated;

uint64_t initialTime = 0;
int y = 0;
/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES             10
FILE    *sys_handles[MAX_HANDLES];

int             findhandle (void)
{
	int             i;

	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int             pos;
	int             end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE    *f;
	int             i;

	i = findhandle ();

	f = fopen(path, "rb");
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;

	return filelength(f);
}

int Sys_FileOpenWrite (char *path)
{
	FILE    *f;
	int             i;

	i = findhandle ();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error ("Error opening %s: %s", path,strerror(errno));
	sys_handles[i] = f;

	return i;
}

void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite (int handle, void *data, int count)
{
	return fwrite (data, 1, count, sys_handles[handle]);
}

int     Sys_FileTime (char *path)
{
	FILE    *f;

	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}

	return -1;
}

void Sys_mkdir (char *path)
{
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
}


void Sys_Error (char *error, ...)
{

	font_draw_string(0, y, 0xFFFFFFFF, "SYS ERROR!");
	char buf[256];
	va_list argptr;
	y = y + 15;
	if (y > 500) y = 0;
	va_start (argptr,error);
	vsnprintf (buf, sizeof(buf), error,argptr);
	va_end (argptr);
	font_draw_string(0, y, 0xFFFFFFFF, buf);
	y = y + 15;
	if (y > 500) y = 0;
	font_draw_string(0, y, 0xFFFFFFFF, "Press START to exit");
	y = y + 15;
	if (y > 500) y = 0;
	while(1){
		int kDown;
		sceCtrlPeekBufferPositive(0, &kDown, 1);
		if (kDown & PSP2_CTRL_START)
			break;
	}
	Host_Shutdown();
	exit(1);
}

void Sys_Printf (char *fmt, ...)
{
	char buf[256];
	va_list argptr;

	va_start (argptr,fmt);
	vsnprintf (buf, sizeof(buf), fmt,argptr);
	va_end (argptr);
	font_draw_string(0, y, 0xFFFFFFFF, buf);
	y = y + 15;
	if (y > 500) y = 0;
	
}

void Sys_Quit (void)
{
	Host_Shutdown();
	end_video();
	exit (0);
}

double Sys_FloatTime (void)
{
	if(!initialTime){
		sceRtcGetCurrentTick(&initialTime);
	}
	uint64_t curTime;
	sceRtcGetCurrentTick(&curTime);
	return (curTime - initialTime)/TICKS_PER_SEC;
}

char *Sys_ConsoleInput (void)
{
	return NULL;
}

void Sys_Sleep (void)
{
}

void PSP2_KeyDown(int keys){
	if( keys & PSP2_CTRL_SELECT)
		Key_Event(K_ESCAPE, true);
	if( keys & PSP2_CTRL_START)
		Key_Event(K_ENTER, true);
	if( keys & PSP2_CTRL_UP)
		Key_Event(K_UPARROW, true);
	if( keys & PSP2_CTRL_DOWN)
		Key_Event(K_DOWNARROW, true);
	if( keys & PSP2_CTRL_LEFT)
		Key_Event(K_LEFTARROW, true);
	if( keys & PSP2_CTRL_RIGHT)
		Key_Event(K_RIGHTARROW, true);
	if( keys & PSP2_CTRL_SQUARE)
		Key_Event('y', true);
	if( keys & PSP2_CTRL_TRIANGLE)
		Key_Event('x', true);
	if( keys & PSP2_CTRL_CROSS)
		Key_Event('b', true);
	if( keys & PSP2_CTRL_CIRCLE)
		Key_Event('a', true);
	if( keys & PSP2_CTRL_LTRIGGER)
		Key_Event('l', true);
	if( keys & PSP2_CTRL_RTRIGGER)
		Key_Event('r', true);
}

void PSP2_KeyUp(int keys){
	if( keys & PSP2_CTRL_SELECT)
		Key_Event(K_ESCAPE, false);
	if( keys & PSP2_CTRL_START)
		Key_Event(K_ENTER, false);
	if( keys & PSP2_CTRL_UP)
		Key_Event(K_UPARROW, false);
	if( keys & PSP2_CTRL_DOWN)
		Key_Event(K_DOWNARROW, false);
	if( keys & PSP2_CTRL_LEFT)
		Key_Event(K_LEFTARROW, false);
	if( keys & PSP2_CTRL_RIGHT)
		Key_Event(K_RIGHTARROW, false);
	if( keys & PSP2_CTRL_SQUARE)
		Key_Event('y', false);
	if( keys & PSP2_CTRL_TRIANGLE)
		Key_Event('x', false);
	if( keys & PSP2_CTRL_CROSS)
		Key_Event('b', false);
	if( keys & PSP2_CTRL_CIRCLE)
		Key_Event('a', false);
	if( keys & PSP2_CTRL_LTRIGGER)
		Key_Event('l', false);
	if( keys & PSP2_CTRL_RTRIGGER)
		Key_Event('r', false);
}

void Sys_SendKeyEvents (void)
{
	int kDown;
	sceCtrlPeekBufferPositive(0, &kDown, 1);
	int kUp;
	sceCtrlPeekBufferNegative(0, &kUp, 1);
	if(kDown)
		PSP2_KeyDown(kDown);
	if(kUp)
		PSP2_KeyUp(kUp);
}

void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}

//=============================================================================

int main (int argc, char **argv)
{
	float		time, oldtime;
	init_video();
	static quakeparms_t    parms;

	parms.memsize = 16*1024*1024;
	parms.membase = malloc (parms.memsize);
	parms.basedir = ".";

	COM_InitArgv (argc, argv);

	parms.argc = com_argc;
	parms.argv = com_argv;
	Host_Init (&parms);
	//Sys_Init();
	oldtime = Sys_FloatTime() -0.1;
	while (1)
	{
		time = Sys_FloatTime();
		Host_Frame (time - oldtime);
		oldtime = time;
	}
	end_video();
	sceKernelExitProcess(0);
	return 0;
}
