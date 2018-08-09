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
#include <vitasdk.h>

CVAR (m_filter,		0, CVAR_ARCHIVE)
CVAR (pstv_rumble,	1, CVAR_ARCHIVE | CVAR_PSTV)

CVAR (retrotouch,					0, CVAR_ARCHIVE | CVAR_PSVITA)
CVAR (psvita_touchmode,				0, CVAR_ARCHIVE | CVAR_PSVITA)	// 0: as a button / 1: as a joystick
CVAR (psvita_front_sensitivity_x,	1, CVAR_ARCHIVE | CVAR_PSVITA)
CVAR (psvita_front_sensitivity_y,	0.5, CVAR_ARCHIVE | CVAR_PSVITA)
CVAR (psvita_back_sensitivity_x,	1, CVAR_ARCHIVE | CVAR_PSVITA)
CVAR (psvita_back_sensitivity_y,	0.5, CVAR_ARCHIVE | CVAR_PSVITA)
CVAR (motioncam,					 0, CVAR_ARCHIVE | CVAR_PSVITA)
CVAR (motion_horizontal_sensitivity,	 0, CVAR_ARCHIVE | CVAR_PSVITA)
CVAR (motion_vertical_sensitivity,	 0, CVAR_ARCHIVE | CVAR_PSVITA)

extern cvar_t always_run, invert_camera;
extern void Log (const char *format, ...);

#define lerp(value, from_max, to_max) ((((value*10) * (to_max*10))/(from_max*10))/10)

uint64_t rumble_tick = 0;
SceCtrlData oldanalogs, analogs;
SceMotionState motionstate;

void IN_Init (void)
{
  Cvar_RegisterVariable (&m_filter);
  Cvar_RegisterVariable (&retrotouch);
  Cvar_RegisterVariable (&always_run);
  Cvar_RegisterVariable (&invert_camera);
  Cvar_RegisterVariable (&pstv_rumble);
  Cvar_RegisterVariable(&psvita_touchmode);

  Cvar_RegisterVariable (&motioncam);
  Cvar_RegisterVariable (&motion_horizontal_sensitivity);
  Cvar_RegisterVariable (&motion_vertical_sensitivity);

  //Touchscreen sensitivity
  Cvar_RegisterVariable(&psvita_front_sensitivity_x);
  Cvar_RegisterVariable(&psvita_front_sensitivity_y);
  Cvar_RegisterVariable(&psvita_back_sensitivity_x);
  Cvar_RegisterVariable(&psvita_back_sensitivity_y);

  sceMotionReset();
  sceMotionStartSampling();
}

void IN_ResetInputs(void)
{
	// Set default PSVITA controls
	Cbuf_AddText("unbindall\n");
	Cbuf_AddText("bind CROSS +jump\n"); // Cross
	Cbuf_AddText("bind SQUARE +attack\n"); // Square
	Cbuf_AddText("bind CIRCLE \"impulse 12\"\n"); // Circle
	Cbuf_AddText("bind TRIANGLE \"impulse 10\"\n"); // Triangle
	Cbuf_AddText("bind LTRIGGER +jump\n"); // Left Trigger
	Cbuf_AddText("bind RTRIGGER +attack\n"); // Right Trigger
	Cbuf_AddText("bind UPARROW +forward\n"); // Up
	Cbuf_AddText("bind DOWNARROW +back\n"); // Down
	Cbuf_AddText("bind LEFTARROW +moveleft\n"); // Left
	Cbuf_AddText("bind RIGHTARROW +moveright\n"); // Right
	Cbuf_AddText("bind TOUCH +showscores\n"); // Touchscreen
	Cbuf_AddText("bind SELECT +showscores\n"); // Touchscreen
	Cbuf_AddText("sensitivity 3.5\n"); // Right Analog Sensitivity
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
}

void IN_StartRumble (void)
{
	if (!pstv_rumble.value) return;
	SceCtrlActuator handle;
	handle.small = 100;
	handle.large = 100;
	sceCtrlSetActuator(1, &handle);
	rumble_tick = sceKernelGetProcessTimeWide();
}

void IN_StopRumble (void)
{
	SceCtrlActuator handle;
	handle.small = 0;
	handle.large = 0;
	sceCtrlSetActuator(1, &handle);
	rumble_tick = 0;
}

void IN_RescaleAnalog(int *x, int *y, int dead) {
	//radial and scaled deadzone
	//http://www.third-helix.com/2013/04/12/doing-thumbstick-dead-zones-right.html

	float analogX = (float) *x;
	float analogY = (float) *y;
	float deadZone = (float) dead;
	float maximum = 128.0f;
	float magnitude = sqrt(analogX * analogX + analogY * analogY);
	if (magnitude >= deadZone)
	{
		float scalingFactor = maximum / magnitude * (magnitude - deadZone) / (maximum - deadZone);
		*x = (int) (analogX * scalingFactor);
		*y = (int) (analogY * scalingFactor);
	} else {
		*x = 0;
		*y = 0;
	}
}

void IN_Move (usercmd_t *cmd)
{
	// ANALOGS
	if ((in_speed.state & 1) || always_run.value){
		cl_forwardspeed.value = 400;
		cl_backspeed.value = 400;
		cl_sidespeed.value = 700;
	}else{
		cl_forwardspeed.value = 200;
		cl_backspeed.value = 200;
		cl_sidespeed.value = 300;
	}

	sceCtrlPeekBufferPositive(0, &analogs, 1);
	int left_x = analogs.lx - 127;
	int left_y = analogs.ly - 127;
	int right_x = analogs.rx - 127;
	int right_y = analogs.ry - 127;

	// Left analog support for player movement
	float x_mov = abs(left_x) < 30 ? 0 : (left_x * cl_sidespeed.value) * 0.01;
	float y_mov = abs(left_y) < 30 ? 0 : (left_y * (left_y > 0 ? cl_backspeed.value : cl_forwardspeed.value)) * 0.01;
	cmd->forwardmove -= y_mov;
	if (gl_xflip.value) cmd->sidemove -= x_mov;
	else cmd->sidemove += x_mov;

	// Right analog support for camera movement
	IN_RescaleAnalog(&right_x, &right_y, 30);
	float x_cam = (right_x * sensitivity.value) * 0.008;
	float y_cam = (right_y * sensitivity.value) * 0.008;
	if (gl_xflip.value) cl.viewangles[YAW] += x_cam;
	else cl.viewangles[YAW] -= x_cam;
	V_StopPitchDrift();
	if (invert_camera.value) cl.viewangles[PITCH] -= y_cam;
	else cl.viewangles[PITCH] += y_cam;

	// TOUCH SUPPORT

	// Retrotouch support for camera movement
	SceTouchData touch;
	if (retrotouch.value){
		sceTouchPeek(SCE_TOUCH_PORT_BACK, &touch, 1);
		if (touch.reportNum > 0) {
			int raw_x = lerp(touch.report[0].x, 1919, 960);
			int raw_y = lerp(touch.report[0].y, 1087, 544);
			int touch_x = raw_x - 480;
			int touch_y = raw_y - 272;
			x_cam = abs(touch_x) < 20 ? 0 : touch_x * psvita_back_sensitivity_x.value * 0.008;
			y_cam = abs(touch_y) < 20 ? 0 : touch_y * psvita_back_sensitivity_x.value * 0.008;
			cl.viewangles[YAW] -= x_cam;
			V_StopPitchDrift();
			if (invert_camera.value) cl.viewangles[PITCH] -= y_cam;
			else cl.viewangles[PITCH] += y_cam;
		}
	}

	if (psvita_touchmode.value == 1)
	{
		sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
		if (touch.reportNum > 0) {
			int raw_x = lerp(touch.report[0].x, 1919, 960);
			int raw_y = lerp(touch.report[0].y, 1087, 544);
			int touch_x = raw_x - 480;
			int touch_y = raw_y - 272;
			x_cam = abs(touch_x) < 20 ? 0 : touch_x * psvita_front_sensitivity_x.value * 0.008;
			y_cam = abs(touch_y) < 20 ? 0 : touch_y * psvita_front_sensitivity_y.value * 0.008;
			cl.viewangles[YAW] -= x_cam;
			V_StopPitchDrift();
			if (invert_camera.value) cl.viewangles[PITCH] -= y_cam;
			else cl.viewangles[PITCH] += y_cam;
		}
	}

  // gyro analog support for camera movement

  if (motioncam.value){
    sceMotionGetState(&motionstate);

    // not sure why YAW or the horizontal x axis is the controlled by angularVelocity.y
    // and the PITCH or the vertical y axis is controlled by angularVelocity.x but its what seems to work
    float x_gyro_cam = motionstate.angularVelocity.y * motion_horizontal_sensitivity.value;
    float y_gyro_cam = motionstate.angularVelocity.x * motion_vertical_sensitivity.value;

    if (gl_xflip.value)
      cl.viewangles[YAW] -= x_gyro_cam;
    else
      cl.viewangles[YAW] += x_gyro_cam;

    V_StopPitchDrift();

    if (invert_camera.value)
      cl.viewangles[PITCH] += y_gyro_cam;
    else
      cl.viewangles[PITCH] -= y_gyro_cam;
  }

	if (pq_fullpitch.value)
		cl.viewangles[PITCH] = COM_Clamp(cl.viewangles[PITCH], -90, 90);
	else
		cl.viewangles[PITCH] = COM_Clamp(cl.viewangles[PITCH], -70, 80);
}
