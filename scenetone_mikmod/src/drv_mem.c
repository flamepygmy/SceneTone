//  ---------------------------------------------------------------------------
//  This file is part of Scenetone, a music player aimed for playing old
//  music tracker modules and C64 tunes.
//
//  Copyright (C) 2006  Jani Vaarala <flame@pygmyprojects.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  --------------------------------------------------------------------------

#include "mikmod_internals.h"

int   mikmod_mem_sample_mix_count;
void *mikmod_mem_sample_out_ptr;

#define ZEROLEN 32768

static BOOL MEM_IsThere(void)
{
	return 1;
}

static BOOL MEM_Init(void)
{
    return VC_Init();
}

static void MEM_Exit(void)
{
    VC_Exit();
}

static void MEM_Update(void)
{
    if(mikmod_mem_sample_out_ptr && (mikmod_mem_sample_mix_count > 0))
    {
	VC_WriteBytes((SBYTE *)mikmod_mem_sample_out_ptr,mikmod_mem_sample_mix_count);
	mikmod_mem_sample_mix_count = 0;
    }
}

MIKMODAPI MDRIVER drv_mem={
	NULL,
	"Mem",
	"Mem Driver v0.1",
	255,255,
	"mem",

	NULL,
	MEM_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	VC_SampleSpace,
	VC_SampleLength,
	MEM_Init,
	MEM_Exit,
	NULL,
	VC_SetNumVoices,
	VC_PlayStart,
	VC_PlayStop,
	MEM_Update,
	NULL,
	VC_VoiceSetVolume,
	VC_VoiceGetVolume,
	VC_VoiceSetFrequency,
	VC_VoiceGetFrequency,
	VC_VoiceSetPanning,
	VC_VoiceGetPanning,
	VC_VoicePlay,
	VC_VoiceStop,
	VC_VoiceStopped,
	VC_VoiceGetPosition,
	VC_VoiceRealVolume
};
