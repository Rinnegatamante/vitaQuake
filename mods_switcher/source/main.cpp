#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vita2d.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>

struct ModsList{
	char name[256];
	ModsList* next;
};

bool CheckForPak(char* dir){
	int dd = sceIoDopen(dir);
	SceIoDirent entry;
	int res;
	bool ret = false;
	while ((res = sceIoDread(dd, &entry)) > 0 && (!ret)){
		int len = strlen(entry.d_name);
		if (strstr(entry.d_name,".pak") != NULL) ret = true;
		if (strstr(entry.d_name,".PAK") != NULL) ret = true;
	}
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

int main(){

	// Initializing empty ModList
	ModsList* mods = NULL;
	int i = 0;
	
	// Scanning main folder in search of mods
	int dd = sceIoDopen("ux0:/data/Quake");
	SceIoDirent entry;
	int res;
	int max_idx = -1;
	while (sceIoDread(dd, &entry) > 0){
		if (SCE_S_ISDIR(entry.d_stat.st_mode)){
			char fullpath[256];
			sprintf(fullpath,"ux0:/data/Quake/%s",entry.d_name);
			if (CheckForPak(fullpath)){
				mods = addMod(entry.d_name, mods);
				max_idx++;
			}
		}
	}
	sceIoDclose(dd);
	
	// Reading current used mod
	char cur_mod[256];
	FILE* f;
	if ((f = fopen("ux0:/data/Quake/mods.txt", "r")) != NULL){
		char tmp[256];
		fseek(f, 0, SEEK_END);
		int len = ftell(f);
		fseek(f, 0, SEEK_SET);
		fread(tmp, 1, len, f);
		fclose(f);
		tmp[len] = 0;
		sprintf(cur_mod, "Current in use mod: %s", tmp);
	}else strcpy(cur_mod,"Current in use mod: id1");
	
	// Initializing graphics stuffs
	vita2d_init();
	vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));
	vita2d_pgf* debug_font = vita2d_load_default_pgf();
	uint32_t white = RGBA8(0xFF, 0xFF, 0xFF, 0xFF);
	uint32_t green = RGBA8(0x00, 0xFF, 0x00, 0xFF);
	uint32_t red = RGBA8(0xFF, 0x00, 0x00, 0xFF);
	SceCtrlData pad, oldpad;
	
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
			sprintf(cur_mod,"Current in use mod: %s", tmp->name);
			if (strcmp(tmp->name, "id1") == 0) sceIoRemove("ux0:/data/Quake/mods.txt");
			else{
				f = fopen("ux0:/data/Quake/mods.txt", "w");
				fwrite(tmp->name,1,strlen(tmp->name),f);
				fclose(f);
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
	vita2d_fini();
	
	sceKernelExitProcess(0);
	return 0;
}