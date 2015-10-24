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

// mouse variables
cvar_t	m_filter = {"m_filter","0"};
int inverted = false;

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

}
