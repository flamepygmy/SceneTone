//
// 1997/09/27 21:31:51
// 
// --------------------------------------------------------------------------
// Copyright (c) 1994-1997 Michael Schwendt. All rights reserved.
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
//
// MOS-6510 Interpreter. Known bugs, missing features, incompatibilities:
//
//  - No support for code execution in Basic-ROM/Kernal-ROM.
//    Only execution of code in RAM is allowed.
//  - Probably inconsistent emulation of illegal instructions part 3.
//  - No detection of deadlocks.
//  - Faked RTI (= RTS).
//  - Anybody knows, whether it is ``Kernel'' instead of ``Kernal'' ?
//    Perhaps it is a proper name invented by CBM ?
//    It is spelled ``Kernal'' in nearly every C64 documentation !
//

#include "6510_.h"
#include "emucfg.h"

#ifdef __SYMBIAN32__
#include <e32base.h>
#else
#include <stdlib.h>
#endif

//inline void RTS_();               // proto





C6510::C6510()
/**
 * C'tor
 */
	:stackIsOkay(true)
	,memoryMode(MPU_TRANSPARENT_ROM)
	,iFakeRndSeed(0)
	{
	CTOR(C6510);

	// no need for initialising member data to zero here.
	// that is done by the CBase class

	// Use pointers to allow plain-memory modifications.
	readData = &C6510::readData_bs;
	writeData = &C6510::writeData_bs;

// --------------------------------------------------------------------------


	//
	// initialise 6510 instructions array
	//
	int i=0;
	 
	instrList[i++] = &C6510::BRK_;
	instrList[i++] = &C6510::ORA_indx;
	instrList[i++] = &C6510::ILL_TILT;
	instrList[i++] = &C6510::ASLORA_indx;
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::ORA_zp;
	instrList[i++] = &C6510::ASL_zp;
	instrList[i++] = &C6510::ASLORA_zp;
	instrList[i++] = &C6510::PHP_;
	instrList[i++] = &C6510::ORA_imm;
	instrList[i++] = &C6510::ASL_AC;
	instrList[i++] = &C6510::ILL_0B;
	instrList[i++] = &C6510::ILL_3NOP;
	instrList[i++] = &C6510::ORA_abso;
	instrList[i++] = &C6510::ASL_abso;
	instrList[i++] = &C6510::ASLORA_abso;
	instrList[i++] = &C6510::BPL_;
	instrList[i++] = &C6510::ORA_indy;
	instrList[i++] = &C6510::ILL_TILT;
	instrList[i++] = &C6510::ASLORA_indy;
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::ORA_zpx;
	instrList[i++] = &C6510::ASL_zpx;
	instrList[i++] = &C6510::ASLORA_zpx;
	instrList[i++] = &C6510::CLC_;
	instrList[i++] = &C6510::ORA_absy;
	instrList[i++] = &C6510::ILL_1NOP;
	instrList[i++] = &C6510::ASLORA_absy;
	instrList[i++] = &C6510::ILL_3NOP;
	instrList[i++] = &C6510::ORA_absx;
	instrList[i++] = &C6510::ASL_absx;
	instrList[i++] = &C6510::ASLORA_absx;
	instrList[i++] = &C6510::JSR_;
	instrList[i++] = &C6510::AND_indx;
	instrList[i++] = &C6510::ILL_TILT;
	instrList[i++] = &C6510::ROLAND_indx;
	instrList[i++] = &C6510::BIT_zp;
	instrList[i++] = &C6510::AND_zp;
	instrList[i++] = &C6510::ROL_zp;
	instrList[i++] = &C6510::ROLAND_zp;
	instrList[i++] = &C6510::PLP_;
	instrList[i++] = &C6510::AND_imm;
	instrList[i++] = &C6510::ROL_AC;
	instrList[i++] = &C6510::ILL_0B;
	instrList[i++] = &C6510::BIT_abso;
	instrList[i++] = &C6510::AND_abso;
	instrList[i++] = &C6510::ROL_abso;
	instrList[i++] = &C6510::ROLAND_abso;
	instrList[i++] = &C6510::BMI_;
	instrList[i++] = &C6510::AND_indy;
	instrList[i++] = &C6510::ILL_TILT;
	instrList[i++] = &C6510::ROLAND_indy;
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::AND_zpx;
	instrList[i++] = &C6510::ROL_zpx;
	instrList[i++] = &C6510::ROLAND_zpx;
	instrList[i++] = &C6510::SEC_;
	instrList[i++] = &C6510::AND_absy;
	instrList[i++] = &C6510::ILL_1NOP;
	instrList[i++] = &C6510::ROLAND_absy;
	instrList[i++] = &C6510::ILL_3NOP;
	instrList[i++] = &C6510::AND_absx;
	instrList[i++] = &C6510::ROL_absx;
	instrList[i++] = &C6510::ROLAND_absx,
	// 0x40
	instrList[i++] = &C6510::RTI_;
	instrList[i++] = &C6510::EOR_indx;
	instrList[i++] = &C6510::ILL_TILT;
	instrList[i++] = &C6510::LSREOR_indx;
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::EOR_zp;
	instrList[i++] = &C6510::LSR_zp;
	instrList[i++] = &C6510::LSREOR_zp;
	instrList[i++] = &C6510::PHA_;
	instrList[i++] = &C6510::EOR_imm;
	instrList[i++] = &C6510::LSR_AC;
	instrList[i++] = &C6510::ILL_4B;
	instrList[i++] = &C6510::JMP_;
	instrList[i++] = &C6510::EOR_abso;
	instrList[i++] = &C6510::LSR_abso;
	instrList[i++] = &C6510::LSREOR_abso;
	instrList[i++] = &C6510::BVC_;
	instrList[i++] = &C6510::EOR_indy;
	instrList[i++] = &C6510::ILL_TILT;
	instrList[i++] = &C6510::LSREOR_indy;
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::EOR_zpx;
	instrList[i++] = &C6510::LSR_zpx;
	instrList[i++] = &C6510::LSREOR_zpx;
	instrList[i++] = &C6510::CLI_;
	instrList[i++] = &C6510::EOR_absy;
	instrList[i++] = &C6510::ILL_1NOP;
	instrList[i++] = &C6510::LSREOR_absy;
	instrList[i++] = &C6510::ILL_3NOP;
	instrList[i++] = &C6510::EOR_absx;
	instrList[i++] = &C6510::LSR_absx;
	instrList[i++] = &C6510::LSREOR_absx;
	instrList[i++] = &C6510::RTS_;
	instrList[i++] = &C6510::ADC_indx;
	instrList[i++] = &C6510::ILL_TILT;
	instrList[i++] = &C6510::RORADC_indx;
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::ADC_zp;
	instrList[i++] = &C6510::ROR_zp;
	instrList[i++] = &C6510::RORADC_zp;
	instrList[i++] = &C6510::PLA_;
	instrList[i++] = &C6510::ADC_imm;
	instrList[i++] = &C6510::ROR_AC;
	instrList[i++] = &C6510::ILL_6B;
	instrList[i++] = &C6510::JMP_vec;
	instrList[i++] = &C6510::ADC_abso;
	instrList[i++] = &C6510::ROR_abso;
	instrList[i++] = &C6510::RORADC_abso;
	instrList[i++] = &C6510::BVS_;
	instrList[i++] = &C6510::ADC_indy;
	instrList[i++] = &C6510::ILL_TILT;
	instrList[i++] = &C6510::RORADC_indy;
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::ADC_zpx;
	instrList[i++] = &C6510::ROR_zpx;
	instrList[i++] = &C6510::RORADC_zpx;
	instrList[i++] = &C6510::SEI_;
	instrList[i++] = &C6510::ADC_absy;
	instrList[i++] = &C6510::ILL_1NOP;
	instrList[i++] = &C6510::RORADC_absy;
	instrList[i++] = &C6510::ILL_3NOP;
	instrList[i++] = &C6510::ADC_absx;
	instrList[i++] = &C6510::ROR_absx;
	instrList[i++] = &C6510::RORADC_absx;
	// 0x80
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::STA_indx;
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::ILL_83;
	instrList[i++] = &C6510::STY_zp;
	instrList[i++] = &C6510::STA_zp;
	instrList[i++] = &C6510::STX_zp;
	instrList[i++] = &C6510::ILL_87;
	instrList[i++] = &C6510::DEY_;
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::TXA_;
	instrList[i++] = &C6510::ILL_8B;
	instrList[i++] = &C6510::STY_abso;
	instrList[i++] = &C6510::STA_abso;
	instrList[i++] = &C6510::STX_abso;
	instrList[i++] = &C6510::ILL_8F;
	instrList[i++] = &C6510::BCC_;
	instrList[i++] = &C6510::STA_indy;
	instrList[i++] = &C6510::ILL_TILT;
	instrList[i++] = &C6510::ILL_93;
	instrList[i++] = &C6510::STY_zpx;
	instrList[i++] = &C6510::STA_zpx;
	instrList[i++] = &C6510::STX_zpy;
	instrList[i++] = &C6510::ILL_97;
	instrList[i++] = &C6510::TYA_;
	instrList[i++] = &C6510::STA_absy;
	instrList[i++] = &C6510::TXS_;
	instrList[i++] = &C6510::ILL_9B;
	instrList[i++] = &C6510::ILL_9C;
	instrList[i++] = &C6510::STA_absx;
	instrList[i++] = &C6510::ILL_9E;
	instrList[i++] = &C6510::ILL_9F;
	instrList[i++] = &C6510::LDY_imm;
	instrList[i++] = &C6510::LDA_indx;
	instrList[i++] = &C6510::LDX_imm;
	instrList[i++] = &C6510::ILL_A3;
	instrList[i++] = &C6510::LDY_zp;
	instrList[i++] = &C6510::LDA_zp;
	instrList[i++] = &C6510::LDX_zp;
	instrList[i++] = &C6510::ILL_A7;
	instrList[i++] = &C6510::TAY_;
	instrList[i++] = &C6510::LDA_imm;
	instrList[i++] = &C6510::TAX_;
	instrList[i++] = &C6510::ILL_1NOP;
	instrList[i++] = &C6510::LDY_abso;
	instrList[i++] = &C6510::LDA_abso;
	instrList[i++] = &C6510::LDX_abso;
	instrList[i++] = &C6510::ILL_AF;
	instrList[i++] = &C6510::BCS_;
	instrList[i++] = &C6510::LDA_indy;
	instrList[i++] = &C6510::ILL_TILT;
	instrList[i++] = &C6510::ILL_B3;
	instrList[i++] = &C6510::LDY_zpx;
	instrList[i++] = &C6510::LDA_zpx;
	instrList[i++] = &C6510::LDX_zpy;
	instrList[i++] = &C6510::ILL_B7;
	instrList[i++] = &C6510::CLV_;
	instrList[i++] = &C6510::LDA_absy;
	instrList[i++] = &C6510::TSX_;
	instrList[i++] = &C6510::ILL_BB;
	instrList[i++] = &C6510::LDY_absx;
	instrList[i++] = &C6510::LDA_absx;
	instrList[i++] = &C6510::LDX_absy;
	instrList[i++] = &C6510::ILL_BF;
	// 0xC0
	instrList[i++] = &C6510::CPY_imm;
	instrList[i++] = &C6510::CMP_indx;
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::DECCMP_indx;
	instrList[i++] = &C6510::CPY_zp;
	instrList[i++] = &C6510::CMP_zp;
	instrList[i++] = &C6510::DEC_zp;
	instrList[i++] = &C6510::DECCMP_zp;
	instrList[i++] = &C6510::INY_;
	instrList[i++] = &C6510::CMP_imm;
	instrList[i++] = &C6510::DEX_;
	instrList[i++] = &C6510::ILL_CB;
	instrList[i++] = &C6510::CPY_abso;
	instrList[i++] = &C6510::CMP_abso;
	instrList[i++] = &C6510::DEC_abso;
	instrList[i++] = &C6510::DECCMP_abso;
	instrList[i++] = &C6510::BNE_;
	instrList[i++] = &C6510::CMP_indy;
	instrList[i++] = &C6510::ILL_TILT;
	instrList[i++] = &C6510::DECCMP_indy;
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::CMP_zpx;
	instrList[i++] = &C6510::DEC_zpx;
	instrList[i++] = &C6510::DECCMP_zpx;
	instrList[i++] = &C6510::CLD_;
	instrList[i++] = &C6510::CMP_absy;
	instrList[i++] = &C6510::ILL_1NOP;
	instrList[i++] = &C6510::DECCMP_absy;
	instrList[i++] = &C6510::ILL_3NOP;
	instrList[i++] = &C6510::CMP_absx;
	instrList[i++] = &C6510::DEC_absx;
	instrList[i++] = &C6510::DECCMP_absx;
	instrList[i++] = &C6510::CPX_imm;
	instrList[i++] = &C6510::SBC_indx;
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::INCSBC_indx;
	instrList[i++] = &C6510::CPX_zp;
	instrList[i++] = &C6510::SBC_zp;
	instrList[i++] = &C6510::INC_zp;
	instrList[i++] = &C6510::INCSBC_zp;
	instrList[i++] = &C6510::INX_;
	instrList[i++] = &C6510::SBC_imm;
	instrList[i++] = &C6510::NOP_;
	instrList[i++] = &C6510::ILL_EB;
	instrList[i++] = &C6510::CPX_abso;
	instrList[i++] = &C6510::SBC_abso;
	instrList[i++] = &C6510::INC_abso;
	instrList[i++] = &C6510::INCSBC_abso;
	instrList[i++] = &C6510::BEQ_;
	instrList[i++] = &C6510::SBC_indy;
	instrList[i++] = &C6510::ILL_TILT;
	instrList[i++] = &C6510::INCSBC_indy;
	instrList[i++] = &C6510::ILL_2NOP;
	instrList[i++] = &C6510::SBC_zpx;
	instrList[i++] = &C6510::INC_zpx;
	instrList[i++] = &C6510::INCSBC_zpx;
	instrList[i++] = &C6510::SED_;
	instrList[i++] = &C6510::SBC_absy;
	instrList[i++] = &C6510::ILL_1NOP;
	instrList[i++] = &C6510::INCSBC_absy;
	instrList[i++] = &C6510::ILL_3NOP;
	instrList[i++] = &C6510::SBC_absx;
	instrList[i++] = &C6510::INC_absx;
	instrList[i++] = &C6510::INCSBC_absx;
	
	if(i!=256)
		{
//		printf("C6510 init error\n"); /// @todo remove
#if defined(__SYMBIAN32__)
		User::Leave(-1);
#else
		exit(-1);
#endif
		}
		
	  
	}


C6510::~C6510()
/**
 * D'tor
 */
	{
	DTOR(C6510);
	}

// --------------------------------------------------------------------------


bool C6510::c64memAlloc()
{
	/// @todo remove c64memAlloc func

	c64mem1 = c64ramBuf;
	c64mem2 = c64romBuf;
	return true;
}


void C6510::initInterpreter(int inMemoryMode)
{
	memoryMode = inMemoryMode;
	if (memoryMode == MPU_TRANSPARENT_ROM)
	{
		readData = &C6510::readData_transp;
		writeData = &C6510::writeData_bs;
		instrList[0x20] = &C6510::JSR_transp;
		instrList[0x4C] = &C6510::JMP_transp;
		instrList[0x6C] = &C6510::JMP_vec_transp;
		// Make the memory buffers accessible to the whole emulator engine.
		// Use two distinct 64KB memory areas.
		c64mem1 = c64ramBuf;
		c64mem2 = c64romBuf;
	}
	else if (memoryMode == MPU_PLAYSID_ENVIRONMENT)
	{
		readData = &C6510::readData_plain;
		writeData = &C6510::writeData_plain;
		instrList[0x20] = &C6510::JSR_plain;
		instrList[0x4C] = &C6510::JMP_plain;
		instrList[0x6C] = &C6510::JMP_vec_plain;
		// Make the memory buffers accessible to the whole emulator engine.
		// Use a single 64KB memory area.
		c64mem2 = (c64mem1 = c64ramBuf);
	}
	else  // if (memoryMode == MPU_BANK_SWITCHING)
	{
		readData = &C6510::readData_bs;
		writeData = &C6510::writeData_bs;
		instrList[0x20] = &C6510::JSR_;
		instrList[0x4C] = &C6510::JMP_;
		instrList[0x6C] = &C6510::JMP_vec;
		// Make the memory buffers accessible to the whole emulator engine.
		// Use two distinct 64KB memory areas.
		c64mem1 = c64ramBuf;
		c64mem2 = c64romBuf;
	}
	bankSelReg = c64ramBuf+1;  // extra pointer
	// Set code execution segment to RAM.
	pPCbase = c64ramBuf;
	pPCend = c64ramBuf+65536;
}


void C6510::c64memReset(int clockSpeed, ubyte randomSeed)
	{
	iFakeRndSeed = (ubyte)(iFakeRndSeed*13+randomSeed);
	fakeReadTimer = iFakeRndSeed;
	
	if ((c64mem1 != 0) && (c64mem2 != 0))
	{
		c64mem1[0] = 0x2F;
		// defaults: Basic-ROM on, Kernal-ROM on, I/O on
		c64mem1[1] = 0x07;
		evalBankSelect();
		
		// CIA-Timer A $DC04/5 = $4025 PAL, $4295 NTSC
		if (clockSpeed == SIDTUNE_CLOCK_NTSC)
		{
			c64mem1[0x02a6] = 0;     // NTSC
			c64mem2[0xdc04] = 0x95;
			c64mem2[0xdc05] = 0x42;
		}
		else  // if (clockSpeed == SIDTUNE_CLOCK_PAL)
		{
			c64mem1[0x02a6] = 1;     // PAL
			c64mem2[0xdc04] = 0x25;
			c64mem2[0xdc05] = 0x40;
		}
		
		// fake VBI-interrupts that do $D019, BMI ...
		c64mem2[0xd019] = 0xff;
		
		// software vectors
		// IRQ to $EA31
		c64mem1[0x0314] = 0x31;
		c64mem1[0x0315] = 0xea;
		// BRK to $FE66
		c64mem1[0x0316] = 0x66;
		c64mem1[0x0317] = 0xfe;
		// NMI to $FE47
		c64mem1[0x0318] = 0x47;
		c64mem1[0x0319] = 0xfe;
		
		// hardware vectors 
		if (memoryMode == MPU_PLAYSID_ENVIRONMENT)
		{
			c64mem1[0xff48] = 0x6c;
			c64mem1[0xff49] = 0x14;
			c64mem1[0xff4a] = 0x03;
			c64mem1[0xfffa] = 0xf8;
			c64mem1[0xfffb] = 0xff;
			c64mem1[0xfffe] = 0x48;
			c64mem1[0xffff] = 0xff;
		}
		else
		{
			// NMI to $FE43
			c64mem1[0xfffa] = 0x43;
			c64mem1[0xfffb] = 0xfe;
			// RESET to $FCE2
			c64mem1[0xfffc] = 0xe2;
			c64mem1[0xfffd] = 0xfc;
			// IRQ to $FF48
			c64mem1[0xfffe] = 0x48;
			c64mem1[0xffff] = 0xff;
		}
		
		// clear SID
		for ( int i = 0; i < 0x1d; i++ )
		{
			c64mem2[0xd400 +i] = 0;
		}
		// default Mastervolume, no filter
		c64mem2[0xd418] = (sidLastValue = 0x0f);
	}
}


void C6510::c64memClear()
{
	// Clear entire RAM and ROM.
#if defined(__SYMBIAN32__)
// 
// use built in Mem functions for Epoc.
// They are much faster than looping 65536 times
// on byte-by-byte basis.
// 
	Mem::FillZ((TAny*)c64mem1, 0x10000);
	if (memoryMode != MPU_PLAYSID_ENVIRONMENT)
		{
		Mem::FillZ((TAny*)c64mem2, 0x10000);
		}
	sidLastValue = 0;

	if (memoryMode == MPU_PLAYSID_ENVIRONMENT)
	{
		Mem::Fill((TAny*)&c64mem1[0xE000], (0x10000-0xE000), 0x40);
	}
	else
	{
		// Fill Basic-ROM address space with RTS instructions.
		Mem::Fill((TAny*)&c64mem2[0xA000], (0xC000-0xA000), 0x60);

		// Fill Kernal-ROM address space with RTI instructions.
		Mem::Fill((TAny*)&c64mem2[0xE000], (0x10000-0xE000), 0x40);

	}
#else
	for ( udword i = 0; i < 0x10000; i++ )
	{
		c64mem1[i] = 0; 
		if (memoryMode != MPU_PLAYSID_ENVIRONMENT)
		{
			c64mem2[i] = 0; 
		}
		sidLastValue = 0;
	}
	if (memoryMode == MPU_PLAYSID_ENVIRONMENT)
	{
		// Fill Kernal-ROM address space with RTI instructions.
		for ( udword j = 0xE000; j < 0x10000; j++ )
		{
			c64mem1[j] = 0x40;
		}
	}
	else
	{
		// Fill Basic-ROM address space with RTS instructions.
		for ( udword j1 = 0xA000; j1 < 0xC000; j1++ )
		{
			c64mem2[j1] = 0x60;
		}
		// Fill Kernal-ROM address space with RTI instructions.
		for ( udword j2 = 0xE000; j2 < 0x10000; j2++ )
		{
			c64mem2[j2] = 0x40;
		}
	}
#endif
}


bool C6510::interpreter(uword p, ubyte ramrom, ubyte a, ubyte x, ubyte y)
{
	if (memoryMode == MPU_PLAYSID_ENVIRONMENT)
	{
		AC = a;
		XR = 0;
		YR = 0;
	}
	else
	{
		*bankSelReg = ramrom;
		evalBankSelect();
		AC = a;
		XR = x;
		YR = y;
	}

	// Set program-counter (pointer instead of raw PC).
	pPC = pPCbase+p;
	
	resetSP();
	clearSR();
	sidKeysOff[4] = (sidKeysOff[4+7] = (sidKeysOff[4+14] = false));
	sidKeysOn[4] = (sidKeysOn[4+7] = (sidKeysOn[4+14] = false));
	
	do
	{ 
	// AlfredH 23. Aug 2001:
	//
	// Nasty bug fixed here where pPC was not updated
	// until *after* the instruction method was called !
	//
		const ubyte instr = *(pPC++);
		(this->*instrList[instr])();
	}
	while (stackIsOkay&&(pPC<pPCend));

	return true;
}


//  Input: A 16-bit effective address
// Output: A default bank-select value for $01.
ubyte C6510::c64memRamRom( uword address )
{ 
	if (memoryMode == MPU_PLAYSID_ENVIRONMENT)
	{
		return 4;  // RAM only, but special I/O mode
	}
	else
	{
		if ( address < 0xa000 )
		{
			return 7;  // Basic-ROM, Kernal-ROM, I/O
		}
		else if ( address < 0xd000 )
		{
			return 6;  // Kernal-ROM, I/O
		}
		else if ( address >= 0xe000 )
		{
			return 5;  // I/O only
		}
		else
		{
			return 4;  // RAM only
		}
	}
}
