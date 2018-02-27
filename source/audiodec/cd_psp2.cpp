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
#include <vitasdk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define min
#define max
extern "C"{
	#include "../quakedef.h"
}
#undef min
#undef max
#include "audio_decoder.h"

extern char* mod_path;

#define BUFSIZE 8192 // Max dimension of audio buffer size
#define NSAMPLES 2048 // Number of samples for output

// Music block struct
struct DecodedMusic{
	uint8_t* audiobuf;
	uint8_t* audiobuf2;
	uint8_t* cur_audiobuf;
	FILE* handle;
	bool isPlaying;
	bool loop;
	volatile bool pauseTrigger;
	volatile bool closeTrigger;
	volatile bool changeVol;
};

// Internal stuffs
DecodedMusic* BGM = NULL;
std::unique_ptr<AudioDecoder> audio_decoder;
SceUID thread, Audio_Mutex, Talk_Mutex;
volatile bool mustExit = false;
float old_vol = 1.0;

// Audio thread code
static int bgmThread(unsigned int args, void* arg){
	
	// Initializing audio port
	int ch = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, NSAMPLES, 48000, SCE_AUDIO_OUT_MODE_STEREO);
	sceAudioOutSetConfig(ch, -1, -1, (SceAudioOutMode)-1);
	old_vol = bgmvolume.value;
	int vol = 32767 * bgmvolume.value;
	int vol_stereo[] = {vol, vol};
	sceAudioOutSetVolume(ch, (SceAudioOutChannelFlag)(SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH), vol_stereo);
	
	DecodedMusic* mus;
	for (;;){
		
		// Waiting for an audio output request
		sceKernelWaitSema(Audio_Mutex, 1, NULL);
		
		// Fetching track
		mus = BGM;
		
		// Checking if a new track is available
		if (mus == NULL){
			
			//If we enter here, we probably are in the exiting procedure
			if (mustExit){
				sceAudioOutReleasePort(ch);
				mustExit = false;
				sceKernelExitDeleteThread(0);
			}
		
		}
		
		// Initializing audio decoder
		audio_decoder = AudioDecoder::Create(mus->handle, "Track");
		audio_decoder->Open(mus->handle);
		audio_decoder->SetLooping(mus->loop);
		audio_decoder->SetFormat(48000, AudioDecoder::Format::S16, 2);
		
		// Initializing audio buffers
		mus->audiobuf = (uint8_t*)malloc(BUFSIZE);
		mus->audiobuf2 = (uint8_t*)malloc(BUFSIZE);
		mus->cur_audiobuf = mus->audiobuf;
		
		// Audio playback loop
		for (;;){
		
			// Check if the music must be paused
			if (mus->pauseTrigger || mustExit){
			
				// Check if the music must be closed
				if (mus->closeTrigger){
					audio_decoder.reset();
					free(mus->audiobuf);
					free(mus->audiobuf2);
					free(mus);
					BGM = NULL;
					mus = NULL;
					if (!mustExit){
						sceKernelSignalSema(Talk_Mutex, 1);
						break;
					}
				}
				
				// Check if the thread must be closed
				if (mustExit){
				
					// Check if the audio stream has already been closed
					if (mus != NULL){
						mus->closeTrigger = true;
						continue;
					}
					
					// Recursively closing all the threads
					sceAudioOutReleasePort(ch);
					mustExit = false;
					sceKernelExitDeleteThread(0);
					
				}
			
				mus->isPlaying = !mus->isPlaying;
				mus->pauseTrigger = false;
			}
			
			// Check if a volume change is required
			if (mus->changeVol){
				old_vol = bgmvolume.value;
				int vol = 32767 * bgmvolume.value;
				int vol_stereo[] = {vol, vol};
				sceAudioOutSetVolume(ch, (SceAudioOutChannelFlag)(SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH), vol_stereo);
				mus->changeVol = false;
			}
			
			if (mus->isPlaying){
				
				// Check if audio playback finished
				if ((!mus->loop) && audio_decoder->IsFinished()) mus->isPlaying = false;
				
				// Update audio output
				if (mus->cur_audiobuf == mus->audiobuf) mus->cur_audiobuf = mus->audiobuf2;
				else mus->cur_audiobuf = mus->audiobuf;
				audio_decoder->Decode(mus->cur_audiobuf, BUFSIZE);	
				sceAudioOutOutput(ch, mus->cur_audiobuf);
				
			}
			
		}
		
	}
	
}

void CDAudio_Play(byte track, bool looping)
{
	CDAudio_Stop();
	char fname[256];
	sprintf (fname, "%s/%s/cdtracks/track", host_parms.basedir, (mod_path == NULL) ? GAMENAME_DIR : mod_path);
	if (track < 100){
		sprintf(fname, "%s0", fname);
		if (track < 10){
			sprintf(fname, "%s0", fname);
		}
	}
	sprintf(fname,"%s%d",fname,track);
	char tmp[256];
	sprintf(tmp,"%s.ogg",fname);
		
	FILE* fd = fopen(tmp,"rb");
	if (fd == NULL){
		sprintf(tmp,"%s.mp3",fname);
		fd = fopen(tmp,"rb");
	}
	if (fd == NULL) return;
	DecodedMusic* memblock = (DecodedMusic*)malloc(sizeof(DecodedMusic));
	memblock->handle = fd;
	memblock->pauseTrigger = false;
	memblock->closeTrigger = false;
	memblock->isPlaying = true;
	memblock->loop = looping;
	BGM = memblock;
	sceKernelSignalSema(Audio_Mutex, 1);
}

void CDAudio_Stop(void)
{
	if (BGM != NULL){
		BGM->closeTrigger = true;
		BGM->pauseTrigger = true;
		sceKernelWaitSema(Talk_Mutex, 1, NULL);
	}
}


void CDAudio_Pause(void)
{
	if (BGM != NULL) BGM->pauseTrigger = true;
}


void CDAudio_Resume(void)
{
	if (BGM != NULL) BGM->pauseTrigger = true;
}


void CDAudio_Update(void)
{
	if (BGM != NULL){
		if (old_vol != bgmvolume.value) BGM->changeVol = true;
	}
}


int CDAudio_Init(void)
{
	int res;
	
	// Creating audio mutex
	Audio_Mutex = sceKernelCreateSema("Audio Mutex", 0, 0, 1, NULL);
	Talk_Mutex = sceKernelCreateSema("Talk Mutex", 0, 0, 1, NULL);
	
	// Creating audio thread
	thread = sceKernelCreateThread("Audio Thread", &bgmThread, 0x10000100, 0x10000, 0, 0, NULL);
	sceKernelStartThread(thread, sizeof(thread), &thread);
	
	return 0;
}


void CDAudio_Shutdown(void)
{	
	mustExit = true;
	sceKernelSignalSema(Audio_Mutex, 1);
	sceKernelWaitThreadEnd(thread, NULL, NULL);
	sceKernelDeleteSema(Audio_Mutex);
	sceKernelDeleteSema(Talk_Mutex);
}
