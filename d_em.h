/* Copyright(C) Caldera International Inc.  2001-2002.  All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
following conditions are met:

Redistributions of source code and documentation must retain the above copyright notice, this list of conditions and the
following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list of conditions
and the following disclaimer in the documentation and/or other materials provided with the distribution.

All advertising materials mentioning features or use of this software must display the following acknowledgement:
This product includes software developed or owned by Caldera International, Inc.
Neither the name of Caldera International, Inc. nor the names of other contributors may be used to endorse or promote
products derived from this software without specific prior written permission.

USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA INTERNATIONAL, INC.
AND CONTRIBUTORS ``AS  IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE FOR
ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE. */

/*
 * Define some macros for the regular expression 
 * routines to use for input and error stuff.
 */

#define INIT        extern int peekc;
#define GETC()      getchr()
#define UNGETC(c)   (peekc = c)
#define PEEKC()     (peekc = getchr())
#define RETURN(c)   return 0
#define ERROR(c)    error1(c)

/* screen dimensions */
#define LINES       18
#define LENGTH      78
#define SPLIT       '-'
#define PROMPT      '>'
#define CONFIRM     '.'
#define SCORE       "^"
#define FORM        014

/* mods by atsb */
#define TEMPORARY_INIT_FILE ".em_tmp"

#define	PUTM()      if(xcode >= 0) em_puts(msgtab[xcode])
#define	FNSIZE      64
#define	LBSIZE      512
#define ESIZE       256
#define	GBSIZE      256
#define	KSIZE       9
#define	READ        0
#define	WRITE       1
#define PRNT        02
