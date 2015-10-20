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

#include <stdio.h>
#include <3ds.h>
#include "quakedef.h"

#define TICKS_PER_SEC 268111856LL
#define SAMPLE_RATE   32730
#define CSND_BUFSIZE 4096

u8 *csnd_buf;
u64 initial_tick;
int snd_inited;

qboolean SNDDMA_Init(void)
{
  snd_initialized = 0;

  if(csndInit() != 0){
    Con_Printf("csndInit() failed\n");
    return 0;
  }

  csnd_buf = linearAlloc(CSND_BUFSIZE);

	/* Fill the audio DMA information block */
	shm = &sn;
	shm->splitbuffer = 0;
	shm->samplebits = 16;
	shm->speed = SAMPLE_RATE;
	shm->channels = 1;
	shm->samples = CSND_BUFSIZE / (shm->samplebits / 8);
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = csnd_buf;
  int ret = csndPlaySound(0x10, SOUND_REPEAT | SOUND_FORMAT_16BIT, SAMPLE_RATE, 0.75f, 0.0f, (u32*)csnd_buf, (u32*)csnd_buf, CSND_BUFSIZE);
  if(ret != 0){
    Con_Printf("csndPlaySound() failed\n");
    return 0;
  }
  initial_tick = svcGetSystemTick();

  snd_initialized = 1;
	return 1;
}

int SNDDMA_GetDMAPos(void)
{
  if(!snd_initialized)
    return 0;

  u64 delta = (svcGetSystemTick() - initial_tick);
  u64 samplepos = delta * (SAMPLE_RATE) / TICKS_PER_SEC;
	shm->samplepos = samplepos;
	return samplepos;
}

void SNDDMA_Shutdown(void)
{
  if(snd_initialized){
    CSND_SetPlayState(0x10, 0);
    csndExecCmds(false);
    linearFree(csnd_buf);
    csndExit();
  }
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
  //if(snd_initialized)
    //GSPGPU_FlushDataCache(NULL, csnd_buf, csnd_bufsize);
}
