/*
 * endian - determine the byte order of a long on your machine
 *
 * Copyright (C) 1999  Landon Curt Noll
 *
 * Calc is open software; you can redistribute it and/or modify it under
 * the terms of the version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 *
 * Calc is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU Lesser General
 * Public License for more details.
 *
 * A copy of version 2.1 of the GNU Lesser General Public License is
 * distributed with calc under the filename COPYING-LGPL.  You should have
 * received a copy with calc; if not, write to Free Software Foundation, Inc.
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * @(#) $Revision: 29.3 $
 * @(#) $Id: endian.c,v 29.3 2001/03/18 02:59:42 chongo Exp $
 * @(#) $Source: /usr/local/src/cmd/calc/RCS/endian.c,v $
 *
 * Under source code control:	1993/11/15 04:32:58
 * File existed as early as:	1993
 *
 * chongo <was here> /\oo/\	http://www.isthe.com/chongo/
 * Share and enjoy!  :-)	http://www.isthe.com/chongo/tech/comp/calc/
 */

/*
 * Big Endian:	    Amdahl, 68k, Pyramid, Mips, Sparc, ...
 * Little Endian:   Vax, 32k, Spim (Dec Mips), i386, i486, ...
 */


#include <stdio.h>

#include "have_unistd.h"
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

/* byte order array */
char byte[8] = { (char)0x12, (char)0x36, (char)0x48, (char)0x59,
		 (char)0x01, (char)0x23, (char)0x45, (char)0x67 };

int
main(void)
{
#if !defined(LITTLE_ENDIAN) && !defined(BIG_ENDIAN)
	/* pointers into the byte order array */
	int *intp = (int *)byte;
#endif
#if defined(DEBUG)
	short *shortp = (short *)byte;
	long *longp = (long *)byte;

	printf("byte: %02x %02x %02x %02x %02x %02x %02x %02x\n",
	    byte[0], byte[1], byte[2], byte[3],
	    byte[4], byte[5], byte[6], byte[7]);
	printf("short: %04x %04x %04x %04x\n",
	    shortp[0], shortp[1], shortp[2], shortp[3]);
	printf("int: %08x %08x\n",
	    intp[0], intp[1]);
	printf("long: %08x %08x\n",
	    longp[0], longp[1]);
#endif

	/* Print the standard <machine/endian.h> defines */
	printf("#define BIG_ENDIAN\t4321\n");
	printf("#define LITTLE_ENDIAN\t1234\n");

#if defined(LITTLE_ENDIAN)
	printf("#define CALC_BYTE_ORDER\tLITTLE_ENDIAN\n");
#elif defined(BIG_ENDIAN)
	printf("#define CALC_BYTE_ORDER\tBIG_ENDIAN\n");
#else
	/* Determine byte order */
	if (intp[0] == 0x12364859) {
	    /* Most Significant Byte first */
	    printf("#define CALC_BYTE_ORDER\tBIG_ENDIAN\n");
	} else if (intp[0] == 0x59483612) {
	    /* Least Significant Byte first */
	    printf("#define CALC_BYTE_ORDER\tLITTLE_ENDIAN\n");
	} else {
	    fprintf(stderr,
		"Unknown int Byte Order, set CALC_BYTE_ORDER in Makefile\n");
	    exit(1);
	}
#endif
	/* exit(0); */
	return 0;
}
