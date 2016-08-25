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
#include <psp2/types.h>
#include <psp2/rtc.h>
#include <psp2/common_dialog.h>
#include <psp2/ime_dialog.h>
#include <psp2/sysmodule.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/touch.h>
#include <psp2/system_param.h>
#include <psp2/apputil.h>
#include <psp2/ctrl.h>
#define u64 uint64_t

extern int old_char;
extern int setup_cursor;
extern int lanConfig_cursor;
extern int isKeyboard;
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

#define MAX_HANDLES             10
FILE    *sys_handles[MAX_HANDLES];

int             Sys_FindHandle(void)
{
	int             i;

	for (i = 1; i<MAX_HANDLES; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error("out of handles");
	return -1;
}

void Log(const char *format, ...) {
#ifdef DEBUG
	__gnuc_va_list arg;
	int done;
	va_start(arg, format);
	char msg[512];
	done = vsprintf(msg, format, arg);
	va_end(arg);
	int i;
	sprintf(msg, "%s\n", msg);
	FILE* log = fopen("ux0:/data/Quake/log.txt", "a+");
	if (log != NULL) {
		fwrite(msg, 1, strlen(msg), log);
		fclose(log);
	}
#endif
}

int Sys_FileLength(FILE *f)
{
	int             pos;
	int             end;

	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead(char *path, int *hndl)
{
	FILE    *f;
	int             i;

	i = Sys_FindHandle();

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

int Sys_FileOpenWrite(char *path)
{
	FILE    *f;
	int             i;

	i = Sys_FindHandle();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error("Error opening %s: %s", path, strerror(errno));
	sys_handles[i] = f;

	return i;
}

void Sys_FileClose(int handle)
{
	fclose(sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek(int handle, int position)
{
	fseek(sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead(int handle, void *dest, int count)
{
	return fread(dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite(int handle, void *data, int count)
{
	return fwrite(data, 1, count, sys_handles[handle]);
}

int     Sys_FileTime(char *path)
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

void Sys_mkdir(char *path)
{
	sceIoMkdir(path, 0777);
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length)
{
}

void Sys_Quit(void)
{
	Host_Shutdown();
	sceKernelExitProcess(0);
}

void Sys_Error(char *error, ...)
{

	va_list         argptr;

	char buf[256];
	va_start(argptr, error);
	vsnprintf(buf, sizeof(buf), error, argptr);
	va_end(argptr);
	sprintf(buf, "%s\n", buf);
	FILE* f = fopen("ux0:/data/Quake/log.txt", "a+");
	fwrite(buf, 1, strlen(buf), f);
	fclose(f);
	Sys_Quit();
}

void Sys_Printf(char *fmt, ...)
{
#ifdef DEBUG
	if (hostInitialized)
		return;

	va_list argptr;
	char buf[256];
	va_start(argptr, fmt);
	vsnprintf(buf, sizeof(buf), fmt, argptr);
	va_end(argptr);
	Log(buf);
#endif

}

char *Sys_ConsoleInput(void)
{
	return NULL;
}

void Sys_Sleep(void)
{
}

double Sys_FloatTime(void)
{
	u64 ticks;
	sceRtcGetCurrentTick(&ticks);
	return ticks * 0.000001;

}

void PSP2_KeyDown(int keys) {
	if (!isKeyboard) {
		if (keys & SCE_CTRL_SELECT)
			Key_Event(K_ESCAPE, true);
		if (keys & SCE_CTRL_START)
			Key_Event(K_ENTER, true);
		if (keys & SCE_CTRL_UP)
			Key_Event(K_UPARROW, true);
		if (keys & SCE_CTRL_DOWN)
			Key_Event(K_DOWNARROW, true);
		if (keys & SCE_CTRL_LEFT)
			Key_Event(K_LEFTARROW, true);
		if (keys & SCE_CTRL_RIGHT)
			Key_Event(K_RIGHTARROW, true);
		if (keys & SCE_CTRL_SQUARE)
			Key_Event(K_AUX2, true);
		if (keys & SCE_CTRL_TRIANGLE)
			Key_Event(K_AUX3, true);
		if (keys & SCE_CTRL_CROSS)
			Key_Event(K_AUX1, true);
		if (keys & SCE_CTRL_CIRCLE)
			Key_Event(K_AUX4, true);
		if (keys & SCE_CTRL_LTRIGGER)
			Key_Event(K_AUX5, true);
		if (keys & SCE_CTRL_RTRIGGER)
			Key_Event(K_AUX6, true);
	}
}

void PSP2_KeyUp(int keys, int oldkeys) {
	if (!isKeyboard) {
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

void Sys_SendKeyEvents(void)
{
	if (key_dest != key_console) {
		sceCtrlPeekBufferPositive(0, &pad, 1);
		int kDown = pad.buttons;
		int kUp = oldpad.buttons;
		if (kDown)
			PSP2_KeyDown(kDown);
		if (kUp != kDown)
			PSP2_KeyUp(kDown, kUp);

		// Touchscreen support for game status showing
		SceTouchData touch;
		sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
		if (touch.reportNum > 0) Key_Event(K_TOUCH, true);
		else Key_Event(K_TOUCH, false);

		oldpad = pad;
	}
}

void Sys_HighFPPrecision(void)
{
}

void Sys_LowFPPrecision(void)
{
}

//=============================================================================
int _newlib_heap_size_user = 192 * 1024 * 1024;
char* mod_path = NULL;

static uint16_t title[SCE_IME_DIALOG_MAX_TITLE_LENGTH];
static uint16_t initial_text[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
static uint16_t input_text[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
char title_keyboard[256];

void ascii2utf(uint16_t* dst, char* src) {
	if (!src || !dst)return;
	while (*src)*(dst++) = (*src++);
	*dst = 0x00;
}

void utf2ascii(char* dst, uint16_t* src) {
	if (!src || !dst)return;
	while (*src)*(dst++) = (*(src++)) & 0xFF;
	*dst = 0x00;
}

void simulateKeyPress(char* text) {

	//We first delete the current text
	int i;
	for (i = 0; i<100; i++) {
		Key_Event(K_BACKSPACE, true);
		Key_Event(K_BACKSPACE, false);
	}

	while (*text) {
		Key_Event(*text, true);
		Key_Event(*text, false);
		text++;
	}
}

int main(int argc, char **argv)
{

	// Initializing stuffs
	scePowerSetArmClockFrequency(444);
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, 1);
	sceAppUtilInit(&(SceAppUtilInitParam) {}, &(SceAppUtilBootParam){});
	sceCommonDialogSetConfigParam(&(SceCommonDialogConfigParam) {});

	const float tickRate = 1.0f / sceRtcGetTickResolution();
	static quakeparms_t    parms;

	parms.memsize = 20 * 1024 * 1024;
	parms.membase = malloc(parms.memsize);
	parms.basedir = "ux0:/data/Quake";

	// Mods support
	FILE* f;
	if ((f = fopen("ux0:/data/Quake/mods.txt", "r")) != NULL) {
		int int_argc = 3;
		char* mod_path = malloc(256);
		fread(mod_path, 1, 256, f);
		fclose(f);
		char* int_argv[] = { "", "-game", mod_path };
		COM_InitArgv(3, int_argv);
	}
	else COM_InitArgv(argc, argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

	Host_Init(&parms);
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
	sprintf(cmd, "_cl_name \"%s\"\n", nickname);
	Cbuf_AddText(cmd);

	// Set default PSVITA controls
	Cbuf_AddText("unbindall\n");
	Cbuf_AddText("bind CROSS +jump\n"); // Cross
	Cbuf_AddText("bind SQUARE +attack\n"); // Square
	Cbuf_AddText("bind CIRCLE +jump\n"); // Circle
	Cbuf_AddText("bind TRIANGLE \"impulse 10\"\n"); // Triangle
	Cbuf_AddText("bind LTRIGGER +speed\n"); // Left Trigger
	Cbuf_AddText("bind RTRIGGER +attack\n"); // Right Trigger
	Cbuf_AddText("bind UPARROW +moveup\n"); // Up
	Cbuf_AddText("bind DOWNARROW +movedown\n"); // Down
	Cbuf_AddText("bind LEFTARROW +moveleft\n"); // Left
	Cbuf_AddText("bind RIGHTARROW +moveright\n"); // Right
	Cbuf_AddText("bind TOUCH +showscores\n"); // Touchscreen
	Cbuf_AddText("sensitivity 5\n"); // Right Analog Sensitivity

									 // Loading default config file
	Cbuf_AddText("exec config.cfg\n");

	// Just to be sure to use the correct resolution in config.cfg
	VID_ChangeRes(res_val.value);

	u64 lastTick;
	sceRtcGetCurrentTick(&lastTick);

	while (1)
	{
		// Prevent screen power-off
		sceKernelPowerTick(0);

		// Rumble effect managing (PSTV only)
		if (rumble_tick != 0) {
			if (sceKernelGetProcessTimeWide() - rumble_tick > 500000) IN_StopRumble(); // 0.5 sec
		}

		// OSK manage for Console / Input
		if (key_dest == key_console || m_state == m_lanconfig || m_state == m_setup) {
			if (old_char != 0) Key_Event(old_char, false);
			SceCtrlData tmp_pad, oldpad;
			sceCtrlPeekBufferPositive(0, &tmp_pad, 1);
			if (isKeyboard) {
				SceCommonDialogStatus status = sceImeDialogGetStatus();
				if (status == 2) {
					SceImeDialogResult result;
					memset(&result, 0, sizeof(SceImeDialogResult));
					sceImeDialogGetResult(&result);

					if (result.button != SCE_IME_DIALOG_BUTTON_CLOSE) {
						if (key_dest == key_console) {
							utf2ascii(title_keyboard, input_text);
							sprintf(title_keyboard, "%s\n", title_keyboard);
							Cbuf_AddText(title_keyboard);
						}
						else {
							utf2ascii(title_keyboard, input_text);
							simulateKeyPress(title_keyboard);
						}
					}

					sceImeDialogTerm();
					isKeyboard = false;
				}
			}
			else {
				if ((tmp_pad.buttons & SCE_CTRL_START) && (!(oldpad.buttons & SCE_CTRL_START))) {
					if (key_dest == key_console) Con_ToggleConsole_f();
				}
				else if ((tmp_pad.buttons & SCE_CTRL_SELECT) && (!(oldpad.buttons & SCE_CTRL_SELECT))) {
					if ((m_state == m_setup && (setup_cursor == 0 || setup_cursor == 1)) || (key_dest == key_console) || (m_state == m_lanconfig && (lanConfig_cursor == 1 || lanConfig_cursor == 3))) {
						memset(input_text, 0, (SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1) << 1);
						memset(initial_text, 0, (SCE_IME_DIALOG_MAX_TEXT_LENGTH) << 1);
						if (key_dest == key_console) {
							sprintf(title_keyboard, "Insert Quake command");
						}
						else if (m_state == m_setup) {
							(setup_cursor == 0) ? sprintf(title_keyboard, "Insert hostname") : sprintf(title_keyboard, "Insert player name");
						}
						else if (m_state == m_lanconfig) {
							(lanConfig_cursor == 1) ? sprintf(title_keyboard, "Insert port number") : sprintf(title_keyboard, "Insert server address");
						}
						ascii2utf(title, title_keyboard);
						isKeyboard = true;
						SceImeDialogParam param;
						sceImeDialogParamInit(&param);
						param.supportedLanguages = 0x0001FFFF;
						param.languagesForced = SCE_TRUE;
						param.type = (m_state == m_lanconfig && lanConfig_cursor == 1) ? SCE_IME_TYPE_NUMBER : SCE_IME_TYPE_BASIC_LATIN;
						param.title = title;
						param.maxTextLength = SCE_IME_DIALOG_MAX_TEXT_LENGTH;
						param.initialText = initial_text;
						param.inputTextBuffer = input_text;
						sceImeDialogInit(&param);
					}
				}
			}
			oldpad = tmp_pad;
		}

		// Get current frame
		u64 tick;
		sceRtcGetCurrentTick(&tick);
		const unsigned int deltaTick = tick - lastTick;
		const float   deltaSecond = deltaTick * tickRate;

		// Show frame
		Host_Frame(deltaSecond);
		lastTick = tick;

	}

	free(parms.membase);
	if (mod_path != NULL) free(mod_path);
	sceKernelExitProcess(0);
	return 0;
}
