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
#include <psp2/ctrl.h>
#include <psp2/touch.h>
#define lerp(value, from_max, to_max) ((((value*10) * (to_max*10))/(from_max*10))/10)

// mouse variables
cvar_t	m_filter = {"m_filter","0"};
int inverted = false;
int retro_touch = true;

SceCtrlData oldanalogs, analogs;

void IN_Init (void)
{
  if ( COM_CheckParm ("-nomouse") )
    return;

  Cvar_RegisterVariable (&m_filter);
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
}

void IN_Move (usercmd_t *cmd)
{

	// ANALOGS
	
	sceCtrlPeekBufferPositive(0, &analogs, 1);
	int left_x = analogs.lx - 127;
	int left_y = analogs.ly - 127;
	int right_x = analogs.rx - 127;
	int right_y = analogs.ry - 127;
	
	// Left analog support for player movement
	int x_mov = abs(left_x) < 10 ? 0 : left_x * m_forward.value * 2;
	int y_mov = abs(left_y) < 15 ? 0 : left_y * m_side.value * 2;
	cmd->forwardmove -= y_mov;
	cmd->sidemove += x_mov;
	
	// Right analog support for camera movement
	int x_cam = abs(right_x) < 10 ? 0 : right_x * sensitivity.value * 0.01;
	int y_cam = abs(right_y) < 15 ? 0 : right_y * sensitivity.value * 0.01;
	if (inverted) cl.viewangles[YAW] += x_cam;
	else cl.viewangles[YAW] -= x_cam;
	V_StopPitchDrift();
	if (inverted) cl.viewangles[PITCH] -= y_cam;
	else cl.viewangles[PITCH] += y_cam;
	
	// TOUCHSCREENS
	
	// Touchscreen support for camera movement
	SceTouchData touch;
	sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
    if (touch.reportNum > 0) {
		int raw_x = lerp(touch.report[0].x, 1919, 960);
		int raw_y = lerp(touch.report[0].y, 1087, 544);
		int touch_x = raw_x - 480;
		int touch_y = raw_y - 272;
		x_cam = abs(touch_x) < 20 ? 0 : touch_x * sensitivity.value * 0.01;
		y_cam = abs(touch_y) < 20 ? 0 : touch_y * sensitivity.value * 0.01;
		if (inverted) cl.viewangles[YAW] += x_cam;
		else cl.viewangles[YAW] -= x_cam;
		V_StopPitchDrift();
		if (inverted) cl.viewangles[PITCH] -= y_cam;
		else cl.viewangles[PITCH] += y_cam;
	}
	
	// Retrotouch support for camera movement
	if (retro_touch){
		sceTouchPeek(SCE_TOUCH_PORT_BACK, &touch, 1);
		if (touch.reportNum > 0) {
			int raw_x = lerp(touch.report[0].x, 1919, 960);
			int raw_y = lerp(touch.report[0].y, 1087, 544);
			int touch_x = raw_x - 480;
			int touch_y = raw_y - 272;
			x_cam = abs(touch_x) < 20 ? 0 : touch_x * sensitivity.value * 0.01;
			y_cam = abs(touch_y) < 20 ? 0 : touch_y * sensitivity.value * 0.01;
			if (inverted) cl.viewangles[YAW] += x_cam;
			else cl.viewangles[YAW] -= x_cam;
			V_StopPitchDrift();
			if (inverted) cl.viewangles[PITCH] -= y_cam;
			else cl.viewangles[PITCH] += y_cam;
		}
	}
	
}
