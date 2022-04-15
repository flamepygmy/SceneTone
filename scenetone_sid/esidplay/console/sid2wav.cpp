//
// /home/ms/sidplay/RCS/sid2wav.cpp,v
//
// --------------------------------------------------------------------------
//
// --- SIDPLAY to .WAV/.AU ---
//
// Copyright (c) 1994-97 Michael Schwendt and Adam Lorentzon.
// All rights reserved.
// InterNet email: <Michael_Schwendt@public.uni-hamburg.de>
//                 Adam Lorentzon   <d93-alo@nada.kth.se>
//
// Some /u-law specific code 'borrowed' from tracker 4.43 by Marc Espie.
//
// Redistribution and use  in source and  binary forms, either  unchanged or
// modified, are permitted provided that the following conditions are met:
//
// (1)  Redistributions  of  source  code  must  retain  the above copyright
// notice, this list of conditions and the following disclaimer.
//
// (2) Redistributions  in binary  form must  reproduce the  above copyright
// notice,  this  list  of  conditions  and  the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE  IS PROVIDED  BY THE  AUTHOR ``AS  IS'' AND  ANY EXPRESS OR
// IMPLIED  WARRANTIES,  INCLUDING,   BUT  NOT  LIMITED   TO,  THE   IMPLIED
// WARRANTIES OF MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR PURPOSE  ARE
// DISCLAIMED.  IN NO EVENT SHALL  THE AUTHOR OR CONTRIBUTORS BE LIABLE  FOR
// ANY DIRECT,  INDIRECT, INCIDENTAL,  SPECIAL, EXEMPLARY,  OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS
// OR SERVICES;  LOSS OF  USE, DATA,  OR PROFITS;  OR BUSINESS INTERRUPTION)
// HOWEVER  CAUSED  AND  ON  ANY  THEORY  OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING  IN
// ANY  WAY  OUT  OF  THE  USE  OF  THIS  SOFTWARE,  EVEN  IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// --------------------------------------------------------------------------

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __MSDOS__
  #include <dir.h>
#endif
#include <time.h>

#include "player.h"
#include "myendian.h"

#if !defined(IS_LITTLE_ENDIAN) && !defined(IS_BIG_ENDIAN)
#error Have to define the endianess of the cpu.
#endif

#if defined(__amigaos__)
#define EXIT_ERROR_STATUS (20)
#else
#define EXIT_ERROR_STATUS (-1)
#endif

const char s2w_version[] = "1.7.2";

enum
{
	TXT_TITLE,
    ERR_NOT_ENOUGH_MEMORY,
    ERR_SYNTAX,	ERR_ENGINE,
	ERR_ENDIANESS
};

const int FLAG_STDIN = 0x01;
const int FLAG_ULAW = 0x08;

static int flags;

struct wav_hdr                  // little endian
{
	char main_chunk[4];         // 'RIFF'
	udword length;              // filelength
	char chunk_type[4];         // 'WAVE'
	
	char sub_chunk[4];          // 'fmt '
	udword clength;             // length of sub_chunk, always 16 bytes
	uword  format;              // currently always = 1 = PCM-Code
	uword  modus;               // 1 = mono, 2 = stereo
	udword samplefreq;          // sample-frequency
	udword bytespersec;         // frequency * bytespersmpl  
	uword  bytespersmpl;        // bytes per sample; 1 = 8-bit, 2 = 16-bit
	uword  bitspersmpl;
	char data_chunk[4];         // keyword, begin of data chunk; = 'data'
	udword data_length;         // length of data
};

struct au_hdr                   // big endian
{
	char id[4];                 // '.snd'
	udword hdrlength;           // const 0x18
	udword length;              // data length
	udword format;              // 1=ulaw
	udword frequency;
	udword channels;            // 1=mono, 2=stereo
	// Cool Edit v1.50 saves au files with a header that contains an extra
	// 4 byte field at the end. It has been filled with zeros in the
	// cases I've come across. This makes hdrlength 0x1C.
};

// Default WAV header: PCM, mono, 22050 Hz, 8-bit
#if defined(IS_LITTLE_ENDIAN)
wav_hdr my_wav_hdr =
{
	{'R','I','F','F'}, 0, {'W','A','V','E'},
	{'f','m','t',' '}, 16, 1, 1, 22050, 22050, 1, 8,
	{'d','a','t','a'}, 0
};
#else
wav_hdr my_wav_hdr =
{ 
	{'R','I','F','F'}, 0, {'W','A','V','E'},
	{'f','m','t',' '}, 
	convertEndianess ((udword)16),
	convertEndianess ((uword)1),
	convertEndianess ((uword)1),
	convertEndianess ((udword)22050),
	convertEndianess ((udword)22050),
	convertEndianess ((uword)1),
	convertEndianess ((uword)8),
	{'d','a','t','a'}, 0
};
#endif

#if defined(IS_LITTLE_ENDIAN) 
au_hdr my_au_hdr =
{ 
	{'.','s','n','d'},
	convertEndianess ((udword)0x18),
	0,
	convertEndianess ((udword)1),
	convertEndianess ((udword)8000),           // 8000bytes = 0x1F40, 8012=0x1F4C
	convertEndianess ((udword)1)
};
#else
au_hdr my_au_hdr =
{
	{'.','s','n','d'}, 0x18, 0, 1, 8000, 1
};
#endif

static int fadein_seconds,
    fadein_step,
    fadein_count,
    fadein_currentlevel,
    fadeout_seconds,
    fadeout_step,
    fadeout_count,
    fadeout_currentlevel,
    fadelevel;


void error( char*, char* );
void printtext( int );

void (*fadeout_buffer)( ubyte*, udword );
void (*fadein_buffer)( ubyte*, udword );

void fadeout_buffer_8( ubyte*, udword );
void fadein_buffer_8( ubyte*, udword );
void fadeout_buffer_16( ubyte*, udword );
void fadein_buffer_16( ubyte*, udword );
void buffer2ulaw( ubyte* samplebuffer, udword samplebuffersize );
void endianswitch_buffer( ubyte* samplebuffer, udword samplebuffersize );


int main(int argc, char *argv[])
{
	// Title.
	printtext(TXT_TITLE);
	
	// ======================================================================
	// INITIALIZE THE EMULATOR ENGINE
	// ======================================================================

	// Initialize the SID-Emulator Engine to defaults.
	emuEngine myEmuEngine;
	// Everything went okay ?
	if ( !myEmuEngine )
	{
		// So far the only possible error.
		printtext(ERR_NOT_ENOUGH_MEMORY);
	}
	if ( !myEmuEngine.verifyEndianess() )
	{
		printtext(ERR_ENDIANESS);
	}
	
	struct emuConfig myEmuConfig;
	myEmuEngine.returnConfig(myEmuConfig);

	// ======================================================================
	// CONFIGURE THE EMULATOR ENGINE
	// ======================================================================

	// Defaults.
	myEmuConfig.frequency = 22050;
	myEmuConfig.channels = SIDEMU_MONO;
	myEmuConfig.bitsPerSample = SIDEMU_8BIT;
	flags = 0;
	uword selectedSong = 0;
	int seconds = 60;
	int beginSeconds = 0;
	fadein_seconds = 0;
	fadeout_seconds = 2;
	fadeout_buffer = fadeout_buffer_8;
	fadein_buffer = fadein_buffer_8;
	int muteVal = 0;

	// File argument numbers.
	int infile = 0, outfile = 0;
  
	// Parse command line arguments.
	for ( int a = 1; a < argc; a++)
	{
		if ( argv[a][0] == '-')
		{
#ifndef __MSDOS__
			// Reading from stdin.
			if ( strlen(argv[a]) == 1 )
			{
				if ( infile == 0 )
				{
					infile = a;
					flags |= FLAG_STDIN;
				}
				else
				{
					printtext(ERR_SYNTAX);
				}
				break;
			}
#endif
			if ( strncasecmp( &argv[a][1], "fout", 4 ) == 0 )
			{
				fadeout_seconds = atoi(argv[a]+5);
			}
			else if ( strncasecmp( &argv[a][1], "fin", 3 ) == 0 )
			{
				fadein_seconds = atoi(argv[a]+4);
			}
			else if ( strncasecmp( &argv[a][1], "nf", 2 ) == 0 )
			{
				myEmuConfig.emulateFilter = false;
			}
			else if ( strncasecmp( &argv[a][1], "a2", 2 ) == 0 )
			{
				myEmuConfig.memoryMode = MPU_BANK_SWITCHING;
			}
			else if ( strncasecmp( &argv[a][1], "a", 1 ) == 0 )
			{
				myEmuConfig.memoryMode = MPU_PLAYSID_ENVIRONMENT;
			}
			else if ( strncasecmp( &argv[a][1], "b", 1 ) == 0 )
			{
				beginSeconds = atoi(argv[a]+2);
			}
			else if ( strncasecmp( &argv[a][1], "f", 1 ) == 0 )
			{
				myEmuConfig.frequency = (ulong)atoi(argv[a]+2);
			}
			else if ( strncasecmp( &argv[a][1], "h", 1 ) == 0 )
			{
				printtext(ERR_SYNTAX);
			}
			else if ( strncasecmp( &argv[a][1], "m", 1 ) == 0 )
			{
				for ( ubyte j = 2; j < strlen(argv[a]); j++ )
				{
					if ( (argv[a][j]>='1') && (argv[a][j]<='4') )
					{
						muteVal |= (1 << argv[a][j]-'1');
					}
				}
				myEmuConfig.volumeControl = SIDEMU_VOLCONTROL;
			}
			else if ( strncasecmp( &argv[a][1], "n", 1 ) == 0 )
			{
				myEmuConfig.clockSpeed = SIDTUNE_CLOCK_NTSC;
				myEmuConfig.forceSongSpeed = true;
			}
			else if ( strncasecmp( &argv[a][1], "o", 1 ) == 0 )
			{
				selectedSong = atoi(argv[a]+2);
			}
			else if ( strncasecmp( &argv[a][1], "t", 1 ) == 0 )
			{
				seconds = atoi(argv[a]+2);
			}
			else if ( strncasecmp( &argv[a][1], "ss", 2 ) == 0 )
			{
				myEmuConfig.channels = SIDEMU_STEREO;
				myEmuConfig.volumeControl = SIDEMU_STEREOSURROUND;
			}
			else if ( strncasecmp( &argv[a][1], "s", 1 ) == 0 )
			{
				myEmuConfig.channels = SIDEMU_STEREO;
			}
			else if ( strncasecmp( &argv[a][1], "u", 1 ) == 0 )
			{
				flags |= FLAG_ULAW;
			}
			else if ( strncasecmp( &argv[a][1], "16", 2 ) == 0 )
			{
				myEmuConfig.bitsPerSample = SIDEMU_16BIT;
			}
			else  
			{
				printtext(ERR_SYNTAX);
			}
		}
		else
		{
			// Set filename argument number.
			if ( infile == 0 )
			{
				infile = a;
			}
			else if ( outfile == 0 )
			{
				outfile = a;
			}
			else
			{
				printtext(ERR_SYNTAX);
			}
		}
	}
	
	if ( infile == 0 )
	{
		printtext(ERR_SYNTAX); 
	}

	//
	
	if (flags & FLAG_ULAW)
	{
		myEmuConfig.frequency = 8000;          // 8000 or 8012??
		myEmuConfig.channels = SIDEMU_MONO;
		myEmuConfig.bitsPerSample = SIDEMU_16BIT;
	}
	if (myEmuConfig.bitsPerSample == SIDEMU_16BIT)
	{
		fadeout_buffer = fadeout_buffer_16;
		fadein_buffer = fadein_buffer_16;
	}

#ifdef __MSDOS__  
	char outpathname[MAXPATH];
	if ( outfile == 0 )
	{
		char indrive[MAXDRIVE], inpath[MAXDIR], inname[MAXFILE], inext[MAXEXT];
		fnsplit(argv[infile], indrive, inpath, inname, inext);
		fnmerge(outpathname, indrive, inpath, inname, ".wav");
	}
	else
	{
		strcpy(outpathname, argv[outfile]);
	}
#else
	char* outpathname;
	if ( outfile == 0 )
	{
		outpathname = new char[ strlen(argv[infile]) +4 +1];
		strcpy( outpathname, argv[infile] );
		if ( flags & FLAG_ULAW )
		{
			strcat( outpathname, ".au" );
		}
		else
		{
			strcat( outpathname, ".wav" );
		}
	}
	else
	{
		if (( outpathname = strdup( argv[outfile] )) == 0 )
		{
			printtext(ERR_NOT_ENOUGH_MEMORY);
		}
	}
#endif

	// Create the sidtune object.
	sidTune myTune( argv[infile] );
	struct sidTuneInfo mySidInfo;
	myTune.returnInfo( mySidInfo );
	if ( !myTune )  
	{
		cerr << mySidInfo.statusString << endl;
		exit(EXIT_ERROR_STATUS);
	}
	else
	{
		cout << "File format  : " << mySidInfo.formatString << endl;
		cout << "Condition    : " << mySidInfo.statusString << endl;
		if ( mySidInfo.numberOfInfoStrings == 3 )
		{
			cout << "Name         : " << mySidInfo.nameString << endl;
			cout << "Author       : " << mySidInfo.authorString << endl;
			cout << "Copyright    : " << mySidInfo.copyrightString << endl;
		}
		else
		{
			for ( int infoi = 0; infoi < mySidInfo.numberOfInfoStrings; infoi++ )
			{
				cout << "Description  : " << mySidInfo.infoString[infoi] << endl;
			}
		}
		cout << "Load address : $" << hex << setw(4) << setfill('0') 
			<< mySidInfo.loadAddr << endl;
		cout << "Init address : $" << hex << setw(4) << setfill('0') 
			<< mySidInfo.initAddr << endl;
		cout << "Play address : $" << hex << setw(4) << setfill('0') 
			<< mySidInfo.playAddr << dec << endl;
	}
	
	// Alter the SIDPLAY Emulator Engine settings.
	myEmuConfig.sampleFormat = (myEmuConfig.bitsPerSample == SIDEMU_16BIT) ?  SIDEMU_SIGNED_PCM : SIDEMU_UNSIGNED_PCM;
	myEmuEngine.setConfig(myEmuConfig);
	// Here mute the voices, if requested.
	if (myEmuConfig.volumeControl == SIDEMU_VOLCONTROL)
	{
		for ( int voice = 1; voice <= 4; voice++ )
		{
			if ( (muteVal & (1<<(voice-1))) != 0 )
			{
				myEmuEngine.setVoiceVolume(voice,0,0,0);
			}
		}
	}
	// Get the current settings. We ignore the return value, because this code is
	// supposed to allow only valid settings.
	myEmuEngine.returnConfig(myEmuConfig);
    // Print the relevant settings.
	cout << "SID Filter   : " << ((myEmuConfig.emulateFilter == true) ? "Yes" : "No") << endl;
	if (myEmuConfig.memoryMode == MPU_PLAYSID_ENVIRONMENT)
	{
		cout << "Memory mode  : PlaySID (this is supposed to fix PlaySID-specific rips)" << endl;
	}
	else if (myEmuConfig.memoryMode == MPU_TRANSPARENT_ROM)
	{
		cout << "Memory mode  : Transparent ROM (SIDPLAY default)" << endl;
	}
	else if (myEmuConfig.memoryMode == MPU_BANK_SWITCHING)
	{
		cout << "Memory mode  : Bank Switching" << endl;
	}
    cout << "Frequency    : " << dec << myEmuConfig.frequency << " Hz" << endl;
	cout << "Bits/sample  : ";
	if (flags & FLAG_ULAW)
	{
		cout << "8 (u-law)" << endl;
	}
	else
	{
	    cout << dec << myEmuConfig.bitsPerSample << endl;
	}
	cout << "Channels     : " << ((myEmuConfig.channels == SIDEMU_MONO) ? "Mono" : "Stereo") << endl;
  
	myTune.setInfo( mySidInfo );

	if ( !sidEmuInitializeSong(myEmuEngine,myTune,selectedSong) )
	{
		cerr << "ERROR: SID Emulator Engine components not ready" << endl;
		exit(EXIT_ERROR_STATUS);
	}
	myTune.returnInfo( mySidInfo );
	if ( !myTune )
	{
		cerr << mySidInfo.statusString;
		exit(EXIT_ERROR_STATUS);
	}
	cout << "Setting song : " << mySidInfo.currentSong
		<< " out of " << mySidInfo.songs
		<< " (default = " << mySidInfo.startSong << ')' << endl;
	cout << "Song speed   : " << mySidInfo.speedString << endl;

	cout << "WAV length   : " << seconds << " second";
	if ( seconds > 1 )
	{
		cout << 's';
	}
	cout << endl;

	// Open output file stream.
	ofstream tofile( outpathname, ios::out | ios::bin | ios::trunc | ios::noreplace );
	if ( !tofile )
	{
		cerr << "ERROR: Cannot create output file " << "'" << outpathname << "', "
			<< "probably already exits" << endl;
		exit(EXIT_ERROR_STATUS);
	}
	cout << "Output file  : " << outpathname << endl;

	udword samplebuffersize;
	udword datalength;
	uword  headersize;
	
	if (flags & FLAG_ULAW)
	{
		datalength = seconds * myEmuConfig.frequency;
#if defined(IS_LITTLE_ENDIAN)
		my_au_hdr.length = convertEndianess (datalength);
#else
		my_au_hdr.length = datalength;
#endif
		samplebuffersize = myEmuConfig.frequency * 2;
		headersize = sizeof(au_hdr);
		tofile.write( &my_au_hdr, headersize );
	}
	else
	{
		udword bytesPerSample = myEmuConfig.channels*myEmuConfig.bitsPerSample/8;
		udword bytesPerSecond = myEmuConfig.frequency*bytesPerSample;
		datalength = seconds * bytesPerSecond;
		samplebuffersize = bytesPerSecond;
		headersize = sizeof(wav_hdr);
#if defined (IS_LITTLE_ENDIAN)
		my_wav_hdr.bitspersmpl = myEmuConfig.bitsPerSample;
		my_wav_hdr.bytespersmpl = bytesPerSample;
		my_wav_hdr.samplefreq = myEmuConfig.frequency;
		my_wav_hdr.modus = myEmuConfig.channels;
		my_wav_hdr.bytespersec = bytesPerSecond;
		my_wav_hdr.length = datalength + headersize - 8;
		my_wav_hdr.data_length = datalength;
#else    
		my_wav_hdr.bitspersmpl = convertEndianess ((uword)myEmuConfig.bitsPerSample);
		my_wav_hdr.bytespersmpl = convertEndianess ((uword)bytesPerSample);
		my_wav_hdr.samplefreq = convertEndianess (myEmuConfig.frequency);
		my_wav_hdr.modus = convertEndianess ((uword)myEmuConfig.channels);
		my_wav_hdr.bytespersec = convertEndianess ((udword)bytesPerSecond);
		my_wav_hdr.length = convertEndianess (datalength + headersize - 8);
		my_wav_hdr.data_length = convertEndianess (datalength);
#endif    
		tofile.write( &my_wav_hdr, headersize );
	}
  
	// Make a buffer that holds 1 second of audio data.
	ubyte* samplebuffer = new ubyte[samplebuffersize];
	if ( samplebuffer == 0 )
	{
		printtext(ERR_NOT_ENOUGH_MEMORY);
	}
	
	// Calculate fadein and -out variables.
	if ( seconds == 0 )
	{
		seconds = 60; // force the default
	}
	if ( seconds < fadein_seconds )
	{
		fadein_seconds = seconds /2;
	}
	if ( seconds < fadeout_seconds )
	{
		fadeout_seconds = seconds /2;
	}
	if (( fadein_seconds + fadeout_seconds  ) > seconds )
	{
		fadein_seconds = 0;
		fadeout_seconds = 0;
	}
	fadein_currentlevel = 0;
	fadelevel = ( fadeout_currentlevel = 128 );
	fadein_step = ( fadein_seconds * samplebuffersize / (myEmuConfig.bitsPerSample/8) ) / fadelevel;
	fadeout_step = ( fadeout_seconds * samplebuffersize / (myEmuConfig.bitsPerSample/8) ) / fadelevel;
	fadeout_count = ( fadein_count = 0 );

	cout << endl;
	cout << "Generating sample data...don't interrupt !" << endl;
	
	if (beginSeconds > 0)
		cout << "Skipping seconds : ";
	int skipped = 0;
	while (skipped < beginSeconds)
	{
		sidEmuFillBuffer( myEmuEngine, myTune, samplebuffer, samplebuffersize );
		skipped++;
		// Print progress report.
		cout << setw(5) << setfill(' ') << skipped << "\b\b\b\b\b" << flush;
	};
	if (beginSeconds > 0)
		cout << endl;
	
	cout << "Length of output file (bytes) : ";

	datalength = 0;

	for ( int sec = 0; sec < seconds; sec++ )
	{
		sidEmuFillBuffer( myEmuEngine, myTune, samplebuffer, samplebuffersize );
		if ( sec < ( fadein_seconds ))
			(*fadein_buffer)( samplebuffer, samplebuffersize );
		if ( sec >= ( seconds - fadeout_seconds ))
			(*fadeout_buffer)( samplebuffer, samplebuffersize );
		if (flags & FLAG_ULAW)
		{
			buffer2ulaw(samplebuffer, samplebuffersize);
			tofile.write( samplebuffer, samplebuffersize / 2 );
		} 
		else
		{
#if defined(IS_BIG_ENDIAN)
			if (myEmuConfig.bitsPerSample == SIDEMU_16BIT)
			{
				endianswitch_buffer( samplebuffer, samplebuffersize );
			}
#endif
			tofile.write( samplebuffer, samplebuffersize );
		}
	  
		// Print progress report.
		cout << setw(10) << setfill(' ') << ( datalength + headersize) << "\b\b\b\b\b\b\b\b\b\b" << flush;
	
		if (flags & FLAG_ULAW)
			datalength += samplebuffersize / 2;
		else
			datalength += samplebuffersize;
	}
	// Finish progress report.
	cout << setw(10) << setfill(' ') << ( datalength + headersize) << endl;
	tofile.close();
	delete[] outpathname;
	delete[] samplebuffer;

	cout << endl << "Please do not forget to give the credits whenever using a waveform created" << endl
		<< "by this application !" << endl << endl;

	return 0;
}


void fadeout_buffer_8( ubyte* samplebuffer, udword samplebuffersize )
{
	for ( udword i = 0; i < samplebuffersize; i++ )
    {
		sbyte sam = (sbyte)( 0x80 ^ *(samplebuffer +i));
		sword modsam = sam * fadeout_currentlevel;
		modsam /= fadelevel;
		sam = (sbyte)modsam;
		*(samplebuffer +i) = 0x80 ^ sam;
		
		fadeout_count++;
		if ( fadeout_count >= fadeout_step )
		{
			if ( fadeout_currentlevel > 0 )
			{
				fadeout_currentlevel--;
			}
			fadeout_count = 0;
		}
	}
}

void fadeout_buffer_16( ubyte* samplebuffer, udword samplebuffersize )
{
	sword *buf = (sword *)samplebuffer;
	samplebuffersize /= 2;
	for ( udword i = 0; i < samplebuffersize; i++ )
	{
		sword sam = *(buf +i);
		sdword modsam = sam * fadeout_currentlevel;
		modsam /= fadelevel;
		sam = (sword)modsam;
		*(buf +i) = sam;
		
		fadeout_count++;
		if ( fadeout_count >= fadeout_step )
		{
			if ( fadeout_currentlevel > 0 )
			{
				fadeout_currentlevel--;
			}
			fadeout_count = 0;
		}
	}
}

void fadein_buffer_8( ubyte* samplebuffer, udword samplebuffersize )
{
	for ( udword i = 0; i < samplebuffersize; i++ )
	{
		sbyte sam = (sbyte)( 0x80 ^ *(samplebuffer +i));
		sword modsam = sam * fadein_currentlevel;
		modsam /= fadelevel;
		sam = (sbyte)modsam;
		*(samplebuffer +i) = 0x80 ^ sam;
		
		fadein_count++;
		if ( fadein_count >= fadein_step )
		{
			if ( fadein_currentlevel < fadelevel )
			{
				fadein_currentlevel++;
			}
			fadein_count = 0;
		}
	}
}

void fadein_buffer_16( ubyte* samplebuffer, udword samplebuffersize )
{
	sword *buf = (sword *)samplebuffer;
	samplebuffersize /= 2;
	for ( udword i = 0; i < samplebuffersize; i++ )
	{
		sword sam = *(buf +i);
		sdword modsam = sam * fadein_currentlevel;
		modsam /= fadelevel;
		sam = (sword)modsam;
		*(buf +i) = sam;
		
		fadein_count++;
		if ( fadein_count >= fadein_step )
		{
			if ( fadein_currentlevel < fadelevel )
			{
				fadein_currentlevel++;
			}
			fadein_count = 0;
		}
	}
}

void error(char* s1, char* s2 = "")
{
	cerr << "ERROR: " << s1 << ' ' << "'" << s2 << "'" << endl;
	exit(EXIT_ERROR_STATUS);
}


void printtext(int number)
{
	switch (number)  
	{
	 case TXT_TITLE:
		{
			cout << "SID2WAV   Synthetic Waveform Generator   " << "Portable Version " << s2w_version << "/" << emu_version << endl
				<< "Copyright (c) 1994-97   All rights reserved." << endl
				<< "Authors: Michael Schwendt <sidplay@geocities.com>" << endl
				<< "         Adam Lorentzon   <d93-alo@nada.kth.se>" << endl
#if defined(__amigaos__)
				<< "AmigaOS port: <phillwooller@geocities.com>" << endl
#endif
				<< endl;
			break;
		}
	 case ERR_ENDIANESS:
		{
			cerr << "ERROR: Hardware endianess improperly configured." << endl;
			exit(EXIT_ERROR_STATUS);
			break;
		}
	 case ERR_ENGINE:  // currently the only reason the engine would fail
	 case ERR_NOT_ENOUGH_MEMORY:
		{
			cerr << "ERROR: Not enough memory" << endl;
			exit(EXIT_ERROR_STATUS);
			break;
		}
	 case ERR_SYNTAX:
		{
#ifdef __MSDOS__
			cout << " syntax: sid2wav [-<commands>] <datafile> [outputfile]" << endl
#else
			cout << " syntax: sid2wav [-<commands>] <datafile>|- [outputfile]" << endl
#endif
				<< " commands: -h         display this screen" << endl
				<< "           -f<num>    set frequency in Hz (default: 22050)" << endl
				<< "           -16        16-bit (default: 8-bit)" << endl
				<< "           -s         stereo (default: mono)" << endl
				<< "           -ss        enable stereo surround" << endl
				<< "           -u         au output (8000Hz mono 8-bit u-law)" << endl
				<< "           -o<num>    set song number (default: preset)" << endl
				<< "           -a         improve PlaySID compatibility (not recommended)" << endl
				<< "           -a2        bank switching mode (overrides -a)" << endl
				<< "           -nf        no SID filter emulation" << endl
				<< "           -n         enable NTSC-clock speed for VBI tunes (not recommended)" << endl
				<< "           -m<num>    mute voices out of 1,2,3,4 (default: none)" << endl
				<< "                      example: -m13 (voice 1 and 3 off)" << endl
				<< "           -t<num>    set seconds to play (default: 60)" << endl
				<< "           -b<num>    begin <num> seconds into the song (default: 0)" << endl
				<< "           -fin<num>  fade-in-time in seconds (default: 0)" << endl
				<< "           -fout<num> fade-out-time in seconds (default: 2)" << endl
				<< endl;
			exit(EXIT_ERROR_STATUS);
			break;
		}
	 default:
		{
			cerr << "ERROR: Internal system error" << endl;
			exit(EXIT_ERROR_STATUS);
			break;
		}
	}
}















// ------------ Beginning of code 'borrowed' from tracker 4.43 by Marc Espie.

// The only modifications to the code were a few changes from C style
// to C++ style to please the compiler.


short seg_end[8] = 
{
	0xFF, 0x1FF, 0x3FF, 0x7FF,
	0xFFF, 0x1FFF, 0x3FFF, 0x7FFF
};

int search(int val, short *table, int size)
{
	int	i;
	
	for (i = 0; i < size; i++) 
	{
		if (val <= *table++)
		{
			return i;
		}
	}
	return size;
}

const int BIAS = 0x84;		    // Bias for linear code.

// linear2ulaw() - Convert a linear PCM value to u-law
//
// In order to simplify the encoding process, the original linear magnitude
// is biased by adding 33 which shifts the encoding range from (0 - 8158) to
// (33 - 8191). The result can be seen in the following encoding table:
//
//	Biased Linear Input Code	Compressed Code
//	------------------------	---------------
//	00000001wxyza			000wxyz
//	0000001wxyzab			001wxyz
//	000001wxyzabc			010wxyz
//	00001wxyzabcd			011wxyz
//	0001wxyzabcde			100wxyz
//	001wxyzabcdef			101wxyz
//	01wxyzabcdefg			110wxyz
//	1wxyzabcdefgh			111wxyz
//
// Each biased linear code has a leading 1 which identifies the segment
// number. The value of the segment number is equal to 7 minus the number
// of leading 0's. The quantization interval is directly available as the
// four bits wxyz. // The trailing bits (a - h) are ignored.
//
// Ordinarily the complement of the resulting code word is used for
// transmission, and so the code word is complemented before it is returned.
//
// For further information see John C. Bellamy's Digital Telephony, 1982,
// John Wiley & Sons, pps 98-111 and 472-476.

unsigned char linear2ulaw(int pcm_val)
// int pcm_val;	// 2's complement (16-bit range)
{
	int	mask;
	int	seg;
	unsigned char uval;
	
	// Get the sign and the magnitude of the value.
	if (pcm_val < 0) 
    {
		pcm_val = BIAS - pcm_val;
		mask = 0x7F;
    }
	else 
    {
		pcm_val += BIAS;
		mask = 0xFF;
    }
  
	// Convert the scaled magnitude to segment number.
	seg = search(pcm_val, seg_end, 8);
  
	// Combine the sign, segment, quantization bits;
	// and complement the code word.

	if (seg >= 8)		// out of range, return maximum value.
	{
		return 0x7F ^ mask;
	}
	else
	{
		uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0xF);
		return uval ^ mask;
	}
}

//
// ------------------- End of code 'borrowed' from tracker 4.43 by Marc Espie













//
// Assume incoming data is 16-bit signed values.
// Samplebuffersize is the size in bytes of the buffer samplebuffer
//
void buffer2ulaw( ubyte* samplebuffer, udword samplebuffersize )
{
	sword *wordbuffer = (sword *) samplebuffer;
	udword numsamples = samplebuffersize / 2;
	for (udword i = 0; i < numsamples; i++)
	{
		samplebuffer[i] = linear2ulaw ((~wordbuffer[i]) + 1);  // two's complement
	}
}


//
// Incoming data is 16-bit values which needs an endian-switch
// Samplebuffersize is the size in bytes of the buffer samplebuffer
//
void endianswitch_buffer( ubyte* samplebuffer, udword samplebuffersize )
{
	uword *wordbuffer = (uword *) samplebuffer;
	udword numsamples = samplebuffersize / 2;
	for (udword i = 0; i < numsamples; i++)
	{
		wordbuffer[i] = convertEndianess (wordbuffer[i]);
	}
}
