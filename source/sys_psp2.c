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

#include "quakedef.h"
#include "errno.h"
#include "danzeff.h"
#include <psp2/types.h>
#include <psp2/rtc.h>
#include <psp2/sysmodule.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/touch.h>
#include <psp2/system_param.h>
#include <psp2/apputil.h>

#define u64 uint64_t

extern int old_char;
extern int isDanzeff;
extern uint64_t rumble_tick;
extern cvar_t res_val;
bool		isDedicated;

uint64_t initialTime = 0;
int hostInitialized = 0;
SceCtrlData pad, oldpad;
/*
===============================================================================

FILE IO

===============================================================================
*/

#define PLATFORM_PSVITA	0x00010000

#define MAX_HANDLES             10
FILE    *sys_handles[MAX_HANDLES];

int             Sys_FindHandle (void)
{
	int             i;

	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

void Log (const char *format, ...){
	#ifdef DEBUG
	__gnuc_va_list arg;
	int done;
	va_start (arg, format);
	char msg[512];
	done = vsprintf (msg, format, arg);
	va_end (arg);
	int i;
	sprintf(msg,"%s\n",msg);
	FILE* log = fopen("ux0:/data/Quake/log.txt","a+");
	if (log != NULL){
		fwrite(msg,1,strlen(msg),log);
		fclose(log);
	}
	#endif
}

int Sys_FileLength (FILE *f)
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

	i = Sys_FindHandle ();

	f = fopen(path, "rb");
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;

	return Sys_FileLength(f);
}

int Sys_FileOpenWrite (char *path)
{
	FILE    *f;
	int             i;

	i = Sys_FindHandle ();

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
	sceIoMkdir(path, 0777);
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
}

void Sys_Quit (void)
{
	Host_Shutdown();
	sceKernelExitProcess(0);
}

void Sys_Error (char *error, ...)
{

	va_list         argptr;
	
	char buf[256];
	va_start (argptr, error);
	vsnprintf (buf, sizeof(buf), error,argptr);
	va_end (argptr);
	sprintf(buf,"%s\n",buf);
	FILE* f = fopen("ux0:/data/Quake/log.txt","a+");
	fwrite(buf,1,strlen(buf),f);
	fclose(f);
	Sys_Quit();
}

void Sys_Printf (char *fmt, ...)
{
	#ifdef DEBUG
	if(hostInitialized)
		return;

	va_list argptr;
	char buf[256];
	va_start (argptr,fmt);
	vsnprintf (buf, sizeof(buf), fmt,argptr);
	va_end (argptr);
	Log(buf);
	#endif
	
}

char *Sys_ConsoleInput (void)
{
	return NULL;
}

void Sys_Sleep (void)
{
}

double Sys_FloatTime (void)
{
	u64 ticks;
	sceRtcGetCurrentTick(&ticks);
	return ticks * 0.000001;

}

void PSP2_KeyDown(int keys){
	if (!isDanzeff){
		if( keys & SCE_CTRL_SELECT)
			Key_Event(K_ESCAPE, true);
		if( keys & SCE_CTRL_START)
			Key_Event(K_ENTER, true);
		if( keys & SCE_CTRL_UP)
			Key_Event(K_UPARROW, true);
		if( keys & SCE_CTRL_DOWN)
			Key_Event(K_DOWNARROW, true);
		if( keys & SCE_CTRL_LEFT)
			Key_Event(K_LEFTARROW, true);
		if( keys & SCE_CTRL_RIGHT)
			Key_Event(K_RIGHTARROW, true);
		if( keys & SCE_CTRL_SQUARE)
			Key_Event(K_AUX2, true);
		if( keys & SCE_CTRL_TRIANGLE)
			Key_Event(K_AUX3, true);
		if( keys & SCE_CTRL_CROSS)
			Key_Event(K_AUX1, true);
		if( keys & SCE_CTRL_CIRCLE)
			Key_Event(K_AUX4, true);
		if( keys & SCE_CTRL_LTRIGGER)
			Key_Event(K_AUX5, true);
		if( keys & SCE_CTRL_RTRIGGER)
			Key_Event(K_AUX6, true);
	}
}

void PSP2_KeyUp(int keys, int oldkeys){
	if (!isDanzeff){
		if ((!(keys & SCE_CTRL_SELECT)) && (oldkeys & SCE_CTRL_SELECT))
			Key_Event(K_ESCAPE, false);
		if ((!(keys & SCE_CTRL_START)) && (oldkeys & SCE_CTRL_START))
			Key_Event(K_ENTER, false);
		if ((!(keys & SCE_CTRL_UP)) && (oldkeys & SCE_CTRL_UP))
			Key_Event(K_UPARROW, false);
		if ((!(keys & SCE_CTRL_DOWN)) && (oldkeys & SCE_CTRL_DOWN))
			Key_Event(K_DOWNARROW, false);
		if ((!(keys & SCE_CTRL_LEFT)) && (oldkeys & SCE_CTRL_LEFT))
			Key_Event(K_LEFTARROW, false);
		if ((!(keys & SCE_CTRL_RIGHT)) && (oldkeys & SCE_CTRL_RIGHT))
			Key_Event(K_RIGHTARROW, false);
		if ((!(keys & SCE_CTRL_SQUARE)) && (oldkeys & SCE_CTRL_SQUARE))
			Key_Event(K_AUX2, false);
		if ((!(keys & SCE_CTRL_TRIANGLE)) && (oldkeys & SCE_CTRL_TRIANGLE))
			Key_Event(K_AUX3, false);
		if ((!(keys & SCE_CTRL_CROSS)) && (oldkeys & SCE_CTRL_CROSS))
			Key_Event(K_AUX1, false);
		if ((!(keys & SCE_CTRL_CIRCLE)) && (oldkeys & SCE_CTRL_CIRCLE))
			Key_Event(K_AUX4, false);
		if ((!(keys & SCE_CTRL_LTRIGGER)) && (oldkeys & SCE_CTRL_LTRIGGER))
			Key_Event(K_AUX5, false);
		if ((!(keys & SCE_CTRL_RTRIGGER)) && (oldkeys & SCE_CTRL_RTRIGGER))
			Key_Event(K_AUX6, false);
	}
}

void Sys_SendKeyEvents (void)
{
	if (key_dest != key_console){
		sceCtrlPeekBufferPositive(0, &pad, 1);
		int kDown = pad.buttons;
		int kUp = oldpad.buttons;
		if(kDown)
			PSP2_KeyDown(kDown);
		if(kUp != kDown)
			PSP2_KeyUp(kDown, kUp);
			
		// Touchscreen support for game status showing
		SceTouchData touch;
		sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
		if (touch.reportNum > 0) Key_Event(K_TOUCH, true);
		else Key_Event(K_TOUCH, false);
		
		oldpad = pad;
	}
}

void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}

extern cvar_t platform;
//=============================================================================
int _newlib_heap_size_user = 192 * 1024 * 1024;

int main (int argc, char **argv)
{

	// Initializing stuffs
	scePowerSetArmClockFrequency(444);
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, 1);
	
	const float tickRate = 1.0f / sceRtcGetTickResolution();
	static quakeparms_t    parms;

	parms.memsize = 20*1024*1024;
	parms.membase = malloc (parms.memsize);
	parms.basedir = "ux0:/data/Quake";
	
	// Mods support
	char* mod_path = NULL;
	FILE* f;
	if ((f = fopen("ux0:/data/Quake/mods.txt","r")) != NULL){
		int int_argc = 3;
		char* mod_path = malloc(256);
		fread(mod_path, 1, 256, f);
		fclose(f);
		char* int_argv[] = {"", "-game", mod_path};
		COM_InitArgv(3,int_argv);
	}else COM_InitArgv(argc,argv);
	
	parms.argc = com_argc;
	parms.argv = com_argv;	
	
	Host_Init (&parms);
	hostInitialized = 1;
	//Sys_Init();
	
	// Setting player name to PSVITA nickname
	char nickname[32];
	SceAppUtilInitParam init_param;
	SceAppUtilBootParam boot_param;
	memset(&init_param, 0, sizeof(SceAppUtilInitParam));
	memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&init_param, &boot_param);
	sceAppUtilSystemParamGetString(SCE_SYSTEM_PARAM_ID_USERNAME, nickname, SCE_SYSTEM_PARAM_USERNAME_MAXSIZE);
	static char cmd[256];
	sprintf(cmd,"_cl_name \"%s\"\n",nickname);
	Cbuf_AddText (cmd);
	
	// Set default PSVITA controls
	Cbuf_AddText ("unbindall\n");
	Cbuf_AddText ("bind CROSS +jump\n"); // Cross
	Cbuf_AddText ("bind SQUARE +attack\n"); // Square
	Cbuf_AddText ("bind CIRCLE +jump\n"); // Circle
	Cbuf_AddText ("bind TRIANGLE \"impulse 10\"\n"); // Triangle
	Cbuf_AddText ("bind LTRIGGER +speed\n"); // Left Trigger
	Cbuf_AddText ("bind RTRIGGER +attack\n"); // Right Trigger
	Cbuf_AddText ("bind UPARROW +moveup\n"); // Up
	Cbuf_AddText ("bind DOWNARROW +movedown\n"); // Down
	Cbuf_AddText ("bind LEFTARROW +moveleft\n"); // Left
	Cbuf_AddText ("bind RIGHTARROW +moveright\n"); // Right
	Cbuf_AddText ("bind TOUCH +showscores\n"); // Touchscreen
	Cbuf_AddText ("sensitivity 5\n"); // Right Analog Sensitivity
	
	Cbuf_AddText ("exec config.cfg\n");

#if 0
	if ( sceKernelGetModelForCDialog() == PLATFORM_PSVITA) // Ch0wW: SOMEONE HEEEELP ME :c
	{
		Cvar_ForceSet("platform", "2");
	}
#endif
	// Just to be sure to use the correct resolution in config.cfg
	VID_ChangeRes(res_val.value);
	
	u64 lastTick;
	sceRtcGetCurrentTick(&lastTick);

		// Check the current system	
	while (1)
	{
		// Prevent screen power-off
		sceKernelPowerTick(0);
		
		// Rumble effect managing (PSTV only)
		if (rumble_tick != 0){
			if (sceKernelGetProcessTimeWide() - rumble_tick > 500000) IN_StopRumble(); // 0.5 sec
		}
		
		// Danzeff keyboard manage for Console / Input
		if (key_dest == key_console || m_state == m_lanconfig){
			if (old_char != 0) Key_Event(old_char, false);
			SceCtrlData danzeff_pad, oldpad;
			sceCtrlPeekBufferPositive(0, &danzeff_pad, 1);
			if (isDanzeff){
				int new_char = danzeff_readInput(danzeff_pad);
				if (new_char != 0){
					if (new_char == DANZEFF_START){
						Key_Console(K_END);
					}else if (new_char == DANZEFF_LEFT){
						Key_Event(K_UPARROW, true);
						old_char = K_UPARROW;
					}else if (new_char == '\n'){
						Key_Event(K_DOWNARROW, true);
						old_char = K_DOWNARROW;
					}else if (new_char == 8){
						Key_Event(K_BACKSPACE, true);
						old_char = K_BACKSPACE;
					}else if (new_char == DANZEFF_RIGHT){
						Key_Event(K_TAB, true);
						old_char = K_TAB;
					}else if (new_char == DANZEFF_SELECT && (!(oldpad.buttons & SCE_CTRL_SELECT))){
						if (m_state == m_lanconfig) danzeff_free();
						isDanzeff = false;
					}else{
						Key_Event(new_char, true);
						old_char = new_char;
					}
				}
			}else if ((danzeff_pad.buttons & SCE_CTRL_START) && (!(oldpad.buttons & SCE_CTRL_START))){
				if (key_dest == key_console){
					danzeff_free();
					Con_ToggleConsole_f ();
				}
			}else if ((danzeff_pad.buttons & SCE_CTRL_SELECT) && (!(oldpad.buttons & SCE_CTRL_SELECT))){
				if (m_state == m_lanconfig) danzeff_load();
				isDanzeff = true;
			}
			oldpad = danzeff_pad;
		}
		
		// Get current frame
		u64 tick;
		sceRtcGetCurrentTick(&tick);
		const unsigned int deltaTick  = tick - lastTick;
		const float   deltaSecond = deltaTick * tickRate;
		
		// Show frame
		Host_Frame(deltaSecond);
		lastTick = tick;
		
#if 0	
		if (platform.value == 2)
			Con_Printf("HOLD ON, YOU'RE ON A VITA?\n");
#endif
	}
	
	free(parms.membase);
	if (mod_path != NULL) free(mod_path);
	sceKernelExitProcess(0);
	return 0; 
}
