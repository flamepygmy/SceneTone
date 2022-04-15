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

#ifndef __SCENETONESOUND_H__
#define __SCENETONESOUND_H__

#define SCENETONE_PYTHON_COMMAND_SET_FILENAME   1
#define SCENETONE_PYTHON_COMMAND_PLAY           2
#define SCENETONE_PYTHON_COMMAND_STOP           3

#include "scenetoneinterfaces.h"
#include <aknglobalprogressdialog.h>
#include <aknquerydialog.h>

#define MIX_BUFFER_TIMES                                     8                // mix in 8 smaller passes
#define MIX_BUFFER_SAMPLES_IN_ONE_STEP                     2048
#define MIX_BUFFER_LENGTH               MIX_BUFFER_SAMPLES_IN_ONE_STEP*MIX_BUFFER_TIMES*4

// INCLUDES
#include <mda/common/audio.h>
#include <mdaaudiooutputstream.h>
#include <e32std.h>

#define SCENETONE_COMMAND_NONE            0
#define SCENETONE_COMMAND_START_PLAYBACK  1
#define SCENETONE_COMMAND_GENERATE_WAV    2
#define SCENETONE_COMMAND_STOP_PLAYBACK   3
#define SCENETONE_COMMAND_SET_VOLUME      4
#define SCENETONE_COMMAND_EXIT            5
#define SCENETONE_COMMAND_WAIT_KILL       6
#define SCENETONE_COMMAND_NEXT_SUBSONG    7
#define SCENETONE_COMMAND_PREV_SUBSONG    8
#define SCENETONE_COMMAND_GET_OFFSET      9

#define SCENETONE_SERVER_STACKSIZE        65536
class CScenetoneSound;

class CCommandHandler : public CActive
{
public:
        IMPORT_C static CCommandHandler* NewL();
	IMPORT_C ~CCommandHandler();
        void Start(CScenetoneSound *aSound);
        void DoCancel();

	IMPORT_C CCommandHandler();
	IMPORT_C void RunL();
private:
     CScenetoneSound *iSound;
};

/***********************************
 * playback:
 *
 * 1) construct
 * 2) setfilename
 * 3) startl
 **********************************/

// CLASS DECLARATION
class CScenetoneSound : public CBase, MMdaAudioOutputStreamCallback
{
    public:
      enum
      {
	EStopped  = 0,
	EStarting,
	EPlaying,
	EStopping
      };
    
    public: // New methods
        static CScenetoneSound* NewL( void );
        virtual ~CScenetoneSound();

        virtual void MaoscOpenComplete(TInt aError);
        virtual void MaoscBufferCopied(TInt aError, const TDesC8 &aBuffer);
        virtual void MaoscPlayComplete(TInt aError);

        void StartL();
        void SetFileName(const TDesC16 &aFilename);
        void StopL();
	TReal GetOffset();

        void SetVolume(TInt aVolume);
        void SetSWVolume(TInt aVolume);
		TInt GetCurrentSubsong();
		TInt GetMaxSubsong();
		void NextSubsong();
		void PrevSubsong();
		void SetVisualizer(MScenetoneVisualizer *aVisualizer);
		
		void PrivateWaitRequestOK();
        void PrivateStart();
        void PrivateStop();
        void PrivateGenerateWav();
        void PrivateSetVolume();
        void PrivateNextSubsong();
        void PrivatePrevSubsong();
	void PrivateGetOffset();
        void Exit();

    void Message(const RMessage2& aMessage);
        void InitializeGenerateWavL(const TDesC16 &aModFilename, const TDesC16 &aWavFilename);
        TInt GenerateWav(const TDesC16 &aOutFileName, TReal aStart, TReal aEnd);
        TInt State();
        void MixNSamples(TInt aBytes, char *aOut);

        RThread iPlayerThread;
        TRequestStatus *iRequestPtr;
        TBool                   iKilling;

	TReal                    iOffset;
        unsigned char 	       *iBuffer1;
        unsigned char 	       *iBuffer2;
        TInt                    iBufferToMix;
        TInt                    iMixStep;
        CIdle                  *iIdle;
        TBool                   iIdleActive;
        TPtr8		        	iDesc1;
        TPtr8		        	iDesc2;
        TBool                   iStartOnNext;

        CMdaAudioOutputStream      *iStream;
        TMdaAudioDataSettings      iSettings;
        TBuf16<KMaxFileName>        iFilename;
        TBuf16<KMaxFileName>        iOutFilename;
        TInt iState;
        TInt iVolume;
        
        TInt iWavGenCount;
        TInt iWavGenSamples;
        TInt iWavGenBytes;
	TInt iWavGenStartByte;
        
        TInt iMaxSubsong;
        TInt iCurrentSubsong;

        CCommandHandler            *iHandler;

	 	MScenetoneSongProvider *iSongProviderSID;
	    MScenetoneSongProvider *iSongProviderMikmod;
	    MScenetoneSongProvider *iSongProvider;
		MScenetoneFileWriter   *iMP3Writer;
		
		MScenetoneVisualizer   *iVisualizer;

		CAknGlobalProgressDialog *iProgressDialog;
		
		TInt iSWVolume;
		  
    private: // Constructors

        void ConstructL();
 
        CScenetoneSound();
};


#endif // __SCENETONESOUND_H__
