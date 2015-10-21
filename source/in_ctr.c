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
// in_ctr.c -- for the Nintendo 3DS

#include "quakedef.h"

#include <3DS.h>
#include "ctr.h"

//Touchscreen mode identifiers
#define TMODE_TOUCHPAD 1
#define TMODE_KEYBOARD 2

//Keyboard is currently laid out on a 14*4 grid of 20px*20px boxes for lazy implementation
char keymap[14 * 4] = {
  '`' , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '+', K_BACKSPACE,
  K_TAB, 'q' , 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '|',
  0, 'a' , 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', K_ENTER, K_ENTER,
  K_SHIFT, 'z' , 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', K_SHIFT, K_SHIFT, K_SHIFT
};

u16* touchpadOverlay;
u16* keyboardOverlay;

cvar_t	m_filter = {"m_filter","0"};

circlePosition cstick;
circlePosition circlepad;
touchPosition oldtouch, touch;
char lastKey = 0;

int tmode;
u16* tfb;

u64 lastTap = 0;
void ctrTouchpadTap(){
  u64 thisTap = Sys_FloatTime();
  if(oldtouch.py > 195 && oldtouch.py < 240 && oldtouch.px > 0 && oldtouch.px < 45){
    Key_Event('`', true);
    lastKey = '`';
  }
  else if(oldtouch.py > 195 && oldtouch.py < 240 && oldtouch.px > 1 && oldtouch.px < 320){
    tmode = 2;
    ctrDrawTouchOverlay();
  }
  else if ((thisTap - lastTap) < 0.5){
    Key_Event(K_SPACE, true);
    lastKey = K_SPACE;
  }
  lastTap = thisTap;
}

void ctrKeyboardTap(){
  if(oldtouch.py > 20 && oldtouch.py < 100 && oldtouch.px > 15 && oldtouch.px < 295){
    char key = keymap[((oldtouch.py - 20) / 20) * 14 + (oldtouch.px - 15)/20];
    Key_Event(key, true);
    lastKey = key;
  }

  else if(oldtouch.py > 100 && oldtouch.py < 120 && oldtouch.px > 95 && oldtouch.px < 215){
    Key_Event(K_SPACE, true);
    lastKey = K_SPACE;
  }

  if(oldtouch.py > 195 && oldtouch.py < 240 && oldtouch.px > 1 && oldtouch.px < 320){
    tmode = 1;
    ctrDrawTouchOverlay();
  }
}

void ctrProcessTap(){
  if(tmode == TMODE_TOUCHPAD)
    ctrTouchpadTap();
  else
    ctrKeyboardTap();
}

void ctrDrawTouchOverlay(){
  u16* overlay;
  if(tmode == 1)
    overlay = touchpadOverlay;
  else
    overlay = keyboardOverlay;
  int x,y;
	for(x=0; x<320; x++){
		for(y=0; y<240;y++){
			tfb[(x*240 + (239 - y))] = overlay[(y*320 + x)];
		}
	}
}

void IN_Init (void)
{
  if ( COM_CheckParm ("-nomouse") )
    return;

  tmode = 1;
  Cvar_RegisterVariable (&m_filter);
  FILE *texture = fopen("/touchpadOverlay.bin", "rb");
  if(!texture)
    Sys_Error("Could not open touchpadOverlay.bin\n");
  fseek(texture, 0, SEEK_END);
  int size = ftell(texture);
  fseek(texture, 0, SEEK_SET);
  touchpadOverlay = malloc(size);
  fread(touchpadOverlay, 1, size, texture);
  fclose(texture);
  texture = fopen("/keyboardOverlay.bin", "rb");
  if(!texture)
    Sys_Error("Could not open keyboardOverlay.bin\n");
  fseek(texture, 0, SEEK_END);
  size = ftell(texture);
  fseek(texture, 0, SEEK_SET);
  keyboardOverlay = malloc(size);
  fread(keyboardOverlay, 1, size, texture);
  fclose(texture);
  tfb = (u16*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
  //ctrDrawTouchOverlay();
}

void IN_Shutdown (void)
{
  free(touchpadOverlay);
  free(keyboardOverlay);
}

void IN_Commands (void)
{
}

u64 tick;

void IN_Move (usercmd_t *cmd)
{

  if(lastKey){
    Key_Event(lastKey, false);
    lastKey = 0;
  }

  if(hidKeysDown() & KEY_TOUCH){
    hidTouchRead(&oldtouch);
    tick = Sys_FloatTime();
  }

  if(hidKeysUp() & KEY_TOUCH){
    if((Sys_FloatTime() - tick) < 1.0)
      ctrProcessTap();
  }



  else if(hidKeysHeld() & KEY_TOUCH){
    hidTouchRead(&touch);
    touch.px = (touch.px + oldtouch.px) / 2;
    touch.py = (touch.py + oldtouch.py) / 2;
    cl.viewangles[YAW] -= (touch.px - oldtouch.px) * sensitivity.value/2;
    if(in_mlook.state & 1)
      cl.viewangles[PITCH] += (touch.py - oldtouch.py) * sensitivity.value/2;
    oldtouch = touch;
  }

  hidCircleRead(&circlepad);
  cmd->forwardmove += m_forward.value * circlepad.dy * 2;
  cmd->sidemove += m_side.value * circlepad.dx * 2;

  if(isN3DS){
    hidCstickRead(&cstick);
    cstick.dx = abs(cstick.dx) < 10 ? 0 : cstick.dx * sensitivity.value * 0.01;
    cstick.dy = abs(cstick.dy) < 15 ? 0 : cstick.dy * sensitivity.value * 0.01;

    cl.viewangles[YAW] -= cstick.dx;
    if(in_mlook.state & 1){
      cl.viewangles[PITCH] -= cstick.dy;
    }
  }

  if(in_mlook.state & 1)
    V_StopPitchDrift ();


}
