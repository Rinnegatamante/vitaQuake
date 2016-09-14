#include "quakedef.h"
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vita2d.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>

#include <ctype.h>

#define MOD_FILE "mods.txt"

typedef struct ModsList{
	char name[256];
	struct ModsList* next;
}ModsList;

/*
================
CheckForPak
================
*/
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

char* modname = NULL;

void MOD_SelectModMenu(char *basedir){

	SceCtrlData pad, oldpad;
	sceCtrlPeekBufferPositive(0, &pad, 1);

	// Ch0wW: Enable the mod menu once the R trigger is hold on startup.
	if (!(pad.buttons & SCE_CTRL_RTRIGGER))
	{
		// Reading current used mod
		char cur_mod[64];
		modname = malloc(64);
		FILE* f;
		if ((f = fopen( va("%s/%s", basedir, MOD_FILE), "r")) != NULL) {
			char tmp[256];
			fseek(f, 0, SEEK_END);
			int len = ftell(f);
			fseek(f, 0, SEEK_SET);
			fread(tmp, 1, len, f);
			fclose(f);
			tmp[len] = 0;
			sprintf(modname, "%s", tmp);
		}
		else
			modname = NULL;
		return;
	}

	// Initializing empty ModList
	ModsList* mods = NULL;
	int i = 0;
	int max_idx = -1;
	
	// Scanning main folder in search of mods
	int dd = sceIoDopen(basedir);
	SceIoDirent entry;
	int res;
	while (sceIoDread(dd, &entry) > 0){
		if (SCE_S_ISDIR(entry.d_stat.st_mode)){
			if (CheckForMod( va("%s/%s", basedir, entry.d_name))){
				mods = addMod(entry.d_name, mods);
				max_idx++;
			}
		}
	}
	sceIoDclose(dd);
	
	// Reading current used mod
	char cur_mod[64];
	modname = malloc(64);
	FILE* f;
	if ((f = fopen(va("%s/%s", basedir, MOD_FILE), "r")) != NULL) {
		char tmp[256];
		fseek(f, 0, SEEK_END);
		int len = ftell(f);
		fseek(f, 0, SEEK_SET);
		fread(tmp, 1, len, f);
		fclose(f);
		tmp[len] = 0;
		sprintf(modname, "%s", tmp);
		sprintf(cur_mod, "Current in use mod: %s - Press START to launch vitaQuake core", tmp);
	}else strcpy(cur_mod,"Current in use mod: id1 - Press START to launch vitaQuake core");
	
	// Initializing graphics stuffs
	vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));
	vita2d_pgf* debug_font = vita2d_load_default_pgf();
	uint32_t white = RGBA8(0xFF, 0xFF, 0xFF, 0xFF);
	uint32_t green = RGBA8(0x00, 0xFF, 0x00, 0xFF);
	uint32_t red = RGBA8(0xFF, 0x00, 0x00, 0xFF);
	
	// Main loop
	while (max_idx >= 0){
		vita2d_start_drawing();
		vita2d_clear_screen();
		vita2d_pgf_draw_text(debug_font, 2, 20, red, 1.0, cur_mod);
		
		// Drawing menu
		int y = 40;
		int d = 0;
		ModsList* ptr = mods;
		while (ptr != NULL && y < 540){
			uint32_t color = white;
			if (d++ == i) color = green;
			vita2d_pgf_draw_text(debug_font, 2, y, color, 1.0, ptr->name);
			ptr = ptr->next;
			y += 20;
		}
		
		// Controls checking
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if ((pad.buttons & SCE_CTRL_CROSS) && (!(oldpad.buttons & SCE_CTRL_CROSS))){
			int z = 0;
			ModsList* tmp = mods;
			while (z < i){
				tmp = tmp->next;
				z++;
			}
			sprintf(cur_mod,"Current in use mod: %s - Press START to launch vitaQuake core", tmp->name);
			if (!strcmp(tmp->name, "id1"))
			{
				sceIoRemove( va("%s/%s", basedir, MOD_FILE) );
				modname = NULL;
			}
			else{
				f = fopen(va("%s/%s", basedir, MOD_FILE), "w");
				fwrite(tmp->name,1,strlen(tmp->name),f);
				fclose(f);
				strcpy(modname, tmp->name);	// Refresh the mod directory.
			}
		}else if ((pad.buttons & SCE_CTRL_UP) && (!(oldpad.buttons & SCE_CTRL_UP))){
			i--;
			if (i < 0) i = max_idx;
		}else if ((pad.buttons & SCE_CTRL_DOWN) && (!(oldpad.buttons & SCE_CTRL_DOWN))){
			i++;
			if (i > max_idx) i = 0;
		}else if (pad.buttons & SCE_CTRL_START) break;
		oldpad = pad;
		
		vita2d_end_drawing();
		vita2d_wait_rendering_done();
		vita2d_swap_buffers();
	}
	
	// Freeing stuffs
	ModsList* tmp = mods;
	ModsList* tmp2;
	while (tmp != NULL){
		tmp2 = tmp->next;
		free(tmp);
		tmp = tmp2;
	}

	return;
}