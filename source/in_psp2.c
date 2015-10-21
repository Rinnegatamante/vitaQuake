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
#include <psp2/ctrl.h>

// mouse variables
cvar_t	m_filter = {"m_filter","0"};

SceCtrlData oldtouch, touch;

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
	sceCtrlPeekBufferPositive(0, &touch, 1);
	int x_val;
	int y_val;
	x_val = abs(touch.lx) < 10 ? 0 : touch.lx * sensitivity.value * 0.01;
	y_val = abs(touch.ly) < 15 ? 0 : touch.ly * sensitivity.value * 0.01;
	if (x_val == 0 && y_val == 0){
		x_val = abs(touch.rx) < 10 ? 0 : touch.rx * sensitivity.value * 0.01;
		y_val = abs(touch.ry) < 15 ? 0 : touch.ry * sensitivity.value * 0.01;
	}
  cl.viewangles[YAW] -= touch.rx;
  if(in_mlook.state & 1){
    V_StopPitchDrift ();
    cl.viewangles[PITCH] -= touch.ry;
  }


}
