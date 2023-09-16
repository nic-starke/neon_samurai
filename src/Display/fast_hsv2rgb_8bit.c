/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016  B. Stultiens
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "fast_hsv2rgb.h"

#if defined(HSV_USE_ASSEMBLY) && !defined(__AVR_ARCH__)
#warning "Only AVR assembly is implemented. Other architectures use C fallback."
#undef HSV_USE_ASSEMBLY
#endif

void fast_hsv2rgb_8bit(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g , uint8_t *b)
{
#ifndef HSV_USE_ASSEMBLY
	HSV_MONOCHROMATIC_TEST(s, v, r, g, b);	// Exit with grayscale if s == 0

	uint8_t sextant = h >> 8;

	HSV_SEXTANT_TEST(sextant);		// Optional: Limit hue sextants to defined space

	HSV_POINTER_SWAP(sextant, r, g, b);	// Swap pointers depending which sextant we are in

	*g = v;		// Top level

	// Perform actual calculations
	uint8_t bb;
	uint16_t ww;

	/*
	 * Bottom level: v * (1.0 - s)
	 * --> (v * (255 - s) + error_corr) / 256
	 */
	bb = ~s;
	ww = v * bb;
	ww += 1;		// Error correction
	ww += ww >> 8;		// Error correction
	*b = ww >> 8;

	uint8_t h_fraction = h & 0xff;	// 0...255

	if(!(sextant & 1)) {
		// *r = ...slope_up...;
		/*
		 * Slope up: v * (1.0 - s * (1.0 - h))
		 * --> (v * (255 - (s * (256 - h) + error_corr1) / 256) + error_corr2) / 256
		 */
		ww = !h_fraction ? ((uint16_t)s << 8) : (s * (uint8_t)(-h_fraction));
		ww += ww >> 8;		// Error correction 1
		bb = ww >> 8;
		bb = ~bb;
		ww = v * bb;
		ww += v >> 1;		// Error correction 2
		*r = ww >> 8;
	} else {
		// *r = ...slope_down...;
		/*
		 * Slope down: v * (1.0 - s * h)
		 * --> (v * (255 - (s * h + error_corr1) / 256) + error_corr2) / 256
		 */
		ww = s * h_fraction;
		ww += ww >> 8;		// Error correction 1
		bb = ww >> 8;
		bb = ~bb;
		ww = v * bb;
		ww += v >> 1;		// Error correction 2
		*r = ww >> 8;

		/*
		 * A perfect match for h_fraction == 0 implies:
		 *	*r = (ww >> 8) + (h_fraction ? 0 : 1)
		 * However, this is an extra calculation that may not be required.
		 */
	}

#else /* HSV_USE_ASSEMBLY */

#ifdef __AVR_ARCH__
	/*
	 * Function arguments passed in registers:
	 *   h = r25:r24
	 *   s = r22
	 *   v = r20
	 *   *r = r19:r18
	 *   *g = r17:r16
	 *   *b = r15:r14
	 */
	asm volatile (
		MOVW("r27", "r26", "r19", "r18")	// r -> X
		MOVW("r29", "r28", "r17", "r16")	// g -> Y
		MOVW("r31", "r30", "r15", "r14")	// b -> Z

		"cpse	r22, __zero_reg__\n\t"	// if(!s) --> monochromatic
		"rjmp	.Lneedcalc%=\n\t"

		"st	X, r20\n\t"		// *r = *g = *b = v;
		"st	Y, r20\n\t"
		"st	Z, r20\n\t"
		"rjmp	.Lendoffunc%=\n"	// return

	".Lneedcalc%=:\n\t"

		"cpi	r25, lo8(6)\n\t"	// if(hi8(h) > 5) hi8(h) = 5;
		"brlo	.Linrange%=\n\t"
		"ldi	r25,lo8(5)\n"
	".Linrange%=:\n\t"

		"sbrs	r25, 1\n\t"		// if(sextant & 2) swapptr(r, b);
		"rjmp	.Lsextno1%=\n\t"
		MOVW("r19", "r18", "r27", "r26")
		MOVW("r27", "r26", "r31", "r30")
		MOVW("r31", "r30", "r19", "r18")
	"\n"
	".Lsextno1%=:\n\t"

		"sbrs	r25, 2\n\t"		// if(sextant & 4) swapptr(g, b);
		"rjmp	.Lsextno2%=\n\t"
		MOVW("r19", "r18", "r29", "r28")
		MOVW("r29", "r28", "r31", "r30")
		MOVW("r31", "r30", "r19", "r18")
	"\n"
	".Lsextno2%=:\n\t"

		"ldi	r18, lo8(6)\n\t"
		"and	r18, r25\n\t"		// if(!(sextant & 6))
		"brne	.Lsext2345%=\n\t"

		"sbrc	r25, 0\n\t"		// if(!(sextant & 6) && !(sextant & 1)) --> doswasp
		"rjmp	.Ldoneswap%=\n"
	".Lsext0%=:\n\t"
		MOVW("r19", "r18", "r27", "r26")
		MOVW("r27", "r26", "r29", "r28")
		MOVW("r29", "r28", "r19", "r18")
		"rjmp	.Ldoneswap%=\n"

	".Lsext2345%=:\n\t"
		"sbrc	r25, 0\n\t"		// if((sextant & 6) && (sextant & 1)) --> doswap
		"rjmp	.Lsext0%=\n"
	".Ldoneswap%=:\n\t"

		/* Top level assignment first to free up Y register (r29:r28) */
		"st	Y, r20\n\t"		// *g = v

		"ldi	r18, 0\n\t"		// Temporary zero reg (r1 is used by mul)
		"ldi	r19, 1\n\t"		// Temporary one reg

		/*
		 * Do bottom level next so we may use Z register (r31:r30).
		 *
		 *	Bottom level: v * (1.0 - s)
		 *	--> (v * (255 - s) + error_corr + 1) / 256
		 *	1 bb = ~s;
		 *	2 ww = v * bb;
		 *	3 ww += 1;
		 *	4 ww += ww >> 8;	// error_corr for division 1/256 instead of 1/255
		 *	5 *b = ww >> 8;
		 */
		"mov	r23, r22\n\t"		// 1 use copy of s
		"com	r23\n\t"		// 1
		MUL("r23", "r20", "a")		// 2 r1:r0 = v *  ~s
		"add	r0, r19\n\t"		// 3 r1:r0 += 1
		"adc	r1, r18\n\t"		// 3
		"add	r0, r1\n\t"		// 4 r1:r0 += r1:r0 >> 8
		"adc	r1, r18\n\t"		// 4
		"st	Z, r1\n\t"		// 5 *b = r1:r0 >> 8

		/* All that is left are the slopes */

		"sbrc	r25, 0\n\t"		// if(sextant & 1) --> slope down
		"rjmp	.Lslopedown%=\n\t"

		/*
		 *	Slope up: v * (1.0 - s * (1.0 - h))
		 *	--> (v * (255 - (s * (256 - h) + error_corr1) / 256) + error_corr2) / 256
		 *	0 ww = 256 - h_fraction;
		 *	1 ww = s * bb;
		 *	2 ww += ww >> 8;	// error_corr1
		 *	3 bb = ww >> 8;		// Implicit operation
		 *	4 bb = ~bb;
		 *	5 ww = v * bb;
		 *	6 ww += v >> 1;		// error_corr2
		 *	7 *r = ww >> 8;
		 */
		"ldi	r28, 0\n\t"		// 0 256
		"ldi	r29, 1\n\t"
		"sub	r28, r24\n\t"		// 0 256 - h_fraction
		"sbc	r29, r18\n\t"
		MUL("r28", "r22", "b")		// 1 r1:r0 = s * (256 - h_fraction)
		"sbrc	r29, 0\n\t"		// 1 if(256 - h_fraction == 0x100)
		"add	r1, r22\n\t"		// 1   r1:r0 += s << 8
		"rjmp	.Lslopecommon%=\n\t"	// r1:r0 holds inner multiplication

		/*
		 *	Slope down: v * (1.0 - s * h)
		 *	--> (v * (255 - (s * h + error_corr1) / 256) + error_corr2) / 256
		 *	1 ww = s * h_fraction;
		 *	2 ww += ww >> 8;	// error_corr1
		 *	3 bb = ww >> 8;		// Implicit operation
		 *	4 bb = ~bb;
		 *	5 ww = v * bb;
		 *	6 ww += v >> 1;		// error_corr2
		 *	7 *r = ww >> 8;
		 */
	"\n"
	".Lslopedown%=:\n\t"
		MUL("r24", "r22", "c")		// 1 r1:r0 = s * h_fraction
	"\n"
	".Lslopecommon%=:\n\t"
		"add	r0, r1\n\t"		// 2 error_corr1
		"adc	r1, r18\n\t"		// 2
		"com	r1\n\t"			// 4 bb = ~bb
		MUL("r1", "r20", "d")		// 5 r1:r0 = v * bb
		"lsr	r20\n\t"		// 6 error_corr2: v >>= 1
		"add	r0, r20\n\t"		// 6 r1:r0 += v >> 1
		"adc	r1, r18\n"		// 6
		"st	X, r1\n\t"		// 7 *r = slope result

		"clr	__zero_reg__\n"		// Restore zero reg

	".Lendoffunc%=:\n"
	:
	:
	: "r31", "r30", "r29", "r28", "r27", "r26", "r25", "r24", "r22", "r19", "r18"
#ifndef __AVR_HAVE_MUL__
	 , "r17", "r16"
#endif
	);
#else /* __AVR_ARCH__ */
#error "No assembly version implemented for architecture"
#endif
#endif /* HSV_USE_ASSEMBLY */
}
