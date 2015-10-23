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
#include <stdio.h>
#include <psp2/audioout.h>
#include <psp2/types.h>
#include "quakedef.h"

#define u64 uint64_t
#define u8 uint8_t

#define SAMPLE_RATE   22050
#define AUDIOSIZE 4096

u8 *audiobuffer;
u64 initial_tick;
int snd_inited;
int chn = -1;
int stop_audio = false;
//int update = false;
float tickRate;

static int audio_thread(int args, void *argp)
{
	chn = sceAudioOutOpenPort(PSP2_AUDIO_OUT_PORT_TYPE_VOICE, AUDIOSIZE / 2, SAMPLE_RATE, PSP2_AUDIO_OUT_MODE_MONO);
	sceAudioOutSetConfig(chn, -1, -1, -1);
    sceAudioOutSetVolume(chn, PSP2_AUDIO_VOLUME_FLAG_L_CH | PSP2_AUDIO_VOLUME_FLAG_R_CH, 32767);
	
    while (!stop_audio)
    {
		//if (update){ 
			sceAudioOutOutput(chn, audiobuffer);
			//update = false;
		//}
		
    }

    sceKernelExitThread(0);
    return 0;
}

qboolean SNDDMA_Init(void)
{
	snd_initialized = 0;

	audiobuffer = malloc(AUDIOSIZE);

	/* Fill the audio DMA information block */
	shm = &sn;
	shm->splitbuffer = 0;
	shm->samplebits = 16;
	shm->speed = SAMPLE_RATE;
	shm->channels = 1;
	shm->samples = AUDIOSIZE / (shm->samplebits / 8);
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = audiobuffer;
	
	tickRate = 1.0f / sceRtcGetTickResolution();
	
	SceUID audiothread = sceKernelCreateThread("Audio Thread", (void*)&audio_thread, 0x10000100, 0x10000, 0, 0, NULL);
	int res = sceKernelStartThread(audiothread, sizeof(audiothread), &audiothread);
	if (res != 0){
		Sys_Error("Failed to init audio thread (0x%x)", res);
		return 0;
	}
	
	sceRtcGetCurrentTick(&initial_tick);

	snd_initialized = 1;
	return 1;
}

int SNDDMA_GetDMAPos(void)
{
	if(!snd_initialized)
		return 0;

	u64 tick;
	sceRtcGetCurrentTick(&tick);
	const unsigned int deltaTick  = tick - initial_tick;
	const float deltaSecond = deltaTick * tickRate;
	u64 samplepos = deltaSecond * SAMPLE_RATE;
	shm->samplepos = samplepos;
	return samplepos;
}

void SNDDMA_Shutdown(void)
{
  if(snd_initialized){
	stop_audio = true;
    sceAudioOutReleasePort(chn);
    free(audiobuffer);
	chn = -1;
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
	//update = true;
}