//  ---------------------------------------------------------------------------
//  This file is part of Scenetone, a music player aimed for playing old
//  music tracker modules and C64 tunes.
//
//  Copyright (C) 2006-2008  Jani Vaarala <flame@pygmyprojects.com>
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

#include <e32const.h>
#include "scenetonesound.h"

#include "scenetone_libsidplay.h"
#include "scenetone_mikmod.h"
#if defined(SCENETONE_INCLUDE_SHINE)
#include "scenetone_shine.h"
#else
#include "scenetone_wav.h"
#endif

#include <estlib.h>

// TODO: correct way would be to create a client and server side class -> destruction and
//       what not would be more clear...

// operation mode:
//   init:
//         1) mix buffer 0
//         2) mix buffer 1
//         3) send buffer 0
//         4) send buffer 1
//
//   buffercopied:
//         1) set flag buffer_<x>_copied
//         2) trig idle active object to mix in N passes into buffer <x>
//
//   idle active object:
//         1) mix <y> samples, if not finished, set active again, go to AOS (this is to guarantee that AOs get run time)
//         2) if finished, write buffer, exit active object
	
CScenetoneSound* CScenetoneSound::NewL(void)
{
	CScenetoneSound* s = new (ELeave) CScenetoneSound();
	s->ConstructL();
	s->iSWVolume = 256;
	return s;
}

CScenetoneSound::~CScenetoneSound()
{
	delete iBuffer1;
	delete iBuffer2;
}

CScenetoneSound::CScenetoneSound() :
    iDesc1(0,0,0),
    iDesc2(0,0,0)
{
	
}

/* preferred order of sample rate selection */
static const TInt sampleRateConversionTable[] =
{
    44100,  TMdaAudioDataSettings::ESampleRate44100Hz,
    32000,  TMdaAudioDataSettings::ESampleRate32000Hz,
    22050,  TMdaAudioDataSettings::ESampleRate22050Hz,
    16000,  TMdaAudioDataSettings::ESampleRate16000Hz,
    11025,  TMdaAudioDataSettings::ESampleRate11025Hz,
    48000,  TMdaAudioDataSettings::ESampleRate48000Hz,
    8000,   TMdaAudioDataSettings::ESampleRate8000Hz
};

_LIT(KSIDSuffix,"sid");

void CScenetoneSound::MaoscOpenComplete(TInt aError)
{
	TInt err;
	
    if( aError != KErrNone )
    {
		User::Panic(_L("STREAMOPEN FAILED"), aError);
		iState = EStopped;
		return;
    }

	TInt mix_freq     = 0;
	TInt mix_channels = 0;

    /* Try out which sample rates are supported, first check stereo */
    for(TUint i=0;i<sizeof(sampleRateConversionTable)/sizeof(TInt);i+=2)
    {
        TRAPD(err, iStream->SetAudioPropertiesL(sampleRateConversionTable[i+1], TMdaAudioDataSettings::EChannelsStereo));
        if(err == KErrNone)
        {
            mix_freq     = sampleRateConversionTable[i];
			mix_channels = 2;
			break;
        }
   }

   /* Then mono */
   if(mix_freq == 0)
   {
   		for(TUint i=0;i<sizeof(sampleRateConversionTable)/sizeof(TInt);i+=2)
    	{
        	TRAPD(err, iStream->SetAudioPropertiesL(sampleRateConversionTable[i+1], TMdaAudioDataSettings::EChannelsMono));
        	if(err == KErrNone)
        	{
            	mix_freq     = sampleRateConversionTable[i];
				mix_channels = 1;
				break;
        	}
		}
    }

	/* Select song provider depending on file suffix. If it is ".sid" -> sid, else mikmod */
	/* TODO: let the song providers recognize by themselves..what if N can recognize same? */
	
	if(iFilename.Right(3) == KSIDSuffix)		iSongProvider = iSongProviderSID;
	else										iSongProvider = iSongProviderMikmod;

	err = iSongProvider->InitializeSong( iFilename, mix_freq, mix_channels, iMaxSubsong, iCurrentSubsong );
	if( err != KErrNone )
	{
		iState = EStopped;
		return;
	}

	err = iSongProvider->StartSong(iCurrentSubsong);
	if( err != KErrNone )
	{
		iState = EStopped;
		return;
	}

    iState = EPlaying;

    // Mix 2 buffers ready
	iSongProvider->GetSamples( iDesc1 );
	iSongProvider->GetSamples( iDesc2 );

    iStream->SetVolume((iStream->MaxVolume()*iVolume)/100);
    iStream->SetBalanceL();

    // Write both buffers
    iStream->WriteL(iDesc1);
    iStream->WriteL(iDesc2);

    iMixStep = MIX_BUFFER_TIMES;
    iIdleActive = EFalse;
}

void CScenetoneSound::MixNSamples(TInt aBytes, char *aOut)
{
	TPtr8 tmp_ptr = TPtr8((unsigned char *)aOut, aBytes, aBytes);
	iSongProvider->GetSamples(tmp_ptr);	
	if(iVisualizer)
		iVisualizer->NewSamples((unsigned char *)aOut, aBytes, FALSE); // force stereo for now

	if(iSWVolume < 256)
    {
        TInt16 *p = (TInt16*)aOut;
        TInt v  = iSWVolume;
        int i;
        
        
        for(i=0;i<aBytes/2;i++)
        {
            p[i] = (TInt16)((v * p[i]) >> 8);
        }
    }
}


/***************************************************************************************************
 *  MixLoop AO tries to premix the buffer to be ready before the next "buffer copied"
 *  callback. If it is ready in time, it will exit the AO to mark the completion. All mixing is
 *  called from here (except the startup one). No mixing is called from "buffer copied" callback.
 *
 *  Relevant flags:
 *                    + iStartOnNext: if true, AO mixloop should finish the current buffer ASAP
 *                                    and move to the next one (buffercopied callback was called!)
 *                    + iIdleActive:  if true, AO mixloop is running (in idle)
 *                    + iMixStep:     counting number (0....MIX_BUFFER_TIMES) counting the mix loop.
 * 									  idle mixing loop is called MIX_BUFFER_TIMES for the buffer,
 * 									  then it's complete.
 * 					  + iBufferToMix: which is the buffer to mix (currently)
 **************************************************************************************************/
 
static TInt MixLoop(TAny *t)
{
    CScenetoneSound *s = (CScenetoneSound*)t;
    TInt samplesLeft = ((MIX_BUFFER_TIMES - s->iMixStep) * (MIX_BUFFER_LENGTH/MIX_BUFFER_TIMES));

    /* If we are stopping or already stopped, exit idle loop */
    if(s->State() != CScenetoneSound::EPlaying)    return EFalse;

    if(s->iStartOnNext)
    {
		/* OK. We are late. Mix the current buffer to the end, write it and start on the next one */
		if(s->iBufferToMix == 0)
		{
	    	s->MixNSamples(samplesLeft, (char*)(s->iBuffer1 + ((MIX_BUFFER_LENGTH/MIX_BUFFER_TIMES)*s->iMixStep)));
	    	s->iStream->WriteL(s->iDesc1);
		}
		else
		{
	   		s->MixNSamples(samplesLeft, (char*)(s->iBuffer2 + ((MIX_BUFFER_LENGTH/MIX_BUFFER_TIMES)*s->iMixStep)));
	    	s->iStream->WriteL(s->iDesc2);
		}

		/* Initialize mixing on the other buffer */
		s->iMixStep     = 0;
		s->iBufferToMix = 1 - s->iBufferToMix;
		s->iStartOnNext = EFalse;

		return ETrue;
    }
    else
    {
		/* Normal mixing step, callback has not been called, select buffer first */
		char   *mix_buffer = (char*)s->iBuffer1;
		
		/* Select buffer */		
		if(s->iBufferToMix == 1)
		{
			mix_buffer   = (char*)s->iBuffer2;
		}


    	s->MixNSamples(MIX_BUFFER_LENGTH/MIX_BUFFER_TIMES, (char*)(mix_buffer + ((MIX_BUFFER_LENGTH/MIX_BUFFER_TIMES)*s->iMixStep)));
    	s->iMixStep++;

		if(s->iMixStep == MIX_BUFFER_TIMES)
	    {
			/* Buffer is complete, write it and complete the AO */
			if(s->iBufferToMix == 0)	s->iStream->WriteL(s->iDesc1);
			else						s->iStream->WriteL(s->iDesc2);

			s->iIdleActive = EFalse;
			return EFalse;
    	}
	}

	/* Continue mixing */
	return ETrue;
}

void CScenetoneSound::MaoscBufferCopied(TInt aError, const TDesC8 &aBuffer)
{
    if( aError != KErrNone )
    {
		iState = EStopped;
		return;
    }

    if(iState == EPlaying)
    {
		if(!iIdleActive)
		{
	    	/* If background mixing is not being done (active object doing the mixing), lets start mixing to next buffer (other is still being copied) */
			iBufferToMix = 0;
	    	if(aBuffer.Ptr() == iBuffer2)
	    	{
 			    iBufferToMix = 1;
	    	}
	    	
	    	iMixStep = 0;

	    	/* Trig AO to do the mix in N steps for the next buffer */
	    	iStartOnNext = EFalse;
	    	iIdle = CIdle::NewL(CActive::EPriorityIdle);
	    	iIdle->Start( TCallBack(MixLoop, this) );
	    	iIdleActive = ETrue;
		}
		else
		{
	    	/* Previous buffer was NOT completed in time.. flag AO to finish the previous one and start on next */
	    	iStartOnNext = ETrue;
		}
    }
}

void CScenetoneSound::MaoscPlayComplete(TInt aError)
{
    if( aError == KErrCancel )
    {
		iState = EStopped;
    }
}

TInt CScenetoneSound::GetCurrentSubsong()
{
	return iCurrentSubsong;	
}

TInt CScenetoneSound::GetMaxSubsong()
{
	return iMaxSubsong;	
}
		
void CScenetoneSound::SetFileName(const TDesC16 &aFilename)
{
    iFilename = aFilename;
}

//#define WAVGEN_TIME           35
#define WAVGEN_CHANNELS       1
#define WAVGEN_SAMPLERATE     44100
#define WAVGEN_BUFFERSIZE     32768
//#define WAVGEN_SAMPLES        WAVGEN_SAMPLERATE*WAVGEN_TIME
//#define WAVGEN_BYTES          WAVGEN_SAMPLES*WAVGEN_CHANNELS*2
//#define UPDATE_COUNT          (((SAMPLELEN)/(WAVGEN_BUFFERSIZE))+1)
#define WAVGEN_BITRATE        192

TInt CScenetoneSound::GenerateWav(const TDesC16 &aOutName, TReal aStart, TReal aEnd)
{
        iWavGenStartByte   = (TInt)(WAVGEN_SAMPLERATE*aStart) * WAVGEN_CHANNELS * 2;
	iWavGenSamples     = (TInt)(WAVGEN_SAMPLERATE*(aEnd-aStart));
	iWavGenBytes       = iWavGenSamples * WAVGEN_CHANNELS * 2;
	 
	iOutFilename = aOutName;
	PrivateWaitRequestOK();
	iPlayerThread.RequestComplete(iRequestPtr, SCENETONE_COMMAND_GENERATE_WAV);
	
	return 1;
}

/* This nasty global stuff is needed fot the mixcallback. TODO: could this be removed ? */
static CScenetoneSound *soundi 	= NULL;
static TInt wavgencount      	= 0;
static TInt wavgenskip          = 0;
static TInt wavgenfinalcount 	= 0;
static TInt notified         	= 0;
static TRequestStatus dummyreq;

TInt mixcallback(TAny *aOut, TInt aBytes)
{
        /* first things first.. lets skip N bytes */
        if(wavgenskip > 0)
	{
	  unsigned char *tmp = new unsigned char[1024];
	  TPtr8 tmp_ptr = TPtr8(tmp, 1024, 1024);

	  while(1)
	  {
	    if(wavgenskip >= 1024)
	    {
	      soundi->iSongProvider->GetSamples(tmp_ptr);	
	    }
	    else break;

	    wavgenskip -= 1024;
	  }

	  if(wavgenskip > 0)
	  {
	    tmp_ptr = TPtr8(tmp, wavgenskip, wavgenskip);
	    soundi->iSongProvider->GetSamples(tmp_ptr);	
	    wavgenskip = 0;
	  }
	  delete tmp;
	}

	/* now the actual generation starts ... */
	if(wavgencount >= wavgenfinalcount)		return 0;

	if((wavgencount & 0x3fff) == 0)
	{
		soundi->iProgressDialog->UpdateProgressDialog( wavgencount, wavgenfinalcount );
	}
	
	TPtr8 tmp_ptr = TPtr8((unsigned char *)aOut, aBytes, aBytes);
	soundi->iSongProvider->GetSamples(tmp_ptr);	

	wavgencount += aBytes;
	return aBytes;
}
	

void CScenetoneSound::InitializeGenerateWavL(const TDesC16 &aName, const TDesC16 &aOutName)
{
	iFilename = aName;
	iOutFilename = aOutName;

	PrivateWaitRequestOK();
    iPlayerThread.RequestComplete(iRequestPtr, SCENETONE_COMMAND_GENERATE_WAV);
}

_LIT(KProgressPrompt,"Generating FILE");
_LIT(KIconText,"Los icons");

void CScenetoneSound::PrivateGenerateWav()
{
	TInt err;

	iState = EStopped;
	
	if(iFilename.Right(3) == KSIDSuffix)		iSongProvider = iSongProviderSID;
	else										iSongProvider = iSongProviderMikmod;

	/* Initialize song from the currently active subsong, note: song must be the same + playing.. */

	TInt mix_subsongs,mix_defaultsong;
	err = iSongProvider->InitializeSong( iFilename, WAVGEN_SAMPLERATE, WAVGEN_CHANNELS, mix_subsongs, mix_defaultsong );
	if( err != KErrNone )
	{
		iState = EStopping;
		iStream->Stop();
		User::Panic(_L("testing.."), 1);
	}

	err = iSongProvider->StartSong(iCurrentSubsong);
	if( err != KErrNone )
	{
		User::Panic(_L("testing.."), 2);
	}

	notified            = 0;
	wavgencount 		= 0;
	wavgenfinalcount 	= iWavGenBytes;
	wavgenskip              = iWavGenStartByte;
	soundi = this;

	// Invert thread importance
 	RThread curthread;
//	curthread.SetProcessPriority(EPriorityBackground);
// 	curthread.SetPriority( EPriorityMuchLess );

	iProgressDialog = CAknGlobalProgressDialog::NewL();
	iProgressDialog->ShowProgressDialogL(dummyreq, KProgressPrompt);
	iProgressDialog->UpdateProgressDialog( 0, iWavGenBytes );
	
	iMP3Writer->Start(iOutFilename, WAVGEN_SAMPLERATE, WAVGEN_CHANNELS, iWavGenSamples, WAVGEN_BITRATE, mixcallback );

	// We come here when the conversion is complete, restore thread importance
	iProgressDialog->ProcessFinished();
	delete iProgressDialog;

//	curthread.SetProcessPriority(EPriorityHigh);
// 	curthread.SetPriority( EPriorityRealTime );
}

void CScenetoneSound::StartL()
{
	PrivateWaitRequestOK();
	iPlayerThread.RequestComplete(iRequestPtr, SCENETONE_COMMAND_START_PLAYBACK);
}

void CScenetoneSound::PrivateStart()
{
    /* StartL should not be called unless playback is stopped */
    if(iState != EStopped) return;

    delete iStream;

    iState = EStarting;
    iStream = CMdaAudioOutputStream::NewL(*this);
    iStream->Open(&iSettings);
}

void CScenetoneSound::PrivateNextSubsong()
{
    if((iState != EPlaying) || (iCurrentSubsong == iMaxSubsong))	return;

	iCurrentSubsong++;
	TInt err = iSongProvider->StartSong(iCurrentSubsong);
}

void CScenetoneSound::PrivatePrevSubsong()
{
    if((iState != EPlaying) || (iCurrentSubsong == 0))			return;

	iCurrentSubsong--;
	TInt err = iSongProvider->StartSong(iCurrentSubsong);
}

TInt CScenetoneSound::State()
{
    return iState;
}

void CScenetoneSound::StopL()
{
	PrivateWaitRequestOK();
	iPlayerThread.RequestComplete(iRequestPtr, SCENETONE_COMMAND_STOP_PLAYBACK);
}

void CScenetoneSound::PrivateStop()
{
    if(iState == EPlaying)
    {
		iState = EStopping;
		iStream->Stop();
    }
}

#if defined(SCENETONE_PYTHON_VERSION)

/* This is the message processor in the Python server */
void CScenetoneSound::Message(const RMessage2& aMessage)
{
    TInt deslen = aMessage.GetDesLength(0);
    RBuf8 buffer;
    buffer.CreateL(deslen);
    buffer.CleanupClosePushL();
    aMessage.ReadL(0,buffer,0);             /* read the command byte stream */

    const TUint8 *ptr = buffer.Ptr();
    TUint16       cmd = *((TUint16*)ptr);             // get command (short)
    TUint16       len = *((TUint16*)&ptr[2]);         // get length (short) of the data packet

    switch(cmd)
    {
        case SCENETONE_PYTHON_COMMAND_SET_FILENAME:
	    TPtr16 ptr = TPtr16( ((TUint16*)&ptr[4]), len);
	    SetFileName(ptr);

	    // no return data
	    break;

        case SCENETONE_PYTHON_COMMAND_PLAY:
	    StartL();

	    // no return data
	    break;

//        default:

    }

    // Clean up the memory acquired by the RBuf variable "buffer"
    CleanupStack::PopAndDestroy();
}

#endif /* SCENETONE_PYTHON_VERSION */

void CScenetoneSound::SetSWVolume(TInt aVolume)
{
   iSWVolume = aVolume;    
}

TReal CScenetoneSound::GetOffset()
{
  PrivateWaitRequestOK();
  iPlayerThread.RequestComplete(iRequestPtr, SCENETONE_COMMAND_GET_OFFSET);

  // in this case we have to wait for the offset to arrive
  PrivateWaitRequestOK();

  return iOffset;
}

void CScenetoneSound::SetVolume(TInt aVolume)
{
    iVolume = aVolume;
	PrivateWaitRequestOK();
	iPlayerThread.RequestComplete(iRequestPtr, SCENETONE_COMMAND_SET_VOLUME);
}

void CScenetoneSound::PrivateGetOffset()
{
  TTimeIntervalMicroSeconds t = iStream->Position();
  iOffset = ((TReal)(t.Int64())) / (1000000.f);     // from micros to seconds
}

void CScenetoneSound::NextSubsong()
{
	PrivateWaitRequestOK();
	iPlayerThread.RequestComplete(iRequestPtr, SCENETONE_COMMAND_NEXT_SUBSONG);
}

void CScenetoneSound::PrevSubsong()
{
	PrivateWaitRequestOK();
	iPlayerThread.RequestComplete(iRequestPtr, SCENETONE_COMMAND_PREV_SUBSONG);
}

void CScenetoneSound::PrivateSetVolume()
{
    if(iState == EPlaying)
    {
	iStream->SetVolume((iStream->MaxVolume()*iVolume)/100);
    }
}

_LIT(KThreadName,"Scenetoneplaybackthread");

CCommandHandler::~CCommandHandler()
{
}

void CCommandHandler::DoCancel()
{
}


CCommandHandler::CCommandHandler() :
    CActive(CActive::EPriorityIdle)
{
}

CCommandHandler* CCommandHandler::NewL()
{
    CCommandHandler *a = new (ELeave) CCommandHandler;
    return a;
}

void CCommandHandler::Start(CScenetoneSound *aSound)
{
    iSound = aSound;
    iSound->iRequestPtr = &iStatus;

    iStatus = KRequestPending;
    SetActive();

    iSound->iKilling = EFalse;
}

void CCommandHandler::RunL(void)
{
    /* We got triggered, check command */
    switch(iStatus.Int())
    {
        case SCENETONE_COMMAND_START_PLAYBACK:
	    	// assumes: filename is ok
	    	iSound->PrivateStart();
	    	break;
        case SCENETONE_COMMAND_GENERATE_WAV:
			iSound->PrivateGenerateWav();
	    	break;
        case SCENETONE_COMMAND_STOP_PLAYBACK:
	    	iSound->PrivateStop();
	    	break;
        case SCENETONE_COMMAND_SET_VOLUME:
	    	iSound->PrivateSetVolume();
	    	break;
        case SCENETONE_COMMAND_GET_OFFSET:
	        iSound->PrivateGetOffset();
		break;
        case SCENETONE_COMMAND_EXIT:
	    	iSound->PrivateStop();
	   		iSound->iKilling = ETrue;
	    	break;
		case SCENETONE_COMMAND_NEXT_SUBSONG:
			iSound->PrivateNextSubsong();
			break;
		case SCENETONE_COMMAND_PREV_SUBSONG:
			iSound->PrivatePrevSubsong();
			break;
        case SCENETONE_COMMAND_WAIT_KILL:
	    	if(iSound->State() == CScenetoneSound::EStopped)
	    	{
				Deque();
				CActiveScheduler::Stop();

				delete iSound->iStream;
				iSound->iStream = NULL;

				return;
	    	}
    	default:
			break;
    }

    iSound->iRequestPtr = &iStatus;
    iStatus = KRequestPending;
    SetActive();

    if(iSound->iKilling)
    {
		/* wait a bit, then loop the AO again */
		User::After(10000);
		User::RequestComplete(iSound->iRequestPtr, SCENETONE_COMMAND_WAIT_KILL);
    }
}

void CScenetoneSound::PrivateWaitRequestOK()
{
	/* Dummy loop.. but works :-) */
	while((iRequestPtr == NULL) || (*iRequestPtr != KRequestPending))
	{
		User::After(10000);
	}
}


TInt serverthreadfunction(TAny *aThis)
{
    CScenetoneSound *a = (CScenetoneSound*)aThis;
	
	/* We will be using LIBC (possibly) from multiple threads.. -> use Multi-Thread mode of ESTLIB */
//	SpawnPosixServerThread();

    CTrapCleanup *ctrap = CTrapCleanup::New();

    CActiveScheduler *scheduler = new CActiveScheduler();
    CActiveScheduler::Install(scheduler);

    a->iHandler          = CCommandHandler::NewL();
    CActiveScheduler::Add(a->iHandler);

    a->iSongProviderSID    = ScenetoneCreateSIDProvider();
    a->iSongProviderMikmod = ScenetoneCreateMikmodProvider();

#if defined(SCENETONE_INCLUDE_SHINE)
    a->iMP3Writer          = ScenetoneCreateShineWriter();
#else
    a->iMP3Writer          = ScenetoneCreateWavWriter();
#endif

    a->iHandler->Start(a);
    CActiveScheduler::Start();

	// Delete objects created in this thread
	delete a->iSongProviderSID;
	delete a->iSongProviderMikmod;
	delete a->iMP3Writer;
	delete a->iHandler;
	delete scheduler;
    delete ctrap;

    return 0;
}

void CScenetoneSound::Exit()
{
    TRequestStatus  req = KRequestPending;

    iPlayerThread.Logon(req);
    iPlayerThread.RequestComplete(iRequestPtr, SCENETONE_COMMAND_EXIT);

    User::WaitForRequest(req);
    // thread died ok, we can go out now
}

void CScenetoneSound::SetVisualizer(MScenetoneVisualizer *aVisualizer)
{
	iVisualizer = aVisualizer;	
}

void CScenetoneSound::ConstructL()
{
    iVisualizer = NULL;
    iVolume = 50;

    iBuffer1 = new (ELeave) unsigned char [MIX_BUFFER_LENGTH];
    iDesc1.Set(iBuffer1, MIX_BUFFER_LENGTH, MIX_BUFFER_LENGTH);
    iBuffer2 = new (ELeave) unsigned char [MIX_BUFFER_LENGTH];
    iDesc2.Set(iBuffer2, MIX_BUFFER_LENGTH, MIX_BUFFER_LENGTH);

    iStream = NULL;
    iState  = EStopped;

    /*********************************************************************************
       Priority scheme:

       1) set owning process to high (-> more than foreground)
       2) set UI thread to be Normal
       3) set playback thread to be RealTime
     *********************************************************************************/

    RThread curthread;

    /* Spawn new thread for actual playback and command control, shares the heap with main thread */
    iPlayerThread.Create(KThreadName, serverthreadfunction, SCENETONE_SERVER_STACKSIZE, NULL, (TAny*)this);
    iPlayerThread.SetProcessPriority(EPriorityHigh);
    iPlayerThread.SetPriority(EPriorityRealTime);
    curthread.SetPriority(EPriorityLess);

    iPlayerThread.Resume();                    /* start the streaming thread */
}
