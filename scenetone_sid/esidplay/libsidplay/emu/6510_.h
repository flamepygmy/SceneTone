//
// 1997/05/30 13:36:14
//

#ifndef __6510_H
  #define __6510_H


#include "mytypes.h"
#include "myendian.h"
#include "epocglue.h"



// ------------------------------------------------------ (S)tatus (R)egister
// MOS-6510 SR: NV-BDIZC
//              76543210
//

struct statusRegister
	{
	unsigned Carry     : 1;
	unsigned Zero      : 1;
	unsigned Interrupt : 1;
	unsigned Decimal   : 1;
	unsigned Break     : 1;
	unsigned NotUsed   : 1;
	unsigned oVerflow  : 1;
	unsigned Negative  : 1;
	};


//
// Some handy defines to ease SR access.
//
#define CF SR.Carry
#define ZF SR.Zero
#define IF SR.Interrupt
#define DF SR.Decimal
#define BF SR.Break
#define NU SR.NotUsed
#define VF SR.oVerflow
#define NF SR.Negative


/**
 * implements the main CPU in the C64, MOS6510
 */

#if defined (__SYMBIAN32__)
class C6510 : public CBase // for Zero filling object
#else
class C6510
#endif
	{
  public:
	C6510();
	~C6510();
	//
	bool c64memAlloc();
	//
	// called by eeconfig.cpp
	//
	void initInterpreter(int inMemoryMode);
	void c64memReset(int clockSpeed, ubyte randomSeed);
	void c64memClear();
	//
	// called by player.cpp
	//
	bool interpreter(uword p, ubyte ramrom, ubyte a, ubyte x, ubyte y);
	ubyte c64memRamRom( uword address );


  private:
	//
	inline void affectNZ(ubyte reg)
		{
		ZF = (reg == 0);
		NF = ((reg & 0x80) != 0);
		}

	inline void clearSR()
		{
		// Explicit paranthesis looks great.
		CF = (ZF = (IF = (DF = (BF = (NU = (VF = (NF = 0)))))));
		}

	inline ubyte codeSR()
		{
		register ubyte tempSR = CF;
		tempSR |= (ZF<<1);
		tempSR |= (IF<<2);
		tempSR |= (DF<<3);
		tempSR |= (BF<<4);
		tempSR |= (NU<<5);
		tempSR |= (VF<<6);
		tempSR |= (NF<<7);
		return tempSR;
		}

	inline void decodeSR(ubyte stackByte)
		{
		CF = (stackByte & 1);               
		ZF = ((stackByte & 2) !=0 );
		IF = ((stackByte & 4) !=0 );
		DF = ((stackByte & 8) !=0 );
		BF = ((stackByte & 16) !=0 );
		NU = ((stackByte & 32) !=0 );
		VF = ((stackByte & 64) !=0 );
		NF = ((stackByte & 128) !=0 );
		}

// --------------------------------------------------------------------------
// Handling conditional branches.

	inline void branchIfClear(ubyte flag)
		{
		if (flag == 0)
			{
			PC = (uword)(pPC-pPCbase);   // calculate 16-bit PC
			PC += (sbyte)(*pPC);  // add offset, keep it 16-bit (uword)
			pPC = pPCbase+PC;   // calc new pointer-PC
			}
		pPC++;
		}

	inline void branchIfSet(ubyte flag)
		{
		if (flag != 0)
			{
			PC = (uword)(pPC-pPCbase);   // calculate 16-bit PC
			PC += (sbyte)(*pPC);  // add offset, keep it 16-bit (uword)
			pPC = pPCbase+PC;   // calc new pointer-PC
			}
		pPC++;
		}

// --------------------------------------------------------------------------
// Addressing modes:
// Calculating 8/16-bit effective addresses out of data operands.

public:
	  inline uword abso()
		{
		return readLEword(pPC);
		}
private:

	inline uword absx()
		{
		return (uword)(readLEword(pPC)+XR);
		}

	inline uword absy()
		{
		return (uword)(readLEword(pPC)+YR);
		}

	inline ubyte imm()
		{
		return *pPC;
		}

	inline uword indx()
		{
		return readEndian(c64mem1[(*pPC+1+XR)&0xFF],c64mem1[(*pPC+XR)&0xFF]);
		}

	inline uword indy()
		{
		return (uword)(YR+readEndian(c64mem1[(*pPC+1)&0xFF],c64mem1[*pPC]));
		}

	inline ubyte zp()
		{
		return *pPC;
		}

	inline ubyte zpx()
		{
		return (ubyte)(*pPC+XR);
		}

	inline ubyte zpy()
		{
		return (ubyte)(*pPC+YR);
		}



// --------------------------------------------------------------------------
// Relevant configurable memory banks:
//
//  $A000 to $BFFF = RAM, Basic-ROM
//  $C000 to $CFFF = RAM
//  $D000 to $DFFF = RAM, I/O, Char-ROM 
//  $E000 to $FFFF = RAM, Kernal-ROM
//
// Bank-Select Register $01:
// 
//   Bits
//   210    $A000-$BFFF   $D000-$DFFF   $E000-$FFFF
//  ------------------------------------------------
//   000       RAM           RAM            RAM
//   001       RAM        Char-ROM          RAM
//   010       RAM        Char-ROM      Kernal-ROM
//   011    Basic-ROM     Char-ROM      Kernal-ROM
//   100       RAM           RAM            RAM
//   101       RAM           I/O            RAM
//   110       RAM           I/O        Kernal-ROM
//   111    Basic-ROM        I/O        Kernal-ROM
//
// "Transparent ROM" mode:
//
// Basic-ROM and Kernal-ROM are considered transparent to read/write access.
// Basic-ROM is also considered transparent to branches (JMP, BCC, ...).
// I/O and Kernal-ROM are togglable via bank-select register $01.



	inline void evalBankSelect()
		{
		// Determine new memory configuration.
		isBasic = ((*bankSelReg & 3) == 3);
		isIO = ((*bankSelReg & 7) > 4);
		isKernal = ((*bankSelReg & 2) != 0);
		}


// Upon JMP/JSR prevent code execution in Basic-ROM/Kernal-ROM.
	inline void evalBankJump()
		{
		if (PC < 0xA000)
			{
			;
			}
		else
			{
			// Get high-nibble of address.
			switch (PC >> 12)
				{
			  case 0xa:
			  case 0xb:
				  {
				  if (isBasic)
					  {
					  RTS_();
					  }
				  break;
				  }
			  case 0xc:
				  {
				  break;
				  }
			  case 0xd:
				  {
				  if (isIO)
					  {
					  RTS_();
					  }
				  break;
				  }
			  case 0xe:
			  case 0xf:
			  default:  // <-- just to please the compiler
				  {
				  if (isKernal)
					  {
					  RTS_();
					  }
				  break;
				  }
				}
			}
		}


  private:

// Functions to retrieve data.
	ubyte readData_bs(uword addr)
		{
		if (addr < 0xA000)
			{
			return c64mem1[addr];
			}
		else
			{
			// Get high-nibble of address.
			switch (addr >> 12)
				{
			  case 0xa:
			  case 0xb:
				  {
				  if (isBasic)
					  return c64mem2[addr];
				  else
					  return c64mem1[addr];
				  }
			  case 0xc:
				  {
				  return c64mem1[addr];
				  }
			  case 0xd:
				  {
				  if (isIO)
					  {
					  uword tempAddr = (uword)(addr & 0xfc1f);
					  // Not SID ?
					  if (( tempAddr & 0xff00 ) != 0xd400 )
						  {
						  switch (addr)
							  {
							case 0xd011:
							case 0xd012:
							case 0xdc04:
							case 0xdc05:
								{
								return (ubyte)(fakeReadTimer = (ubyte)(c64mem2[addr]+fakeReadTimer*13+1));
								}
							default:
								{
								return c64mem2[addr];
								}
							  }
						  }
					  else
						  {
						  // $D41D/1E/1F, $D43D/, ... SID not mirrored
						  if (( tempAddr & 0x00ff ) >= 0x001d )
							  return(c64mem2[addr]); 
						  // (Mirrored) SID.
						  else
							  {
							  switch (tempAddr)
								  {
								case 0xd41b:
									{
									return optr3readWave;
									}
								case 0xd41c:
									{
									return optr3readEnve;
									}
								default:
									{
									return sidLastValue;
									}
								  }
							  }
						  }
					  }
				  else
					  return c64mem1[addr];
				  }
			  case 0xe:
			  case 0xf:
			  default:  // <-- just to please the compiler
				  {
				  if (isKernal)
					  return c64mem2[addr];
				  else
					  return c64mem1[addr];
				  }
				}
			}
		}

	ubyte readData_transp(uword addr)
		{
		if (addr < 0xD000)
			{
			return c64mem1[addr];
			}
		else
			{
			// Get high-nibble of address.
			switch (addr >> 12)
				{
			  case 0xd:
				  {
				  if (isIO)
					  {
					  uword tempAddr = (uword)(addr & 0xfc1f);
					  // Not SID ?
					  if (( tempAddr & 0xff00 ) != 0xd400 )
						  {
						  switch (addr)
							  {
							case 0xd011:
							case 0xd012:
							case 0xdc04:
							case 0xdc05:
								{
								return (ubyte)(fakeReadTimer = (ubyte)(c64mem2[addr]+fakeReadTimer*13+1));
								}
							default:
								{
								return c64mem2[addr];
								}
							  }
						  }
					  else
						  {
						  // $D41D/1E/1F, $D43D/, ... SID not mirrored
						  if (( tempAddr & 0x00ff ) >= 0x001d )
							  return(c64mem2[addr]); 
						  // (Mirrored) SID.
						  else
							  {
							  switch (tempAddr)
								  {
								case 0xd41b:
									{
									return optr3readWave;
									}
								case 0xd41c:
									{
									return optr3readEnve;
									}
								default:
									{
									return sidLastValue;
									}
								  }
							  }
						  }
					  }
				  else
					  return c64mem1[addr];
				  }
			  case 0xe:
			  case 0xf:
			  default:  // <-- just to please the compiler
				  {
				  return c64mem1[addr];
				  }
				}
			}
		}

	ubyte readData_plain(uword addr)
		{
		return c64mem1[addr];
		}

	inline ubyte readData_zp(uword addr)
		{
		return c64mem1[addr];
		}


// Functions to store data.

	void writeData_bs(uword addr, ubyte data)
		{
		if ((addr < 0xd000) || (addr >= 0xe000))
			{
			c64mem1[addr] = data;
			if (addr == 0x01)  // write to Bank-Select Register ?
				{
				evalBankSelect();
				}
			}
		else
			{
			if (isIO)
				{
				// Check whether real SID or mirrored SID.
				uword tempAddr = (uword)(addr & 0xfc1f);
				// Not SID ?
				if (( tempAddr & 0xff00 ) != 0xd400 )
					{
					c64mem2[addr] = data;
					}
				// $D41D/1E/1F, $D43D/3E/3F, ...
				// Map to real address to support PlaySID
				// Extended SID Chip Registers.
				else if (( tempAddr & 0x00ff ) >= 0x001d )
					{
					// Mirrored SID.
					c64mem2[addr] = (sidLastValue = data);
					}
				else
					{
					// SID.
					c64mem2[tempAddr] = (sidLastValue = data);
					// Handle key_ons.
					sidKeysOn[tempAddr&0x001f] = sidKeysOn[tempAddr&0x001f] || ((data&1)!=0); 
					// Handle key_offs.
					sidKeysOff[tempAddr&0x001f] = sidKeysOff[tempAddr&0x001f] || ((data&1)==0); 
					}
				}
			else
				{
				c64mem1[addr] = data;
				}
			}
		}

	void writeData_plain(uword addr, ubyte data)
		{
		// Check whether real SID or mirrored SID.
		uword tempAddr = (uword)(addr & 0xfc1f);
		// Not SID ?
		if (( tempAddr & 0xff00 ) != 0xd400 )
			{
			c64mem1[addr] = data; 
			}
		// $D41D/1E/1F, $D43D/3E/3F, ...
		// Map to real address to support PlaySID
		// Extended SID Chip Registers.
		else if (( tempAddr & 0x00ff ) >= 0x001d )
			{
			// Mirrored SID.
			c64mem1[addr] = (sidLastValue = data);
			}
		else
			{
			// SID.
			c64mem2[tempAddr] = (sidLastValue = data);
			// Handle key_ons.
			sidKeysOn[tempAddr&0x001f] = sidKeysOn[tempAddr&0x001f] || ((data&1)!=0); 
			// Handle key_offs.
			sidKeysOff[tempAddr&0x001f] = sidKeysOff[tempAddr&0x001f] || ((data&1)==0); 
			}
		}

	inline void writeData_zp(uword addr, ubyte data)
		{
		c64mem1[addr] = data;
		if (addr == 0x01)  // write to Bank-Select Register ?
			{
			evalBankSelect();
			}
		}


// --------------------------------------------------------------------------
// Legal instructions in alphabetical order.
//
// LIFO-Stack:
//
// |xxxxxx|
// |xxxxxx|
// |______|<- SP <= (hi)-return-address
// |______|      <= (lo)
// |______|
//


	inline void resetSP()
		{
		SP = 0x1ff;          // SP to top of stack
		stackIsOkay = true;
		}

	inline void checkSP()
		{
		stackIsOkay = ((SP>0xff)&&(SP<=0x1ff));  // check boundaries
		}

// ---

	inline void ADC_m(ubyte x)
		{
		if ( DF == 1 )
			{
			uword AC2 = (uword)(AC +x +CF);
			ZF = ( AC2 == 0 );
			if ((( AC & 15 ) + ( x & 15 ) + CF ) > 9 )
				{
				AC2 += 6;
				}
			VF = ((( AC ^ x ^ AC2 ) & 0x80 ) != 0 ) ^ CF;
			NF = (( AC2 & 128 ) != 0 );
			if ( AC2 > 0x99 )
				{
				AC2 += 96;
				}
			CF = ( AC2 > 0x99 );
			AC = (ubyte)( AC2 & 255 );
			}
		else
			{
			uword AC2 = (uword)(AC +x +CF);
			CF = ( AC2 > 255 );
			VF = ((( AC ^ x ^ AC2 ) & 0x80 ) != 0 ) ^ CF;
			affectNZ( AC = (ubyte)( AC2 & 255 ));
			}
		}
	void ADC_imm()  { ADC_m(imm()); pPC++; }
	void ADC_abso()  { ADC_m( (this->*readData)(abso()) ); pPC += 2; }
	void ADC_absx()  { ADC_m( (this->*readData)(absx()) ); pPC += 2; }
	void ADC_absy()  { ADC_m( (this->*readData)(absy()) ); pPC += 2; }
	void ADC_indx()  { ADC_m( (this->*readData)(indx()) ); pPC++; }
	void ADC_indy()  { ADC_m( (this->*readData)(indy()) ); pPC++; }
	void ADC_zp()  { ADC_m( readData_zp(zp()) ); pPC++; }
	void ADC_zpx()  { ADC_m( readData_zp(zpx()) ); pPC++; }


	inline void AND_m(ubyte x)
		{
		affectNZ( AC &= x );
		}
	void AND_imm()  { AND_m(imm()); pPC++; }
	void AND_abso()  { AND_m( (this->*readData)(abso()) ); pPC += 2; }
	void AND_absx()  { AND_m( (this->*readData)(absx()) ); pPC += 2; }
	void AND_absy()  { AND_m( (this->*readData)(absy()) ); pPC += 2; }
	void AND_indx()  { AND_m( (this->*readData)(indx()) ); pPC++; }
	void AND_indy()  { AND_m( (this->*readData)(indy()) ); pPC++; }
	void AND_zp()  { AND_m( readData_zp(zp()) ); pPC++; }
	void AND_zpx()  { AND_m( readData_zp(zpx()) ); pPC++; }


	inline ubyte ASL_m(ubyte x)
		{ 
		CF = (( x & 128 ) != 0 );
		affectNZ( x <<= 1);
		return x;
		}
	void ASL_AC()
		{ 
		AC = ASL_m(AC);                         
		}
	void ASL_abso()
		{ 
		uword tempAddr = abso();
		pPC += 2;
		(this->*writeData)( tempAddr, ASL_m( (this->*readData)(tempAddr)) );
		}
	void ASL_absx()
		{ 
		uword tempAddr = absx();
		pPC += 2;
		(this->*writeData)( tempAddr, ASL_m( (this->*readData)(tempAddr)) );
		}
	void ASL_zp()
		{ 
		uword tempAddr = zp();
		pPC++;
		writeData_zp( tempAddr, ASL_m( readData_zp(tempAddr)) );
		}
	void ASL_zpx()
		{ 
		uword tempAddr = zpx();
		pPC++;
		writeData_zp( tempAddr, ASL_m( readData_zp(tempAddr)) );
		}


	void BCC_()  { branchIfClear(CF); }

	void BCS_()  { branchIfSet(CF); }

	void BEQ_()  { branchIfSet(ZF); }


	inline void BIT_m(ubyte x)
		{ 
		ZF = (( AC & x ) == 0 );                  
		VF = (( x & 64 ) != 0 );
		NF = (( x & 128 ) != 0 );
		}
	void BIT_abso()  { BIT_m( (this->*readData)(abso()) ); pPC += 2; }
	void BIT_zp()  {	BIT_m( readData_zp(zp()) );	pPC++; }


	void BMI_()  { branchIfSet(NF); }

	void BNE_()  { branchIfClear(ZF); }

	void BPL_()  { branchIfClear(NF); }


	void BRK_()
		{ 
		BF = (IF = 1);
#if !defined(NO_RTS_UPON_BRK)
		RTS_();
#endif
		}


	void BVC_()  { branchIfClear(VF); }

	void BVS_()  { branchIfSet(VF); }


	void CLC_()  { CF = 0; }

	void CLD_()  { DF = 0; }

	void CLI_()  { IF = 0; }

	void CLV_()  { VF = 0; }


	inline void CMP_m(ubyte x)
		{ 
		ZF = ( AC == x );                     
		CF = ( AC >= x );                         
		NF = ( (sbyte)( AC - x ) < 0 );
		}
	void CMP_abso()  { CMP_m( (this->*readData)(abso()) ); pPC += 2; }
	void CMP_absx()  { CMP_m( (this->*readData)(absx()) ); pPC += 2; }
	void CMP_absy()  { CMP_m( (this->*readData)(absy()) ); pPC += 2; }
	void CMP_imm()  { CMP_m(imm()); pPC++; }
	void CMP_indx()  { CMP_m( (this->*readData)(indx()) ); pPC++; }
	void CMP_indy()  { CMP_m( (this->*readData)(indy()) ); pPC++; }
	void CMP_zp()  { CMP_m( readData_zp(zp()) ); pPC++; }
	void CMP_zpx()  { CMP_m( readData_zp(zpx()) ); pPC++; }


	inline void CPX_m(ubyte x)
		{ 
		ZF = ( XR == x );                         
		CF = ( XR >= x );                         
		NF = ( (sbyte)( XR - x ) < 0 );      
		}
	void CPX_abso()  { CPX_m( (this->*readData)(abso()) ); pPC += 2; }
	void CPX_imm()  { CPX_m(imm()); pPC++; }
	void CPX_zp()  { CPX_m( readData_zp(zp()) ); pPC++; }


	inline void CPY_m(ubyte x)
		{ 
		ZF = ( YR == x );                         
		CF = ( YR >= x );                         
		NF = ( (sbyte)( YR - x ) < 0 );      
		}
	void CPY_abso()  { CPY_m( (this->*readData)(abso()) ); pPC += 2; }
	void CPY_imm()  { CPY_m(imm()); pPC++; }
	void CPY_zp()  { CPY_m( readData_zp(zp()) ); pPC++; }


	inline void DEC_m(uword addr)
		{ 
		ubyte x = (this->*readData)(addr);                   
		affectNZ(--x);
		(this->*writeData)(addr, x);
		}
	inline void DEC_m_zp(uword addr)
		{ 
		ubyte x = readData_zp(addr);
		affectNZ(--x);
		writeData_zp(addr, x);
		}
	void DEC_abso()  { DEC_m( abso() ); pPC += 2; }
	void DEC_absx()  { DEC_m( absx() ); pPC += 2; }
	void DEC_zp()  { DEC_m_zp( zp() ); pPC++; }
	void DEC_zpx()  { DEC_m_zp( zpx() ); pPC++; }


	void DEX_()  { affectNZ(--XR); }

	void DEY_()  { affectNZ(--YR); }


	inline void EOR_m(ubyte x)
		{ 
		AC ^= x;                          
		affectNZ(AC);                         
		}
	void EOR_abso()  { EOR_m( (this->*readData)(abso()) ); pPC += 2; }
	void EOR_absx()  { EOR_m( (this->*readData)(absx()) ); pPC += 2; }
	void EOR_absy()  { EOR_m( (this->*readData)(absy()) ); pPC += 2; }
	void EOR_imm()  { EOR_m(imm()); pPC++; }
	void EOR_indx()  { EOR_m( (this->*readData)(indx()) ); pPC++; }
	void EOR_indy()  { EOR_m( (this->*readData)(indy()) ); pPC++; }
	void EOR_zp()  { EOR_m( readData_zp(zp()) ); pPC++; }
	void EOR_zpx()  { EOR_m( readData_zp(zpx()) ); pPC++; }


	inline void INC_m(uword addr)
		{
		ubyte x = (this->*readData)(addr);                   
		affectNZ(++x);
		(this->*writeData)(addr, x);
		}
	inline void INC_m_zp(uword addr)
		{
		ubyte x = readData_zp(addr);                   
		affectNZ(++x);
		writeData_zp(addr, x);
		}
	void INC_abso()  { INC_m( abso() ); pPC += 2; }
	void INC_absx()  { INC_m( absx() ); pPC += 2; }
	void INC_zp()  { INC_m_zp( zp() ); pPC++; }
	void INC_zpx()  { INC_m_zp( zpx() ); pPC++; }


	void INX_()  { affectNZ(++XR); }

	void INY_()  { affectNZ(++YR); }



	void JMP_()
		{ 
		PC = abso();
		pPC = pPCbase+PC;
		evalBankJump();
		}

	void JMP_transp()
		{ 
		PC = abso();
		if ( (PC>=0xd000) && isKernal )
			{
			RTS_();  // will set pPC
			}
		else
			{
			pPC = pPCbase+PC;
			}
		}

	void JMP_plain()
		{ 
		PC = abso();
		pPC = pPCbase+PC;
		}


	void JMP_vec()
		{ 
		uword tempAddrLo = abso();
		uword tempAddrHi = (uword)((tempAddrLo&0xFF00) | ((tempAddrLo+1)&0x00FF));
		PC = readEndian((this->*readData)(tempAddrHi),(this->*readData)(tempAddrLo));
		pPC = pPCbase+PC;
		evalBankJump();
		}

	void JMP_vec_transp()
		{ 
		uword tempAddrLo = abso();
		uword tempAddrHi = (uword)((tempAddrLo&0xFF00) | ((tempAddrLo+1)&0x00FF));
		PC = readEndian((this->*readData)(tempAddrHi),(this->*readData)(tempAddrLo));
		if ( (PC>=0xd000) && isKernal )
			{
			RTS_();  // will set pPC
			}
		else
			{
			pPC = pPCbase+PC;
			}
		}

	void JMP_vec_plain()
		{ 
		uword tempAddrLo = abso();
		uword tempAddrHi = (uword)((tempAddrLo&0xFF00) | ((tempAddrLo+1)&0x00FF));
		PC = readEndian((this->*readData)(tempAddrHi),(this->*readData)(tempAddrLo));
		pPC = pPCbase+PC;
		}


	inline void JSR_main()
		{
		uword tempPC = abso();
		pPC += 2;
		PC = (uword)(pPC-pPCbase);
		PC--;
		SP--;
		writeLEword(c64mem1+SP,PC);
		SP--;
		checkSP();
		PC = tempPC;
		}

	void JSR_()
		{ 
		JSR_main();
		pPC = pPCbase+PC;
		evalBankJump();
		}

	void JSR_transp()
		{ 
		JSR_main();
		if ( (PC>=0xd000) && isKernal )
			{
			RTS_();  // will set pPC
			}
		else
			{
			pPC = pPCbase+PC;
			}
		}

	void JSR_plain()
		{ 
		JSR_main();
		pPC = pPCbase+PC;
		}


	void LDA_abso()  { affectNZ( AC = (this->*readData)(abso()) ); pPC += 2; }
	void LDA_absx()  { affectNZ( AC = (this->*readData)( absx() )); pPC += 2; }
	void LDA_absy()  { affectNZ( AC = (this->*readData)( absy() ) ); pPC += 2; }
	void LDA_imm()  { affectNZ( AC = imm() ); pPC++; }
	void LDA_indx()  { affectNZ( AC = (this->*readData)( indx() ) ); pPC++; }
	void LDA_indy()  { affectNZ( AC = (this->*readData)( indy() ) ); pPC++; }
	void LDA_zp()  { affectNZ( AC = readData_zp( zp() ) ); pPC++; }
	void LDA_zpx()  { affectNZ( AC = readData_zp( zpx() ) ); pPC++; }


	void LDX_abso()  { affectNZ(XR=(this->*readData)(abso())); pPC += 2; }
	void LDX_absy()  { affectNZ(XR=(this->*readData)(absy())); pPC += 2; }
	void LDX_imm()  { affectNZ(XR=imm()); pPC++; }
	void LDX_zp()  { affectNZ(XR=readData_zp(zp())); pPC++; }
	void LDX_zpy()  { affectNZ(XR=readData_zp(zpy())); pPC++; }


	void LDY_abso()  { affectNZ(YR=(this->*readData)(abso())); pPC += 2; }
	void LDY_absx()  { affectNZ(YR=(this->*readData)(absx())); pPC += 2; }
	void LDY_imm()  { affectNZ(YR=imm()); pPC++; }
	void LDY_zp()  { affectNZ(YR=readData_zp(zp())); pPC++; }
	void LDY_zpx()  { affectNZ(YR=readData_zp(zpx())); pPC++; }


	inline ubyte LSR_m(ubyte x)
		{ 
		CF = x & 1;
		x >>= 1;
		NF = 0;
		ZF = (x == 0);
		return x;
		}
	void LSR_AC()
		{ 
		AC = LSR_m(AC);
		}
	void LSR_abso()
		{ 
		uword tempAddr = abso();
		pPC += 2; 
		(this->*writeData)( tempAddr, (LSR_m( (this->*readData)(tempAddr))) );
		}
	void LSR_absx()
		{ 
		uword tempAddr = absx();
		pPC += 2; 
		(this->*writeData)( tempAddr, (LSR_m( (this->*readData)(tempAddr))) );
		}
	void LSR_zp()
		{ 
		uword tempAddr = zp();
		pPC++; 
		writeData_zp( tempAddr, (LSR_m( readData_zp(tempAddr))) );
		}
	void LSR_zpx()
		{ 
		uword tempAddr = zpx();
		pPC++; 
		writeData_zp( tempAddr, (LSR_m( readData_zp(tempAddr))) );
		}


	inline void ORA_m(ubyte x)
		{
		affectNZ( AC |= x );
		}
	void ORA_abso()  { ORA_m( (this->*readData)(abso()) ); pPC += 2; }
	void ORA_absx()  { ORA_m( (this->*readData)(absx()) ); pPC += 2; }
	void ORA_absy()  { ORA_m( (this->*readData)(absy()) ); pPC += 2; }
	void ORA_imm()  { ORA_m(imm()); pPC++; }
	void ORA_indx()  { ORA_m( (this->*readData)(indx()) ); pPC++; }
	void ORA_indy()  { ORA_m( (this->*readData)(indy()) ); pPC++; }
	void ORA_zp()  { ORA_m( readData_zp(zp()) ); pPC++; }
	void ORA_zpx()  { ORA_m( readData_zp(zpx()) ); pPC++; }


	void NOP_()  { }

	void PHA_()  { c64mem1[SP--] = AC; }


	void PHP_()
		{ 
		c64mem1[SP--] = codeSR();
		}


	void PLA_()
		{ 
		affectNZ(AC=c64mem1[++SP]);
		}


	void PLP_()
		{ 
		decodeSR(c64mem1[++SP]);
		}

	inline ubyte ROL_m(ubyte x)
		{ 
		ubyte y = (ubyte)(( x << 1 ) + CF);
		CF = (( x & 0x80 ) != 0 );
		affectNZ(y);
		return y;
		}
	void ROL_AC()  { AC=ROL_m(AC); }
	void ROL_abso()
		{ 
		uword tempAddr = abso();
		pPC += 2; 
		(this->*writeData)( tempAddr, ROL_m( (this->*readData)(tempAddr)) );
		}
	void ROL_absx()
		{ 
		uword tempAddr = absx();
		pPC += 2; 
		(this->*writeData)( tempAddr, ROL_m( (this->*readData)(tempAddr)) );
		}
	void ROL_zp()
		{ 
		uword tempAddr = zp();
		pPC++; 
		writeData_zp( tempAddr, ROL_m( readData_zp(tempAddr)) );
		}
	void ROL_zpx()
		{ 
		uword tempAddr = zpx();
		pPC++; 
		writeData_zp( tempAddr, ROL_m( readData_zp(tempAddr)) );
		}

	inline ubyte ROR_m(ubyte x)
		{ 
		ubyte y = (ubyte)(( x >> 1 ) | ( CF << 7 ));
		CF = ( x & 1 );
		affectNZ(y);
		return y;
		}
	void ROR_AC()
		{ 
		AC = ROR_m(AC);
		}
	void ROR_abso()
		{ 
		uword tempAddr = abso();
		pPC += 2; 
		(this->*writeData)( tempAddr, ROR_m( (this->*readData)(tempAddr)) );
		}
	void ROR_absx()
		{ 
		uword tempAddr = absx();
		pPC += 2; 
		(this->*writeData)( tempAddr, ROR_m( (this->*readData)(tempAddr)) );
		}
	void ROR_zp()
		{ 
		uword tempAddr = zp();
		pPC++; 
		writeData_zp( tempAddr, ROR_m( readData_zp(tempAddr)) );
		}
	void ROR_zpx()
		{ 
		uword tempAddr = zpx();
		pPC++; 
		writeData_zp( tempAddr, ROR_m( readData_zp(tempAddr)) );
		}


	void RTI_()	
		{
		// equal to RTS_();
		SP++;
		PC = (uword)(readEndian( c64mem1[SP +1], c64mem1[SP] ) +1);
		pPC = pPCbase+PC;
		SP++;
		checkSP();
		}


	inline void RTS_()
		{ 
		SP++;
		PC = (uword)(readEndian( c64mem1[SP +1], c64mem1[SP] ) +1);
		pPC = pPCbase+PC;
		SP++;
		checkSP();
		}


	inline void SBC_m(ubyte s)
		{ 
		s = (ubyte)((~s) & 255);
		if ( DF == 1 )
			{
			uword AC2 = (uword)(AC +s +CF);
			ZF = ( AC2 == 0 );
			if ((( AC & 15 ) + ( s & 15 ) + CF ) > 9 )
				{
				AC2 += 6;
				}
			VF = ((( AC ^ s ^ AC2 ) & 0x80 ) != 0 ) ^ CF;
			NF = (( AC2 & 128 ) != 0 );
			if ( AC2 > 0x99 )
				{
				AC2 += 96;
				}
			CF = ( AC2 > 0x99 );
			AC = (ubyte)( AC2 & 255 );
			}
		else
			{
			uword AC2 = (uword)(AC + s + CF);
			CF = ( AC2 > 255 );
			VF = ((( AC ^ s ^ AC2 ) & 0x80 ) != 0 ) ^ CF;
			affectNZ( AC = (ubyte)( AC2 & 255 ));
			}
		}
	void SBC_abso()  { SBC_m( (this->*readData)(abso()) ); pPC += 2; }
	void SBC_absx()  { SBC_m( (this->*readData)(absx()) ); pPC += 2; }
	void SBC_absy()  { SBC_m( (this->*readData)(absy()) ); pPC += 2; }
	void SBC_imm()  { SBC_m(imm()); pPC++; }
	void SBC_indx()  { SBC_m( (this->*readData)( indx()) ); pPC++; }
	void SBC_indy()  { SBC_m( (this->*readData)(indy()) ); pPC++; }
	void SBC_zp()  { SBC_m( readData_zp(zp()) ); pPC++; }
	void SBC_zpx()  { SBC_m( readData_zp(zpx()) ); pPC++; }


	void SEC_()  { CF=1; }

	void SED_()  { DF=1; }

	void SEI_()  { IF=1; }


	void STA_abso()  { (this->*writeData)( abso(), AC ); pPC += 2; }
	void STA_absx()  { (this->*writeData)( absx(), AC ); pPC += 2; }
	void STA_absy()  { (this->*writeData)( absy(), AC ); pPC += 2; }
	void STA_indx()  { (this->*writeData)( indx(), AC ); pPC++; }
	void STA_indy()  { (this->*writeData)( indy(), AC ); pPC++; }
	void STA_zp()  { writeData_zp( zp(), AC ); pPC++; }
	void STA_zpx() { writeData_zp( zpx(), AC ); pPC++; }


	void STX_abso()  { (this->*writeData)( abso(), XR ); pPC += 2; }
	void STX_zp()  { writeData_zp( zp(), XR ); pPC++; }
	void STX_zpy()  { writeData_zp( zpy(), XR ); pPC++; }


	void STY_abso()  { (this->*writeData)( abso(), YR ); pPC += 2; }
	void STY_zp()  { writeData_zp( zp(), YR ); pPC++; }
	void STY_zpx()  { writeData_zp( zpx(), YR ); pPC++; }


	void TAX_()  { affectNZ(XR=AC); }

	void TAY_()  { affectNZ(YR=AC); }


	void TSX_()
		{ 
		XR = (ubyte)(SP & 255);
		affectNZ(XR);
		}

	void TXA_()  { affectNZ(AC=XR); }

	void TXS_()  { SP = XR | 0x100; checkSP(); }

	void TYA_()  { affectNZ(AC=YR); }


// --------------------------------------------------------------------------
// Illegal codes/instructions part (1).

	void ILL_TILT()  { }

	void ILL_1NOP()  { }

	void ILL_2NOP()  { pPC++; }

	void ILL_3NOP()  { pPC += 2; }


// --------------------------------------------------------------------------
// Illegal codes/instructions part (2).

	inline void ASLORA_m(uword addr)
		{
		ubyte x = ASL_m((this->*readData)(addr));
		(this->*writeData)(addr,x);
		ORA_m(x);
		}
	inline void ASLORA_m_zp(uword addr)
		{
		ubyte x = ASL_m(readData_zp(addr));
		writeData_zp(addr,x);
		ORA_m(x);
		}
	void ASLORA_abso()
		{
		ASLORA_m(abso());
		pPC += 2; 
		}
	void ASLORA_absx()
		{
		ASLORA_m(absx());
		pPC += 2; 
		}
	void ASLORA_absy()
		{
		ASLORA_m(absy());
		pPC += 2; 
		}
	void ASLORA_indx()
		{
		ASLORA_m(indx());
		pPC++; 
		}
	void ASLORA_indy()
		{
		ASLORA_m(indy());
		pPC++; 
		}
	void ASLORA_zp()
		{
		ASLORA_m_zp(zp());
		pPC++; 
		} 
	void ASLORA_zpx()
		{
		ASLORA_m_zp(zpx());
		pPC++; 
		}


	inline void ROLAND_m(uword addr)
		{
		ubyte x = ROL_m((this->*readData)(addr));
		(this->*writeData)(addr, x);
		AND_m(x);
		}
	inline void ROLAND_m_zp(uword addr)
		{
		ubyte x = ROL_m(readData_zp(addr));
		writeData_zp(addr, x);
		AND_m(x);
		}
	void ROLAND_abso()
		{
		ROLAND_m(abso());
		pPC += 2; 
		}
	void ROLAND_absx()
		{
		ROLAND_m(absx());
		pPC += 2; 
		}
	void ROLAND_absy()
		{
		ROLAND_m(absy());
		pPC += 2; 
		}  
	void ROLAND_indx()
		{
		ROLAND_m(indx());
		pPC++; 
		}  
	void ROLAND_indy()
		{
		ROLAND_m(indy());
		pPC++; 
		} 
	void ROLAND_zp()
		{
		ROLAND_m_zp(zp());
		pPC++; 
		} 
	void ROLAND_zpx()
		{
		ROLAND_m_zp(zpx());
		pPC++; 
		} 


	inline void LSREOR_m(uword addr)
		{
		ubyte x = LSR_m((this->*readData)(addr));
		(this->*writeData)(addr, x);
		EOR_m(x);
		}
	inline void LSREOR_m_zp(uword addr)
		{
		ubyte x = LSR_m(readData_zp(addr));
		writeData_zp(addr,x);
		EOR_m(x);
		}
	void LSREOR_abso()
		{
		LSREOR_m(abso());
		pPC += 2; 
		}
	void LSREOR_absx()
		{
		LSREOR_m(absx());
		pPC += 2; 
		}  
	void LSREOR_absy()
		{
		LSREOR_m(absy());
		pPC += 2; 
		}  
	void LSREOR_indx()
		{
		LSREOR_m(indx());
		pPC++; 
		} 
	void LSREOR_indy()
		{
		LSREOR_m(indy());
		pPC++; 
		}  
	void LSREOR_zp()
		{
		LSREOR_m_zp(zp());
		pPC++; 
		}  
	void LSREOR_zpx()
		{
		LSREOR_m_zp(zpx());
		pPC++; 
		}  


	inline void RORADC_m(uword addr)
		{
		ubyte x = ROR_m((this->*readData)(addr));
		(this->*writeData)(addr,x);
		ADC_m(x);
		}
	inline void RORADC_m_zp(uword addr)
		{
		ubyte x = ROR_m(readData_zp(addr));
		writeData_zp(addr,x);
		ADC_m(x);
		}
	void RORADC_abso()
		{
		RORADC_m(abso());
		pPC += 2; 
		} 
	void RORADC_absx()
		{
		RORADC_m(absx());
		pPC += 2; 
		} 
	void RORADC_absy()
		{
		RORADC_m(absy());
		pPC += 2; 
		}  
	void RORADC_indx()
		{
		RORADC_m(indx());
		pPC++; 
		}  
	void RORADC_indy()
		{
		RORADC_m(indy());
		pPC++; 
		} 
	void RORADC_zp()
		{
		RORADC_m_zp(zp());
		pPC++; 
		}  
	void RORADC_zpx()
		{
		RORADC_m_zp(zpx());
		pPC++; 
		}


	inline void DECCMP_m(uword addr)
		{
		ubyte x = (this->*readData)(addr);
		(this->*writeData)(addr,(--x));
		CMP_m(x);
		}
	inline void DECCMP_m_zp(uword addr)
		{
		ubyte x = readData_zp(addr);
		writeData_zp(addr,(--x));
		CMP_m(x);
		}
	void DECCMP_abso()
		{
		DECCMP_m(abso());
		pPC += 2; 
		}  
	void DECCMP_absx()
		{
		DECCMP_m(absx());
		pPC += 2; 
		}  
	void DECCMP_absy()
		{
		DECCMP_m(absy());
		pPC += 2; 
		}  
	void DECCMP_indx()
		{
		DECCMP_m(indx());
		pPC++; 
		}  
	void DECCMP_indy()
		{
		DECCMP_m(indy());
		pPC++; 
		} 
	void DECCMP_zp()
		{
		DECCMP_m_zp(zp());
		pPC++; 
		} 
	void DECCMP_zpx()
		{
		DECCMP_m_zp(zpx());
		pPC++; 
		} 


	inline void INCSBC_m(uword addr)
		{
		ubyte x = (this->*readData)(addr);
		(this->*writeData)(addr,(++x));
		SBC_m(x);
		}
	inline void INCSBC_m_zp(uword addr)
		{
		ubyte x = readData_zp(addr);
		writeData_zp(addr,(++x));
		SBC_m(x);
		}
	void INCSBC_abso()
		{
		INCSBC_m(abso());
		pPC += 2; 
		} 
	void INCSBC_absx()
		{
		INCSBC_m(absx());
		pPC += 2; 
		}  
	void INCSBC_absy()
		{
		INCSBC_m(absy());
		pPC += 2; 
		}  
	void INCSBC_indx()
		{
		INCSBC_m(indx());
		pPC++; 
		}  
	void INCSBC_indy()
		{
		INCSBC_m(indy());
		pPC++; 
		} 
	void INCSBC_zp()
		{
		INCSBC_m_zp(zp());
		pPC++; 
		} 
	void INCSBC_zpx()
		{
		INCSBC_m_zp(zpx());
		pPC++; 
		}  


// --------------------------------------------------------------------------
// Illegal codes/instructions part (3). This implementation is considered to
// be only partially working due to inconsistencies in the available
// documentation.
// Note: In some of the functions emulated, defined instructions are used and
// already increment the PC ! Take care, and do not increment further !
// Double-setting of processor flags can occur, too.

	void ILL_0B() // equal to 2B
		{ 
		AND_imm();
		CF = NF;
		}
	
	void ILL_4B()
		{ 
		AND_imm();
		LSR_AC();
		}
	
	void ILL_6B()
		{ 
		if (DF == 0)
			{
			AND_imm();
			ROR_AC();
			CF = (AC & 1);
			VF = (AC >> 5) ^ (AC >> 6);
			}
		}
	
	void ILL_83()
		{
		(this->*writeData)(indx(),AC & XR);
		pPC++; 
		}

	void ILL_87()
		{
		writeData_zp(zp(),AC & XR);
		pPC++; 
		}

	void ILL_8B()
		{ 
		TXA_();
		AND_imm();
		}
	
	void ILL_8F()
		{
		(this->*writeData)(abso(),AC & XR);
		pPC += 2; 
		}

	void ILL_93()
		{
		(this->*writeData)(indy(), AC & XR & (1+((this->*readData)((*pPC)+1) & 0xFF)));
		pPC++; 
		}

	void ILL_97()
		{ 
		writeData_zp(zpx(),AC & XR);
		pPC++; 
		}

	void ILL_9B()
		{
		SP = (uword)(0x100 | (AC & XR));
		(this->*writeData)(absy(),(SP & ((*pPC+1)+1)));
		pPC += 2;
		checkSP();
		}

	void ILL_9C()
		{
		(this->*writeData)(absx(),(YR & ((*pPC+1)+1)));
		pPC += 2;
		}

	void ILL_9E()
		{ 
		(this->*writeData)(absy(),(XR & ((*pPC+1)+1)));
		pPC += 2;
		}

	void ILL_9F()
		{
		(this->*writeData)(absy(),(AC & XR & ((*pPC+1)+1)));
		pPC += 2;
		}

	void ILL_A3()
		{
		LDA_indx();
		TAX_();
		}

	void ILL_A7()
		{
		LDA_zp();
		TAX_();
		}

	void ILL_AF()
		{
		LDA_abso();
		TAX_();
		}

	void ILL_B3()
		{
		LDA_indy();
		TAX_();
		}

	void ILL_B7()
		{
		affectNZ(AC = readData_zp(zpy()));  // would be LDA_zpy()
		TAX_();
		pPC++; 
		}

	void ILL_BB()
		{
		XR = (ubyte)(SP & absy());
		pPC += 2;
		TXS_();
		TXA_();
		}

	void ILL_BF()
		{
		LDA_absy();
		TAX_();
		}

	void ILL_CB()
		{ 
		uword tmp = (uword)(XR & AC);
		tmp -= imm();
		CF = (tmp > 255);
		affectNZ(XR=(tmp&255));
		}

	void ILL_EB()
		{ 
		SBC_imm();
		}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

  public:
	ubyte* c64mem1;       ///< 64KB C64-RAM
	ubyte* c64mem2;       ///< Basic-ROM, VIC, SID, I/O, Kernal-ROM
	bool sidKeysOff[32];  ///< key_off detection
	bool sidKeysOn[32];   ///< key_on detection
	ubyte sidLastValue;   ///< last value written to the SID
	ubyte optr3readWave;  ///< D41B
	ubyte optr3readEnve;  ///< D41C
  public:
	ubyte AC, XR, YR;     ///< 6510 processor registers
	uword PC, SP;         ///< program-counter, stack-pointer
// PC is only used temporarily ! 
// The current program-counter is pPC-pPCbase
	ubyte* pPCbase;       ///< pointer to RAM/ROM buffer base
	ubyte* pPCend;        ///< pointer to RAM/ROM buffer end
	ubyte* pPC;           ///< pointer to PC location
	ubyte c64ramBuf[65536+256]; ///< RAM buffer
	ubyte c64romBuf[65536+256]; ///< ROM buffer
// 
	statusRegister SR;    ///< 8-bit status register
//
	bool isBasic;         ///< these flags are used to not have to repeatedly
	bool isIO;            ///< evaluate the bank-select register for each
	bool isKernal;        ///< address operand
	ubyte* bankSelReg;    ///< pointer to RAM[1], bank-select register
	ubyte fakeReadTimer;
 
public:
//
// Use pointers to allow plain-memory modifications.
	ubyte (C6510::*readData)(uword);
	void (C6510::*writeData)(uword, ubyte);
	bool stackIsOkay;     ///< true if the Stack is OK
//
	int memoryMode;              ///< the default is MPU_TRANSPARENT_ROM

	typedef void (C6510::*C6510ptr2func)();
	C6510ptr2func instrList[256]; ///< list of instructions

	ubyte iFakeRndSeed;           ///< Fake Random Seed

public:
	inline ubyte my_read_data(uword w)
		{
		return (this->*readData)(w);
		}
};


#endif
