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

char *msgtab[] =
{
    "write or open on pipe failed",         /*  0 */
    "warning: expecting `w'",           /*  1 */
    "mark not lower case",              /*  2 */
    "cannot open input file",           /*  3 */
    "PWB spec problem",             /*  4 */
    "nothing to undo",              /*  5 */
    "restricted shell",             /*  6 */
    "cannot create output file",            /*  7 */
    "filesystem out of space!",         /*  8 */
    "cannot open file",             /*  9 */
    "cannot link",                  /* 10 */
    "Range endpoint too large",         /* 11 */
    "unknown command",              /* 12 */
    "search string not found",          /* 13 */
    "-",                        /* 14 */
    "line out of range",                /* 15 */
    "bad number",                   /* 16 */
    "bad range",                    /* 17 */
    "Illegal address count",            /* 18 */
    "incomplete global expression",         /* 19 */
    "illegal suffix",               /* 20 */
    "illegal or missing filename",          /* 21 */
    "no space after command",           /* 22 */
    "fork failed - try again",          /* 23 */
    "maximum of 64 characters in file names",   /* 24 */
    "`\\digit' out of range",           /* 25 */
    "interrupt",                    /* 26 */
    "line too long",                /* 27 */
    "illegal character in input file",      /* 28 */
    "write error",                  /* 29 */
    "out of memory for append",         /* 30 */
    "temp file too big",                /* 31 */
    "I/O error on temp file",           /* 32 */
    "multiple globals not allowed",         /* 33 */
    "global too long",              /* 34 */
    "no match",                 /* 35 */
    "illegal or missing delimiter",         /* 36 */
    "-",                        /* 37 */
    "replacement string too long",          /* 38 */
    "illegal move destination",         /* 39 */
    "-",                        /* 40 */
    "no remembered search string",          /* 41 */
    "'\\( \\)' imbalance",              /* 42 */
    "Too many `\\(' s",             /* 43 */
    "more than 2 numbers given",            /* 44 */
    "'\\}' expected",               /* 45 */
    "first number exceeds second",          /* 46 */
    "incomplete substitute",            /* 47 */
    "newline unexpected",               /* 48 */
    "'[ ]' imbalance",              /* 49 */
    "regular expression overflow",          /* 50 */
    "regular expression error",         /* 51 */
    "command expected",             /* 52 */
    "a, i, or c not allowed in G",          /* 53 */
    "end of line expected",             /* 54 */
    "no remembered replacement string",     /* 55 */
    "no remembered command",            /* 56 */
    "illegal redirection",              /* 57 */
    "possible concurrent update",           /* 58 */
    "that command confuses yed",            /* 59 */
    "the x command has become X (upper case)",  /* 60 */
    "Warning: 'w' may destroy input file (due to `illegal char' read earlier)", /* 61 */
    "Caution: 'q' may lose data in buffer; 'w' may destroy input file", /* 62 */
    0
};

int Xqt = 0;
int lastc;
char savedfile[MAXPATHLEN];
char efile[MAXPATHLEN];
char funny[LBSIZE];
char linebuf[LBSIZE];
char rhsbuf[LBSIZE/2];
char expbuf[ESIZE+4];

int globflg;
int initflg;
char genbuf[LBSIZE];
long count;
char *nextip;
char *linebp;
int ninbuf;
int peekc;
int io;
int (*oldhup)();
int (*oldquit)(), (*oldpipe)();
int vflag = 1;
int yflag;
int hflag;
int xcode = -1;

int col;
char *globp;
int tfile = -1;
int tline;
char *tfname;
extern char *locs;
char ibuff[512];
int iblock = -1;
char obuff[512];
int oblock = -1;
int ichanged;
int nleft;
int names[26];
int anymarks;
int subnewa;
int fchange;
int nline;
int fflg, shflg;
char prompt[16] = ">";
int wrapp=0;
int rflg;
int readflg;
int eflg;
int ncflg;
int listn;
int listf;
int pflag;
int flag28 = 0; /* Prevents write after a partial read */
int save28 = 0; /* Flag whether buffer empty at start of read */

long savtime;
char *name = "SHELL";
char *rshell = "/bin/rsh";
char *val;
char *home;

int Short = 0;
int oldmask; /* No umask while writing */

jmp_buf savej;

#ifdef  NULLS
int nulls;  /* Null count */
#endif

long ccount;
int errcnt = 0;