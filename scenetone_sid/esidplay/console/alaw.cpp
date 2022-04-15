/**
 * @file alaw.cpp Implements the static Alaw utility class.
 *
 *  A-Law conversion Plug-In Interface
 *  Copyright (c) 1999 by Jaroslav Kysela <perex@suse.cz>
 *                        Uros Bizjak <uros@kss-loka.si>
 *
 *  Based on reference implementation by Sun Microsystems, Inc.
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "alaw.h"

#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

static const short alaw_seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF,
				0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

inline int Alaw::search(int val, const short *table, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (val <= *table++)
			return (i);
	}
	return (size);
}

/*
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 * linear2alaw() accepts an 16-bit integer and encodes it as A-law data.
 *
 *		Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	0000000wxyza			000wxyz
 *	0000001wxyza			001wxyz
 *	000001wxyzab			010wxyz
 *	00001wxyzabc			011wxyz
 *	0001wxyzabcd			100wxyz
 *	001wxyzabcde			101wxyz
 *	01wxyzabcdef			110wxyz
 *	1wxyzabcdefg			111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
inline unsigned char Alaw::linear2alaw(int pcm_val)	/* 2's complement (16-bit range) */
{
	int		mask;
	int		seg;
	unsigned char	aval;

	if (pcm_val >= 0) {
		mask = 0xD5;		/* sign (7th) bit = 1 */
	} else {
		mask = 0x55;		/* sign bit = 0 */
		pcm_val = -pcm_val - 8;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, alaw_seg_end, NSEGS);

	/* Combine the sign, segment, and quantization bits. */

	if (seg >= 8)		/* out of range, return maximum value. */
		return (unsigned char)(0x7F ^ mask);
	else {
		aval = (unsigned char)(seg << SEG_SHIFT);
		if (seg < 2)
			aval |= (pcm_val >> 4) & QUANT_MASK;
		else
			aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
		return (unsigned char)(aval ^ mask);
	}
}


void Alaw::conv_u8bit_alaw(unsigned char *src_ptr, unsigned char *dst_ptr, size_t size)
{
	unsigned int pcm;

	while (size-- > 0) {
		pcm = ((*src_ptr++) ^ 0x80) << 8;
		*dst_ptr++ = linear2alaw((signed short)(pcm));
	}
}

void Alaw::conv_s8bit_alaw(unsigned char *src_ptr, unsigned char *dst_ptr, size_t size)
{
	unsigned int pcm;

	while (size-- > 0) {
		pcm = *src_ptr++ << 8;
		*dst_ptr++ = Alaw::linear2alaw((signed short)(pcm));
	}
}

void Alaw::conv_s16bit_alaw(unsigned short *src_ptr, unsigned char *dst_ptr, size_t size)
{
	while (size-- > 0)
		*dst_ptr++ = Alaw::linear2alaw((signed short)(*src_ptr++));
}

/*
static void alaw_conv_s16bit_swap_alaw(unsigned short *src_ptr, unsigned char *dst_ptr, size_t size)
{
	while (size-- > 0)
		*dst_ptr++ = linear2alaw((signed short)(bswap_16(*src_ptr++)));
}
*/

void Alaw::conv_u16bit_alaw(unsigned short *src_ptr, unsigned char *dst_ptr, size_t size)
{
	while (size-- > 0)
		*dst_ptr++ = Alaw::linear2alaw((signed short)((*src_ptr++) ^ 0x8000));
}

/*
static void alaw_conv_u16bit_swap_alaw(unsigned short *src_ptr, unsigned char *dst_ptr, size_t size)
{
	while (size-- > 0)
		*dst_ptr++ = linear2alaw((signed short)(bswap_16(*src_ptr++) ^ 0x8000));
}
*/

/*
static void alaw_conv_alaw_u8bit(unsigned char *src_ptr, unsigned char *dst_ptr, size_t size)
{
	while (size-- > 0)
		*dst_ptr++ = (alaw2linear(*src_ptr++) >> 8) ^ 0x80;
}

static void alaw_conv_alaw_s8bit(unsigned char *src_ptr, unsigned char *dst_ptr, size_t size)
{
	while (size-- > 0)
		*dst_ptr++ = alaw2linear(*src_ptr++) >> 8;
}

static void alaw_conv_alaw_s16bit(unsigned char *src_ptr, unsigned short *dst_ptr, size_t size)
{
	while (size-- > 0)
		*dst_ptr++ = alaw2linear(*src_ptr++);
}

static void alaw_conv_alaw_swap_s16bit(unsigned char *src_ptr, unsigned short *dst_ptr, size_t size)
{
	while (size-- > 0)
		*dst_ptr++ = bswap_16(alaw2linear(*src_ptr++));
}

static void alaw_conv_alaw_u16bit(unsigned char *src_ptr, unsigned short *dst_ptr, size_t size)
{
	while (size-- > 0)
		*dst_ptr++ = alaw2linear(*src_ptr++) ^ 0x8000;
}
*/

/*
static void alaw_conv_alaw_swap_u16bit(unsigned char *src_ptr, unsigned short *dst_ptr, size_t size)
{
	while (size-- > 0)
		*dst_ptr++ = bswap_16(alaw2linear(*src_ptr++) ^ 0x8000);
}
*/
