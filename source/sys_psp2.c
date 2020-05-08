/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2020 Asakura Reiko

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
#include "net_dgrm.h"
#include <vitasdk.h>

#define BIGSTACK_SIZE 20 * 1024 * 1024
byte sys_bigstack[BIGSTACK_SIZE];
int sys_bigstack_cursize;

uint8_t is_uma0 = 0;
int tex_cache = 1;

extern int antialiasing;
extern uint8_t netcheck_dialog_running;
extern uint8_t proto_idx;
extern int gl_ssaa;

// Mods support
int max_mod_idx = -1;

ModsList* mods;

ModsList* addMod(char* name, ModsList* db){
	ModsList* entry = (ModsList*)malloc(sizeof(ModsList));
	strcpy(entry->name, name);
	if (db == NULL) return entry;
	else{
		ModsList* ptr = db;
		while (ptr->next != NULL){
			ptr = ptr->next;
		}
		ptr->next = entry;
		return db;
	}
}


int old_char;
extern int setup_cursor;
extern int lanConfig_cursor;
int isKeyboard;
extern uint64_t rumble_tick;
extern cvar_t psvita_touchmode;
extern cvar_t vid_vsync;
extern int scr_width;
extern int scr_height;
extern int cfg_width;
extern int cfg_height;

bool isDedicated;
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

int Sys_FindHandle(void)
{
	int i;

	for (i=1;i<MAX_HANDLES;i++)
		if (!sys_handles[i]) return i;
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
	FILE* f = NULL;
	if (is_uma0) f = fopen("uma0:/data/Quake/log.txt", "a+");
	else f = fopen("ux0:/data/Quake/log.txt", "a+");
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

void Sys_BigStackRewind(void)
{
	sys_bigstack_cursize = 0;
}

void* Sys_BigStackAlloc(int size, char* purpose)
{
	void* p;

	p = 0;
	if (sys_bigstack_cursize + size < BIGSTACK_SIZE)
	{
		p = sys_bigstack + sys_bigstack_cursize;
		sys_bigstack_cursize = sys_bigstack_cursize + size;
	}
	else
	{
		Sys_Error("Sys_BigStackAlloc: %s - failed on %i bytes", purpose, size);
	};
	return p;
}

void Sys_BigStackFree(int size, char* purpose)
{
	if (sys_bigstack_cursize - size >= 0)
	{
		sys_bigstack_cursize = sys_bigstack_cursize - size;
	}
	else
	{
		Sys_Error("Sys_BigStackFree: %s - underflow on %i bytes", purpose, sys_bigstack_cursize - size);
	};
}
// <<< FIX


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

void Sys_Error(const char *error, ...)
{

	va_list         argptr;

	char buf[256];
	va_start(argptr, error);
	vsnprintf(buf, sizeof(buf), error, argptr);
	va_end(argptr);
	sprintf(buf, "%s\n", buf);
	FILE* f = NULL;
	if (is_uma0) f = fopen("uma0:/data/Quake/log.txt", "a+");
	else f = fopen("ux0:/data/Quake/log.txt", "a+");
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
	SceRtcTick ticks;
	sceRtcGetCurrentTick(&ticks);
	return ticks.tick * 0.000001;
}

void Sys_HighFPPrecision(void)
{
}

void Sys_LowFPPrecision(void)
{
}

/*
===============================================================================

KEYS & INPUTS

===============================================================================
*/
typedef struct
{
	int	    button;
	int		key;
} psvita_buttons;

#define MAX_PSVITA_KEYS 12
psvita_buttons KeyTable[MAX_PSVITA_KEYS] =
{
	{ SCE_CTRL_SELECT, K_SELECT },
	{ SCE_CTRL_START, K_START },

	{ SCE_CTRL_UP, K_UPARROW },
	{ SCE_CTRL_DOWN, K_DOWNARROW},
	{ SCE_CTRL_LEFT, K_LEFTARROW },
	{ SCE_CTRL_RIGHT, K_RIGHTARROW },
	{ SCE_CTRL_LTRIGGER, K_LEFTTRIGGER },
	{ SCE_CTRL_RTRIGGER, K_RIGHTTRIGGER },

	{ SCE_CTRL_SQUARE, K_SQUARE },
	{ SCE_CTRL_TRIANGLE, K_TRIANGLE },
	{ SCE_CTRL_CROSS, K_CROSS },
	{ SCE_CTRL_CIRCLE, K_CIRCLE }
};

void PSP2_KeyDown(int keys) {
	int i;
	for (i = 0; i < MAX_PSVITA_KEYS; i++) {
		if (keys & KeyTable[i].button) {
				Key_Event(KeyTable[i].key, true);
		}
	}
}

void PSP2_KeyUp(int keys, int oldkeys) {
	int i;
	for (i = 0; i < MAX_PSVITA_KEYS; i++) {
		if ((!(keys & KeyTable[i].button)) && (oldkeys & KeyTable[i].button)) {
			Key_Event(KeyTable[i].key, false);
		}
	}
}

void Sys_SendKeyEvents(void)
{
	sceCtrlPeekBufferPositive(0, &pad, 1);
	int kDown = pad.buttons;
	int kUp = oldpad.buttons;

	if (kDown)
		PSP2_KeyDown(kDown);
	if (kUp != kDown)
		PSP2_KeyUp(kDown, kUp);

	if (psvita_touchmode.value == 0)
	{
		// Touchscreen support for game status showing
		SceTouchData touch;
		sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
		Key_Event(K_TOUCH, touch.reportNum > 0 ? true : false);
	}
	oldpad = pad;
}

//=============================================================================
int _newlib_heap_size_user = 192 * 1024 * 1024;
char* mod_path = NULL;
char mp_path[32];

static uint16_t title[SCE_IME_DIALOG_MAX_TITLE_LENGTH];
static uint16_t initial_text[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
static uint16_t input_text[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
char title_keyboard[256];

void ascii2utf(uint16_t* dst, char* src){
	if(!src || !dst)return;
	while(*src)*(dst++)=(*src++);
	*dst=0x00;
}

void utf2ascii(char* dst, uint16_t* src){
	if(!src || !dst)return;
	while(*src)*(dst++)=(*(src++))&0xFF;
	*dst=0x00;
}

void simulateKeyPress(char* text){
	
	//We first delete the current text
	int i;
	for (i=0;i<100;i++){
		Key_Event(K_BACKSPACE, true);
		Key_Event(K_BACKSPACE, false);
	}
	
	while (*text){
		Key_Event(*text, true);
		Key_Event(*text, false);
		text++;
	}
}

#define		MAXCMDLINE	256	// If changed, don't forget to change it in keys.c too!!
extern	char	key_lines[32][MAXCMDLINE];
extern	int		edit_line;

bool CheckForMod(char* dir)
{
	int dd = sceIoDopen(dir);
	SceIoDirent entry;
	int res;
	bool ret = false;

	while ((res = sceIoDread(dd, &entry)) > 0 && (!ret))
		if ( strstr(strtolower(entry.d_name),".pak") != NULL || !strcmp(strtolower(entry.d_name), "progs.dat") ) ret = true;	// Enable checks for progs.dat only mods
	
	sceIoDclose(dd);
	return ret;
}

int quake_main (unsigned int argc, void* argv){
	cl_entities = malloc(sizeof(entity_t) * MAX_EDICTS);
	cl_temp_entities = malloc(sizeof(entity_t) * MAX_TEMP_ENTITIES);
	cl_efrags = malloc(sizeof(efrag_t) * MAX_EFRAGS);

	// Checking for uma0 support
	FILE *f = fopen("uma0:/data/Quake/id1/pak0.pak", "rb");
	if (f) {
		fclose(f);
		is_uma0 = 1;
	}
	
	// Initializing stuffs
	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	sceSysmoduleLoadModule(SCE_SYSMODULE_PSPNET_ADHOC);
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, 1);
	sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
	SceCommonDialogConfigParam cmnDlgCfgParam;
	sceCommonDialogConfigParamInit(&cmnDlgCfgParam);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, (int *)&cmnDlgCfgParam.language);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, (int *)&cmnDlgCfgParam.enterButtonAssign);
	sceCommonDialogSetConfigParam(&cmnDlgCfgParam);

	const float tickRate = 1.0f / sceRtcGetTickResolution();
	static quakeparms_t    parms;
	
	// Loading resolution and MSAA mode from config files, those are not handled via Host cause Host_Init requires vitaGL to be working
	if (is_uma0) f = fopen("uma0:data/Quake/resolution.cfg", "rb");
	else f = fopen("ux0:data/Quake/resolution.cfg", "rb");
	if (f != NULL){
		fscanf(f, "%dx%d", &scr_width, &scr_height);
		fclose(f);
	}
	if (is_uma0) f = fopen("uma0:data/Quake/antialiasing.cfg", "rb");
	else f = fopen("ux0:data/Quake/antialiasing.cfg", "rb");
	if (f != NULL){
		fscanf(f, "%d", &antialiasing);
		fclose(f);
	}
	cfg_width = scr_width;
	cfg_height = scr_height;

	// Check framebuffer size is valid
	int scr_pitch = (scr_width + 63) & ~63;
	void *fb_base = memalign(64 * 4, scr_pitch * scr_height * 4);
	SceDisplayFrameBuf fb = {
		sizeof(fb),
		fb_base,
		scr_pitch,
		SCE_DISPLAY_PIXELFORMAT_A8B8G8R8,
		scr_width,
		scr_height
	};
	if (sceDisplaySetFrameBuf(&fb, SCE_DISPLAY_SETBUF_NEXTFRAME) != 0) {
		scr_width = cfg_width = 960;
		scr_height = cfg_height = 544;
	}
	free(fb_base);

	// Set framebuffer to blank to prevent garbage from showing on screen
	sceDisplaySetFrameBuf(NULL, SCE_DISPLAY_SETBUF_NEXTFRAME);

	// Initializing vitaGL
	switch (antialiasing) {
	case 1:
	case 5:
	case 6:
		vglInitExtended(0x1400000, scr_width, scr_height, 0x1000000, SCE_GXM_MULTISAMPLE_2X);
		break;
	case 2:
	case 7:
	case 8:
		vglInitExtended(0x1400000, scr_width, scr_height, 0x1000000, SCE_GXM_MULTISAMPLE_4X);
		break;
	default:
		vglInitExtended(0x1400000, scr_width, scr_height, 0x1000000, SCE_GXM_MULTISAMPLE_NONE);
		break;
	}
	vglUseVram(GL_TRUE);
	
	// Properly setting SSAA
	switch (antialiasing) {
	case 3:
	case 5:
	case 7:
		gl_ssaa = 2;
		break;
	case 4:
	case 6:
	case 8:
		gl_ssaa = 4;
		break;
	default:
		gl_ssaa = 1;
		break;
	}
	
	// Official mission packs support
	SceAppUtilAppEventParam eventParam;
	memset(&eventParam, 0, sizeof(SceAppUtilAppEventParam));
	sceAppUtilReceiveAppEvent(&eventParam);
	if (eventParam.type == 0x05) {
		char *int_argv[16];
		int_argv[0] = int_argv[2] = "";
		char buffer[2048];
		memset(buffer, 0, 2048);
		sceAppUtilAppEventParseLiveArea(&eventParam, buffer);
		if (strstr(buffer, "custom") != NULL) {
			memset(input_text, 0, (SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1) << 1);
			memset(initial_text, 0, (SCE_IME_DIALOG_MAX_TEXT_LENGTH) << 1);
			sprintf(title_keyboard, "Insert args list");
			ascii2utf(title, title_keyboard);
			SceImeDialogParam param;
			sceImeDialogParamInit(&param);
			param.supportedLanguages = 0x0001FFFF;
			param.languagesForced = SCE_TRUE;
			param.type = SCE_IME_TYPE_BASIC_LATIN;
			param.title = title;
			param.maxTextLength = SCE_IME_DIALOG_MAX_TEXT_LENGTH;
			param.initialText = initial_text;
			param.inputTextBuffer = input_text;
			sceImeDialogInit(&param);
			while (sceImeDialogGetStatus() != 2) {
				vglStartRendering();
				vglStopRenderingInit();
				vglUpdateCommonDialog();
				vglStopRenderingTerm();
			}
			SceCommonDialogStatus status = sceImeDialogGetStatus();
			SceImeDialogResult result;
			memset(&result, 0, sizeof(SceImeDialogResult));
			sceImeDialogGetResult(&result);
			if (result.button == SCE_IME_DIALOG_BUTTON_ENTER)
			{
				utf2ascii(title_keyboard, input_text);
				if (strlen(title_keyboard) > 1) {
					int int_argc = 2;
					int_argv[1] = title_keyboard;
					char *ptr = title_keyboard;
					for (;;) {
						char *space = strstr(ptr, " ");
						if (space == NULL) break;
						*space = 0;
						int_argv[int_argc++] = ptr = space + 1;
					}
					int_argv[int_argc++] = "";
					COM_InitArgv(int_argc, int_argv);
				} else COM_InitArgv(argc, argv);
			} else COM_InitArgv(argc, argv);
			sceImeDialogTerm();
		} else {
			int_argv[1] = buffer;
			COM_InitArgv(3, int_argv);
			sprintf(mp_path, "%s", &buffer[1]);
			mod_path = mp_path;
		}
	} else COM_InitArgv(argc, argv);
	
	if (is_uma0) parms.basedir = "uma0:/data/Quake";
	else parms.basedir = "ux0:/data/Quake";
	
	// Initializing empty ModList
	mods = NULL;
	int i = 0;
	int max_idx = -1;
	
	// Scanning main folder in search of mods
	int dd = sceIoDopen(parms.basedir);
	SceIoDirent entry;
	int res;
	while (sceIoDread(dd, &entry) > 0){
		if (SCE_S_ISDIR(entry.d_stat.st_mode)){
			if (CheckForMod(va("%s/%s", parms.basedir, entry.d_name))){
				mods = addMod(entry.d_name, mods);
				max_mod_idx++;
			}
		}
	}
	sceIoDclose(dd);
	
	// Bat files support
	if (COM_CheckParm("-bat"))
	{
		char *int_argv[16];
		int_argv[0] = "";
		int t = COM_CheckParm("-bat") + 1;
		if (t < com_argc) {
			char fname[128];
			sprintf(fname, "%s/%s.bat", parms.basedir, com_argv[t]);
			SceUID fd = sceIoOpen(fname, SCE_O_RDONLY, 0777);
			if (fd >= 0) {
				int size = sceIoLseek32(fd, 0, SCE_SEEK_END);
				char bat_args[2048];
				sceIoLseek32(fd, 0, SCE_SEEK_SET);
				sceIoRead(fd, bat_args, size);
				sceIoClose(fd);
				bat_args[size] = 0;
				int int_argc = 2;
				char *ptr = strstr(bat_args, "quake") + 6;
				int_argv[1] = ptr;
				for (;;) {
					char *space = strstr(ptr, " ");
					if (space == NULL) break;
					*space = 0;
					int_argv[int_argc++] = ptr = space + 1;
				}
				int_argv[int_argc++] = "";
				COM_InitArgv(int_argc, int_argv);
			}
		}
	}
	
	// Texture cache
	if (COM_CheckParm("-no_tex_cache"))
		tex_cache = 0;
	
	if (COM_CheckParm("-mem"))
	{
		int t = COM_CheckParm("-mem") + 1;
		if (t < com_argc)
			parms.memsize = atoi(com_argv[t]) * 1024 * 1024;
	} else
		parms.memsize = 64 * 1024 * 1024;
	
	parms.membase = malloc(parms.memsize);

	parms.argc = com_argc;
	parms.argv = com_argv;

	Host_Init(&parms);
	hostInitialized = 1;
	
	// Setting PSN Account if it's his first time
	if (!strcmp(cl_name.string, "player"))
	{
		char nickname[32];
		sceAppUtilSystemParamGetString(SCE_SYSTEM_PARAM_ID_USERNAME, nickname, SCE_SYSTEM_PARAM_USERNAME_MAXSIZE);

		static char cmd[256];
		sprintf(cmd, "_cl_name \"%s\"\n", nickname);
		Cbuf_AddText(cmd);
	}

	IN_ResetInputs();
	Cbuf_AddText("exec config.cfg\n");

	vglWaitVblankStart(vid_vsync.value);
	int old_vsync = vid_vsync.value;
	
	SceRtcTick lastTick;
	sceRtcGetCurrentTick(&lastTick);
	
	// Disabling all FPU exceptions traps on main thread
	sceKernelChangeThreadVfpException(0x0800009FU, 0x0);

	while (1)
	{
		// Changing V-Sync setting in realtime
		if (old_vsync != vid_vsync.value) {
			vglWaitVblankStart(vid_vsync.value);
			old_vsync = vid_vsync.value;
		}
		
		// Prevent screen power-off
		sceKernelPowerTick(0);

		// Rumble effect managing (PSTV only)
		if (rumble_tick != 0) {
			if (sceKernelGetProcessTimeWide() - rumble_tick > 500000) IN_StopRumble(); // 0.5 sec
		}
		
		// AdHoc Message Dialog
		if (netcheck_dialog_running == 1) {
			SceCommonDialogStatus status = sceNetCheckDialogGetStatus();
			if (status == 2) {
				SceNetCheckDialogResult result;
				memset(&result, 0, sizeof(SceNetCheckDialogResult));
				sceNetCheckDialogGetResult(&result);

				if (result.result == SCE_COMMON_DIALOG_RESULT_OK) {
					proto_idx = 1;
				} else {
					proto_idx = 0;
				}

				sceNetCheckDialogTerm();
				Datagram_Init();
			}
		}

		// OSK manage for Console / Input
		if (key_dest == key_console || m_state == m_lanconfig || m_state == m_setup)
		{
			if (old_char != 0) Key_Event(old_char, false);
			SceCtrlData tmp_pad, oldpad;
			sceCtrlPeekBufferPositive(0, &tmp_pad, 1);
			if (isKeyboard)
			{
				SceCommonDialogStatus status = sceImeDialogGetStatus();
				if (status == 2) {
					SceImeDialogResult result;
					memset(&result, 0, sizeof(SceImeDialogResult));
					sceImeDialogGetResult(&result);

					if (result.button == SCE_IME_DIALOG_BUTTON_ENTER)
					{
						if (key_dest == key_console)
						{
							utf2ascii(title_keyboard, input_text);
							strcpy(key_lines[edit_line] + 1, title_keyboard);
							Key_SendText(key_lines[edit_line] + 1);
						} else {
							utf2ascii(title_keyboard, input_text);
							simulateKeyPress(title_keyboard);
						}
					}

					sceImeDialogTerm();
					isKeyboard = false;
				}
			}
			else {
				if ((tmp_pad.buttons & SCE_CTRL_SELECT) && (!(oldpad.buttons & SCE_CTRL_SELECT)))
				{
					if ((m_state == m_setup && (setup_cursor == 0 || setup_cursor == 1)) || (key_dest == key_console) || (m_state == m_lanconfig && (lanConfig_cursor == 0 || lanConfig_cursor == 3)))
					{
						memset(input_text, 0, (SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1) << 1);
						memset(initial_text, 0, (SCE_IME_DIALOG_MAX_TEXT_LENGTH) << 1);
						if (key_dest == key_console) {
							sprintf(title_keyboard, "Insert Quake command");
						} else if (m_state == m_setup) {
							(setup_cursor == 0) ? sprintf(title_keyboard, "Insert hostname") : sprintf(title_keyboard, "Insert player name");
						} else if (m_state == m_lanconfig){
							(lanConfig_cursor == 0) ? sprintf(title_keyboard, "Insert port number") : sprintf(title_keyboard, "Insert server address");
						}
						ascii2utf(title, title_keyboard);
						isKeyboard = true;
						SceImeDialogParam param;
						sceImeDialogParamInit(&param);
						param.supportedLanguages = 0x0001FFFF;
						param.languagesForced = SCE_TRUE;
						param.type = (m_state == m_lanconfig && lanConfig_cursor == 0) ? SCE_IME_TYPE_NUMBER : SCE_IME_TYPE_BASIC_LATIN;
						param.title = title;
						param.maxTextLength = (m_state == m_lanconfig && lanConfig_cursor == 0) ? 5 : SCE_IME_DIALOG_MAX_TEXT_LENGTH;
						if (key_dest == key_console)
						{
							ascii2utf(initial_text, key_lines[edit_line] + 1);
							ascii2utf(input_text, key_lines[edit_line] + 1);
						}
						param.initialText = initial_text;
						param.inputTextBuffer = input_text;
						sceImeDialogInit(&param);
					}
				}
			}
			oldpad = tmp_pad;
		}

		// Get current frame
		SceRtcTick tick;
		sceRtcGetCurrentTick(&tick);
		const unsigned int deltaTick = tick.tick - lastTick.tick;
		const float   deltaSecond = deltaTick * tickRate;

		// Show frame
		Host_Frame(deltaSecond);
		lastTick.tick = tick.tick;

	}

	vglEnd();
	return 0;
}

int main(int argc, char **argv)
{
	// We need a bigger stack to run Quake, so we create a new thread with a proper stack size
	SceUID main_thread = sceKernelCreateThread("Quake", quake_main, 0x40, 0x800000, 0, 0, NULL);
	if (main_thread >= 0){
		sceKernelStartThread(main_thread, 0, NULL);
		sceKernelWaitThreadEnd(main_thread, NULL, NULL);
	}
	return 0;
}
