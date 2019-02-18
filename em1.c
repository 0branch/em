/**********************************************************************
 *   Copyright (c) Digital Equipment Corporation 1984, 1985, 1986.    *
 *   All Rights Reserved.                         *
 *   Reference "/usr/src/COPYRIGHT" for applicable restrictions.      *
 **********************************************************************/

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
 * Unix 6
 * Editor
 * Original code by Ken Thompson
 *
 * QMC mods Feb. 76, by  George Coulouris
 * mods are:
	prompts (suppress with '-p' flag)
	",%,&, to display a screen full of context
	'x' - as 's' but interactive
	'n' flag when appended to 's' or 'x' commands prints number of replacements
 * also mods by jrh 26 Feb 76
	% == current file name in ! commands
	!! == repeat the last ! command you executed
	-e flag == "elfic" mode :-
		no "w", "r\n" commands, auto w before q
    More mods by George March 76:
	'o' command for text input with local editing via control keys
	'b' to set a threshold for automatic line breaks in 'o' mode.
	'h' displays a screen full of help with editor commands
		(the help is in /usr/lib/emhelp)
bugs:
	should not use printf in substitute()
	(for space reasons).
 */

/* this file contains all of the code except that used in the 'o' command.
	that is in a second segment called em2.c */

static char sccsid[] =  "@(#)ed.c   3.2 12/31/87";
/*
 * SYSTEM V ed text editor
 */

/* Based on:  (SYSTEM V)  ed.c   1.23  */

/*
 * Changes for ULTRIX-11:
 *  Added 'W', append to an existing file.
 *  Added #ifdef FSPEC for format specifications.
 */

/*
**  The assembly code for ed.c should be
**  altered making the .data's for the
**  following array .text's so that it
**  can be shared (a la the Shell).
**  Use the 'edfun' script to accomplish this.
*/

#include "i_em.h"
#include "d_em.h"
#include "v_em.h"
#include "s_em.h"
#include "regexp.h"

char *em_getline();
char *getblock();
char *place();

onpipe()
{
    error(0);
}

main(argc, argv)
char    **argv;
{
    register    char *p1, *p2;
    extern  int onintr(), quit(), onhup();
    int (*oldintr)();

    oldquit = sigset(SIGQUIT, SIG_IGN);
    oldhup = sigset(SIGHUP, SIG_IGN);
    oldintr = sigset(SIGINT, SIG_IGN);
    oldpipe = sigset(SIGPIPE, onpipe);
    if (((int)sigset(SIGTERM, SIG_IGN)&01) == 0)
        sigset(SIGTERM, quit);
    p1 = *argv;
    while(*p1++);
    while(--p1 >= *argv)
        if(*p1 == '/')
            break;
    *argv = p1 + 1;
    /* if SHELL set in environment and is /bin/rsh, set rflg */
    if((val = getenv(name)) != NULL)
        if (strcmp(val, rshell) == 0)
            rflg++;
    if (**argv == 'r')
        rflg++;
    home = getenv("HOME");
    argv++;
    while (argc > 1 && **argv=='-') {
        switch((*argv)[1]) {

        case '\0':
            vflag = 0;
            break;

        case 'p':
            argv++;
            argc--;
            if(!*argv) {
                printf("ed: -p arg missing\n");
                exit(2);
            }
            strncpy(prompt, *argv, 16);
            shflg = 1;
            break;

        case 'q':
            sigset(SIGQUIT, SIG_DFL);
            vflag = 1;
            break;

        case 'y':
            yflag = 03;
            break;
        }
        argv++;
        argc--;
    }
    if (argc>1) {
        p1 = *argv;
        if(strlen(p1) >= FNSIZE) {
            em_puts("file name too long");
            exit(2);
        }
        p2 = savedfile;
        while (*p2++ = *p1++);
        globp = "r";
        fflg++;
    }
    else    /* editing with no file so set savtime to 0 */
        savtime = 0;
    eflg++;
    fendcore = (LINE)sbrk(0);
    puts("Editor");
    init();
    if (((int)oldintr&01) == 0)
        sigset(SIGINT, onintr);
    if (((int)oldhup&01) == 0)
        sigset(SIGHUP, onhup);
    setjmp(savej);
    commands();
    quit();
}

commands()
{
    int getfile(), gettty();
    register LINE a1;
    register c;
    register char *p1, *p2;
    int n, cc;
    char buf[FNSIZE];

    for (;;) {
    nodelim = 0;
    if ( pflag ) {
        pflag = 0;
        addr1 = addr2 = dot;
        goto print;
    }
    if (shflg && globp==0)
        write(1, prompt, strlen(prompt));
    addr1 = 0;
    addr2 = 0;
    if((c=getchr()) == ',') {
        addr1 = zero + 1;
        addr2 = dol;
        c = getchr();
        goto swch;
    } else if(c == ';') {
        addr1 = dot;
        addr2 = dol;
        c = getchr();
        goto swch;
    } else
        peekc = c;
    do {
        addr1 = addr2;
        if ((a1 = address())==0) {
            c = getchr();
            break;
        }
        addr2 = a1;
        if ((c=getchr()) == ';') {
            c = ',';
            dot = a1;
        }
    } while (c==',');
    if (addr1==0)
        addr1 = addr2;
swch:
    switch(c) {

    case 'a':
                setdot();
                newline();
                append(gettty, addr2);
        continue;

    case 'c':
        delete();
        append(gettty, addr1-1);
        continue;

    case 'd':
        delete();
        continue;

    case 'E':
        fchange = 0;
        c = 'e';
    case 'e':
        fflg++;
        setnoaddr();
        if (vflag && fchange) {
            fchange = 0;
            error(1);
        }
        filename(c);
        eflg++;
        init();
        addr2 = zero;
        goto caseread;

    case 'f':
        setnoaddr();
        filename(c);
        if (!ncflg)  /* there is a filename */
            getime();
        else
            ncflg--;
        em_puts(savedfile);
        continue;

    case 'g':
        global(1);
        continue;
    case 'G':
        globaln(1);
        continue;

    case 'h':
        newline();
        setnoaddr();
        PUTM();
        continue;

    case 'H':
        newline();
        setnoaddr();
        if(!hflag) {
            hflag = 1;
            PUTM();
        }
        else
            hflag = 0;
        continue;

    case 'i':
        setdot();
        nonzero();
        newline();
        if (!globflg) save();
        append(gettty, addr2-1);
        if (dot == addr2-1)
            dot += 1;
        continue;


    case 'j':
        if (addr2==0) {
            addr1 = dot;
            addr2 = dot+1;
        }
        setdot();
        newline();
        nonzero();
        if (!globflg) save();
        join();
        continue;

    case 'k':
        if ((c = getchr()) < 'a' || c > 'z')
            error(2);
        newline();
        setdot();
        nonzero();
        names[c-'a'] = addr2->cur & ~01;
        anymarks |= 01;
        continue;

    case 'm':
        move(0);
        continue;

    case '\n':
        if (addr2==0)
            addr2 = dot+1;
        addr1 = addr2;
        goto print;

    case 'n':
        listn++;
        newline();
        goto print;

    case 'l':
        listf++;
    case 'p':
        newline();
    print:
        setdot();
        nonzero();
        a1 = addr1;
        do {
            if (listn) {
                count = a1 - zero;
                putd();
                putchr('\t');
            }
            em_puts(em_getline(a1++->cur));
        }
        while (a1 <= addr2);
        dot = addr2;
        pflag = 0;
        listn = 0;
        listf = 0;
        continue;

    case 'Q':
        fchange = 0;
    case 'q':
        setnoaddr();
        newline();
        quit();

    case 'r':
        filename(c);
    caseread:
        readflg = 1;
        save28 = (dol != fendcore);
        if ((io = eopen(efile, 0)) < 0) {
            lastc = '\n';
            /* if first entering editor and file does not exist */
            /* set saved access time to 0 */
            if (eflg) {
                savtime = 0;
                eflg  = 0;
            }
            error(3);
        }
        /* get last mod time of file */
        /* eflg - entered editor with ed or e  */
        if (eflg) {
            eflg = 0;
            getime();
        }
        setall();
        ninbuf = 0;
        n = zero != dol;
#ifdef NULLS
        nulls = 0;
#endif
        if (!globflg && (c == 'r')) save();
        append(getfile, addr2);
        exfile();
        readflg = 0;
        fchange = n;
        continue;

    case 's':
        setdot();
        nonzero();
        if (!globflg) save();
        substitute(globp!=0);
        continue;

    case 't':
        move(1);
        continue;

    case 'u':
        setdot();
        newline();
        if (!initflg) undo();
        else error(5);
        fchange = 1;
        continue;

    case 'v':
        global(0);
        continue;
    case 'V':
        globaln(0);
        continue;

    case 'W':
        wrapp++;

    case 'w':
        if(flag28){flag28 = 0; fchange = 0; error(61);}
        setall();
        if((zero != dol) && (addr1 <= zero || addr2 > dol))
            error(15);
        filename(c);
        if(Xqt) {
            io = eopen(efile, 1);
            n = 1;  /* set n so newtime will not execute */
        } else {
            fstat(tfile,&Tf);
            if (!wrapp || ((io = open(efile, 1)) == -1) ||
              ((lseek(io, 0L, 2)) == -1)) {

                if (stat(efile, &Fl) < 0) {
                    wrapp=0;    /* do w not W, even if W was specified since file not found */
                    if((io = creat(efile, 0666)) < 0)
                        error(7);
                    fstat(io, &Fl);
                    Fl.st_mtime = 0;
                    close(io);
                }
                else {
#ifndef RESEARCH
                    oldmask = umask(0);
#endif
                }
            }
            if (Fl.st_nlink == 1 && (Fl.st_mode & S_IFMT) == S_IFREG) {
                if (wrapp)  /* append to file */
                    goto outj;
                if (close(open(efile, 1)) < 0)
                    error(9);
                p1 = savedfile;
                p2 = efile;
                if (!(n=strcmp(p1, p2)))
                    chktime();
                mkfunny();
                if ((io = creat(funny, Fl.st_mode)) >= 0) {
                    chown(funny, Fl.st_uid, Fl.st_gid);
                    chmod(funny, Fl.st_mode);
                    wrapp=0;
                    putfile();
                    exfile();
                    lstat(efile, &Lc);
                    if ((Lc.st_mode & S_IFMT) == S_IFLNK) {
                        if( funny, efile, &Fl < 0 )
                            error(10);

                    } else {
                        if (0 != unlink(efile))
                            perror("From file");
                        if( link(funny, efile) < 0)
                            error(10);
                    }
                    if (0 != unlink(funny))
                        perror("From funny");
                    /* if filenames are the same */
                    if (!n)
                        newtime();
                    /* check if entire buffer was written */
                    fchange = ((addr1==zero || addr1==zero+1) && addr2==dol)?0:fchange;
                    continue;
                }
            }
            else   n = 1;   /* set n so newtime will not execute*/
            if (!wrapp) {
                if((io = creat(efile, 0666)) < 0)
                    error(7);
            } else
                error(1);
        }
outj:
        wrapp=0;
        putfile();
        exfile();
        if (!n) newtime();
        fchange = ((addr1==zero||addr1==zero+1)&&addr2==dol)?0:fchange;
        continue;

    case '"':
        setdot();
        newline();
        nonzero();
        dot = addr1;
            if (dot == dol)
                 em_puts("em: error with lines");
        addr1 = dot+1;
        addr2 = dot +LINES-1;
        addr2 = addr2>dol? dol: addr2;
    outlines:
        putchr(FORM);
        a1 = addr1-1;
            while (++a1 <= addr2)
                em_puts(em_getline(*a1,0));
        dot = addr2;
         continue;

    case '&':
        setdot();
        newline();
        nonzero();
        dot = addr1;
        addr1 = dot - (LINES-2);
        addr2 = dot;
        addr1 = addr1>zero? addr1: zero+1;
        goto outlines;

    case '*':
        newline();
        setdot();
        nonzero();
        dot = addr1;
        addr1 = dot - (LINES-6);
        addr2 = dot;
        addr1 = addr1>zero? addr1: zero-1;
        goto outlines;
            
    case '%':
        newline();
        setdot();
        nonzero();
        dot = addr1;
        addr1 = dot - (LINES/2 - 2);
        addr2 = dot + (LINES/2 - 2);
        addr1 = addr1>zero? addr1 : zero+1;
        addr2 = addr2>dol? dol : addr2;
        a1 = addr1 - 1;
        putchr(FORM);
        while(++a1 <= addr2) {
            if (a1 == dot) screensplit();
            em_puts(em_getline(*a1));
            if (a1 == dot) screensplit();
        }
        continue;

    case '>':
        vflag = vflag>0? 0: vflag;
                
    case '<':
        vflag = 1;

    case '=':
        setall();
        newline();
        count = (addr2-zero)&077777;
        putd();
        putchr('\n');
        continue;

    case '!':
        unixcom();
        continue;

    case EOF:
        return 0;

    case 'P':
        if (yflag)
            error(59);
        setnoaddr();
        newline();
        if (shflg)
            shflg = 0;
        else
            shflg++;
        continue;
    }
    if (c == 'x')
        error(60);
    else
        error(12);
    error(12);
    }
}

LINE 
address()
{
    register minus, c;
    register LINE a1;
    int n, relerr;

    minus = 0;
    a1 = 0;
    for (;;) {
        c = getchr();
        if ('0'<=c && c<='9') {
            n = 0;
            do {
                n *= 10;
                n += c - '0';
            } while ((c = getchr())>='0' && c<='9');
            peekc = c;
            if (a1==0)
                a1 = zero;
            if (minus<0)
                n = -n;
            a1 += n;
            minus = 0;
            continue;
        }
        relerr = 0;
        if (a1 || minus)
            relerr++;
        switch(c) {
        case ' ':
        case '\t':
            continue;
    
        case '+':
            minus++;
            if (a1==0)
                a1 = dot;
            continue;

        case '-':
        case '^':
            minus--;
            if (a1==0)
                a1 = dot;
            continue;
    
        case '?':
        case '/':
            compile((char *) 0, expbuf, &expbuf[ESIZE], c);
            a1 = dot;
            for (;;) {
                if (c=='/') {
                    a1++;
                    if (a1 > dol)
                        a1 = zero;
                } else {
                    a1--;
                    if (a1 < zero)
                        a1 = dol;
                }
                if (execute(0, a1))
                    break;
                if (a1==dot)
                    error(13);
            }
            break;
    
        case '$':
            a1 = dol;
            break;
    
        case '.':
            a1 = dot;
            break;

        case '\'':
            if ((c = getchr()) < 'a' || c > 'z')
                error(2);
            for (a1=zero; a1<=dol; a1++)
                if (names[c-'a'] == (a1->cur & ~01))
                    break;
            break;
    
        case 'y' & 037:
            if(yflag) {
                newline();
                setnoaddr();
                yflag ^= 01;
                continue;
            }

        default:
            peekc = c;
            if (a1==0)
                return(0);
            a1 += minus;
            if (a1<zero || a1>dol)
                error(15);
            return(a1);
        }
        if (relerr)
            error(16);
    }
}

setdot()
{
    if (addr2 == 0)
        addr1 = addr2 = dot;
    if (addr1 > addr2)
        error(17);
}

setall()
{
    if (addr2==0) {
        addr1 = zero+1;
        addr2 = dol;
        if (dol==zero)
            addr1 = zero;
    }
    setdot();
}

setnoaddr()
{
    if (addr2)
        error(18);
}

nonzero()
{
    if (addr1<=zero || addr2>dol)
        error(15);
}

newline()
{
    register c;

    c = getchr();
    if ( c == 'p' || c == 'l' || c == 'n' ) {
        pflag++;
        if ( c == 'l') listf++;
        if ( c == 'n') listn++;
        c = getchr();
    }
    if ( c != '\n')
        error(20);
}

filename(comm)
{
    register char *p1, *p2;
    register c;
    register i = 0;

    count = 0;
    c = getchr();
    if (c=='\n' || c==EOF) {
        p1 = savedfile;
        if (*p1==0 && comm!='f')
            error(21);
        /* ncflg set means do not get mod time of file */
        /* since no filename followed f */
        if (comm == 'f')
            ncflg++;
        p2 = efile;
        while (*p2++ = *p1++);
        red(savedfile);
        return 0;
    }
    if (c!=' ')
        error(22);
    while ((c = getchr()) == ' ');
    if(c == '!')
        ++Xqt, c = getchr();
    if (c=='\n')
        error(21);
    p1 = efile;
    do {
        if(++i >= FNSIZE)
            error(24);
        *p1++ = c;
        if(c==EOF || (c==' ' && !Xqt))
            error(21);
    } while ((c = getchr()) != '\n');
    *p1++ = 0;
    if(Xqt)
        if (comm=='f') {
            --Xqt;
            error(57);
        }
        else
            return 0;
    if (savedfile[0]==0 || comm=='e' || comm=='f') {
        p1 = savedfile;
        p2 = efile;
        while (*p1++ = *p2++);
    }
    red(efile);
}

exfile()
{
#ifdef NULLS
    register c;
#endif

#ifndef RESEARCH
    if(oldmask) {
        umask(oldmask);
        oldmask = 0;
    }
#endif
    eclose(io);
    io = -1;
    if (vflag) {
        putd();
        putchr('\n');
#ifdef NULLS
        if(nulls) {
            c = count;
            count = nulls;
            nulls = 0;
            putd();
            em_puts(" nulls replaced by '\\0'");
            count = c;
        }
#endif
    }
}

onintr()
{
    sigset(SIGINT, onintr);
    putchr('\n');
    lastc = '\n';
    if (*funny) unlink(funny); /* remove tmp file */
    /* if interruped a read, only part of file may be in buffer */
    if ( readflg ) {
        em_puts ("\007read may be incomplete - beware!\007");
        fchange = 0;
    }
    error(26);
}

onhup()
{
    sigset(SIGINT, SIG_IGN);
    sigset(SIGHUP, SIG_IGN);
    /* if there are lines in file and file was */
    /* not written since last update, save in em.hup, or $HOME/em.hup */
    if (dol > zero && fchange == 1) {
        addr1 = zero+1;
        addr2 = dol;
        io = creat("em.hup", 0666);
        if(io < 0 && home) {
            char    *fn;

            fn = calloc(strlen(home) + 8, sizeof(char));
            if(fn) {
                strcpy(fn, home);
                strcat(fn, "/em.hup");
                io = creat(fn, 0666);
                free(fn);
            }
        }
        if (io > 0)
            putfile();
    }
    fchange = 0;
    quit();
}

error(code)
register code;
{
    register c;

    if(code == 28 && save28 == 0){fchange = 0; flag28++;}
    readflg = 0;
    ++errcnt;
    listf = listn = 0;
    pflag = 0;
#ifndef RESEARCH
    if(oldmask) {
        umask(oldmask);
        oldmask = 0;
    }
#endif
#ifdef NULLS    /* Not really nulls, but close enough */
    /* This is a bug because of buffering */
    if(code == 28) /* illegal char. */
        putd();
#endif
    putchr('?');
    if(code == 3)   /* Cant open file */
        em_puts(efile);
    else
        putchr('\n');
    count = 0;
    lseek(0, (long)0, 2);
    if (globp)
        lastc = '\n';
    globp = 0;
    peekc = lastc;
    if(lastc)
        while ((c = getchr()) != '\n' && c != EOF);
    if (io > 0) {
        eclose(io);
        io = -1;
    }
    xcode = code;
    if(hflag)
        PUTM();
    if(code==4)return(0);   /* Non-fatal error. */
    longjmp(savej, 1);
}

getchr()
{
    char c;
    if (lastc=peekc) {
        peekc = 0;
        return(lastc);
    }
    if (globp) {
        if ((lastc = *globp++) != 0)
            return(lastc);
        globp = 0;
        return(EOF);
    }
    if (read(0, &c, 1) <= 0)
        return(lastc = EOF);
    lastc = c&0177;
    return(lastc);
}

gettty()
{
    register c;
    register char *gf;
    register char *p;

    p = linebuf;
    gf = globp;
    while ((c = getchr()) != '\n') {
        if (c==EOF) {
            if (gf)
                peekc = c;
            return(c);
        }
        if ((c &= 0177) == 0)
            continue;
        *p++ = c;
        if (p >= &linebuf[LBSIZE-2])
            error(27);
    }
    *p++ = 0;
    if (linebuf[0]=='.' && linebuf[1]==0)
        return(EOF);
    if (linebuf[0]=='\\' && linebuf[1]=='.' && linebuf[2]==0) {
        linebuf[0] = '.';
        linebuf[1] = 0;
    }
    return(0);
}

getfile()
{
    register c;
    register char *lp, *fp;
    int crflag;

    crflag = 0;
    lp = linebuf;
    fp = nextip;
    do {
        if (--ninbuf < 0) {
            if ((ninbuf = read(io, genbuf, LBSIZE)-1) < 0)
                return(EOF);
            fp = genbuf;
            while(fp < &genbuf[ninbuf])
                if(*fp++ & 0200) crflag = 1;
            if(crflag){
                    error(28);
            }
            fp = genbuf;
            while(fp < &genbuf[ninbuf]){
                if(*fp++ & 0200) error(28);
            }
            fp = genbuf;
        }
        if (lp >= &linebuf[LBSIZE]) {
            lastc = '\n';
            error(27);
        }
        if ((*lp++ = c = *fp++ & 0177) == 0) {
#ifdef NULLS
            lp[-1] = '\\';
            *lp++ = '0';
            nulls++;
#else
            lp--;
            continue;
#endif
        }
        count++;
    } while (c != '\n');
    *--lp = 0;
    nextip = fp;
    if (fss.Ffill && fss.Flim && lenchk(linebuf,&fss) < 0) {
        write(1,"line too long: lno = ",21);
        ccount = count;
        count = (++dot-zero)&077777;
        dot--;
        putd();
        count = ccount;
        putchr('\n');
    }
    return(0);
}

putfile()
{
    int n;
    LINE a1;
    register char *fp, *lp;
    register nib;

    nib = 512;
    fp = genbuf;
    a1 = addr1;
    do {
        lp = em_getline(a1++->cur);
        if (fss.Ffill && fss.Flim && lenchk(linebuf,&fss) < 0) {
            write(1,"line too long: lno = ",21);
            ccount = count;
            count = (++dot-zero)&077777;
            dot--;
            putd();
            count = ccount;
            putchr('\n');
        }
        for (;;) {
            if (--nib < 0) {
                n = fp-genbuf;
                if(write(io, genbuf, n) != n)
                    error(29);
                nib = 511;
                fp = genbuf;
            }
            if(dol->cur == 0)break; /* Allow write of null file */
            count++;
            if ((*fp++ = *lp++) == 0) {
                fp[-1] = '\n';
                break;
            }
        }
    } while (a1 <= addr2);
    n = fp-genbuf;
    if(write(io, genbuf, n) != n)
        error(29);
}

append(f, a)
LINE a;
int (*f)();
{
    register LINE a1, a2, rdot;
    int tl;

    nline = 0;
    dot = a;
    while ((*f)() == 0) {
        if (dol >= endcore) {
            if ((int)sbrk(512*sizeof(struct lin)) == -1) {
                lastc = '\n';
                error(30);
            }
            endcore += 512;
        }
        tl = putline();
        nline++;
        a1 = ++dol;
        a2 = a1+1;
        rdot = ++dot;
        while (a1 > rdot)
            (--a2)->cur = (--a1)->cur;
        rdot->cur = tl;
    }
}

unixcom()
{
    register (*savint)(), pid, rpid;
    int retcode;
    static char savcmd[LBSIZE]; /* last command */
    char curcmd[LBSIZE];        /* current command */
    char *psavcmd, *pcurcmd, *psavedfile;
    register c, endflg=1, shflg=0;

    setnoaddr();
    if(rflg)
        error(6);
    pcurcmd = curcmd;
    /* read command til end */
    /* a '!' found in beginning of command is replaced with the saved command.
       a '%' found in command is replaced with the current filename */
    c=getchr();
    if (c == '!') {
        if (savcmd[0]==0) 
            error(56);
        else {
            psavcmd = savcmd;
            while (*pcurcmd++ = *psavcmd++);
            --pcurcmd;
            shflg = 1;
        }
    }
    else UNGETC(c);  /* put c back */
    while (endflg==1) {
        while ((c=getchr()) != '\n' && c != '%' && c != '\\')
            *pcurcmd++ = c;
        if (c=='%') { 
            if (savedfile[0]==0)
                error(21);
            else {
                psavedfile = savedfile;
                while(*pcurcmd++ = *psavedfile++);
                --pcurcmd;
                shflg = 1;
            }
        }
        else if (c == '\\') {
            c = getchr();
            if (c != '%')
                *pcurcmd++ = '\\';
            *pcurcmd++ = c;
        }
        else
            /* end of command hit */
            endflg = 0;
    }
    *pcurcmd++ = 0;
    if (shflg == 1)
        em_puts(curcmd);
    /* save command */
    strcpy(savcmd,curcmd);

    if ((pid = fork()) == 0) {
        sigset(SIGHUP, oldhup);
        sigset(SIGQUIT, oldquit);
        execlp("/bin/sh", "sh", "-c", curcmd, (char *) 0);
        exit(0100);
    }
    savint = sigset(SIGINT, SIG_IGN);
    while ((rpid = wait(&retcode)) != pid && rpid != -1);
    sigset(SIGINT, savint);
    if (vflag) em_puts("!");
}

quit()
{
    if (vflag && fchange) {
        fchange = 0;
        if(flag28){flag28 = 0; error(62);} /* For case where user reads
                    in BOTH a good file & a bad file */
        error(1);
    }
    unlink(tfname);
    exit(errcnt? 2: 0);
}

delete()
{
    setdot();
    newline();
    nonzero();
    if (!globflg) save();
    rdelete(addr1, addr2);
}

rdelete(ad1, ad2)
LINE ad1, ad2;
{
    register LINE a1, a2, a3;

    a1 = ad1;
    a2 = ad2+1;
    a3 = dol;
    dol -= a2 - a1;
    do
        a1++->cur = a2++->cur;
    while (a2 <= a3);
    a1 = ad1;
    if (a1 > dol)
        a1 = dol;
    dot = a1;
    fchange = 1;
}

gdelete()
{
    register LINE a1, a2, a3;

    a3 = dol;
    for (a1=zero+1; (a1->cur&01)==0; a1++)
        if (a1>=a3)
            return 0;
    for (a2=a1+1; a2<=a3;) {
        if (a2->cur&01) {
            a2++;
            dot = a1;
        } else
            a1++->cur = a2++->cur;
    }
    dol = a1-1;
    if (dot>dol)
        dot = dol;
    fchange = 1;
}

char *em_getline(tl)
{
    register char *bp, *lp;
    register nl;

    lp = linebuf;
    bp = getblock(tl, READ);
    nl = nleft;
    tl &= ~0377;
    while (*lp++ = *bp++)
        if (--nl == 0) {
            bp = getblock(tl+=0400, READ);
            nl = nleft;
        }
    return(linebuf);
}

putline()
{
    register char *bp, *lp;
    register nl;
    int tl;

    fchange = 1;
    lp = linebuf;
    tl = tline;
    bp = getblock(tl, WRITE);
    nl = nleft;
    tl &= ~0377;
    while (*bp = *lp++) {
        if (*bp++ == '\n') {
            *--bp = 0;
            linebp = lp;
            break;
        }
        if (--nl == 0) {
            bp = getblock(tl+=0400, WRITE);
            nl = nleft;
        }
    }
    nl = tline;
    tline += (((lp-linebuf)+03)>>1)&077776;
    return(nl);
}

char *getblock(atl, iof)
{
    register bno, off;
    register char *p1, *p2;
    register int n;
    
    bno = (atl>>8)&0377;
    off = (atl<<1)&0774;
    if (bno >= 255) {
        lastc = '\n';
        error(31);
    }
    nleft = 512 - off;
    if (bno==iblock) {
        ichanged |= iof;
        return(ibuff+off);
    }
    if (bno==oblock)
        return(obuff+off);
    if (iof==READ) {
        if (ichanged) {
            blkio(iblock, ibuff, write);
        }
        ichanged = 0;
        iblock = bno;
        blkio(bno, ibuff, read);
        return(ibuff+off);
    }
    if (oblock>=0) {
        blkio(oblock, obuff, write);
    }
    oblock = bno;
    return(obuff+off);
}

blkio(b, buf, iofcn)
char *buf;
int (*iofcn)();
{
    lseek(tfile, (long)b<<9, 0);
    if ((*iofcn)(tfile, buf, 512) != 512) {
        if(dol != zero)error(32); /* Bypass this if writing null file */
    }
}

init()
{
    register *markp;
    int omask;

    close(tfile);
    tline = 2;
    for (markp = names; markp < &names[26]; )
        *markp++ = 0;
    subnewa = 0;
    anymarks = 0;
    iblock = -1;
    oblock = -1;
    ichanged = 0;
    initflg = 1;
    tfname = TEMPORARY_INIT_FILE;
    omask = umask(0);
    close(creat(tfname, 0600));
    umask(omask);
    tfile = open(tfname, 2);
    brk((char *)fendcore);
    dot = zero = dol = savdot = savdol = fendcore;
    flag28 = save28 = 0;
    endcore = fendcore - sizeof(struct lin);
}

screensplit()
{
    register a;
    
    a = LENGTH;
    while(a--) putchr(SPLIT);
    putchr('\n');
}

global(k)
{
    register char *gp;
    register c;
    register LINE a1;
    char globuf[GBSIZE];

    if (globp)
        error(33);
    setall();
    nonzero();
    if ((c=getchr())=='\n')
        error(19);
    save();
    compile((char *) 0, expbuf, &expbuf[ESIZE], c);
    gp = globuf;
    while ((c = getchr()) != '\n') {
        if (c==EOF)
            error(19);
        if (c=='\\') {
            c = getchr();
            if (c!='\n')
                *gp++ = '\\';
        }
        *gp++ = c;
        if (gp >= &globuf[GBSIZE-2])
            error(34);
    }
    if (gp == globuf)
        *gp++ = 'p';
    *gp++ = '\n';
    *gp++ = 0;
    for (a1=zero; a1<=dol; a1++) {
        a1->cur &= ~01;
        if (a1>=addr1 && a1<=addr2 && execute(0, a1)==k)
            a1->cur |= 01;
    }
    /*
     * Special case: g/.../d (avoid n^2 algorithm)
     */
    if (globuf[0]=='d' && globuf[1]=='\n' && globuf[2]=='\0') {
        gdelete();
        return 0;
    }
    for (a1=zero; a1<=dol; a1++) {
        if (a1->cur & 01) {
            a1->cur &= ~01;
            dot = a1;
            globp = globuf;
            globflg = 1;
            commands();
            globflg = 0;
            a1 = zero;
        }
    }
}

join()
{
    register char *gp, *lp;
    register LINE a1;

    if (addr1 == addr2) return 0;
    gp = genbuf;
    for (a1=addr1; a1<=addr2; a1++) {
        lp = em_getline(a1->cur);
        while (*gp = *lp++)
            if (gp++ >= &genbuf[LBSIZE-2])
                error(27);
    }
    lp = linebuf;
    gp = genbuf;
    while (*lp++ = *gp++);
    addr1->cur = putline();
    if (addr1<addr2)
        rdelete(addr1+1, addr2);
    dot = addr1;
}

substitute(inglob)
{
    register gsubf, nl;
    register LINE a1;
    int *markp;
    int getsub();

    gsubf = compsub();
    for (a1 = addr1; a1 <= addr2; a1++) {
        if (execute(0, a1)==0)
            continue;
        inglob |= 01;
        dosub();
        if (gsubf) {
            while (*loc2) {
                if (execute(1, (LINE )0)==0)
                    break;
                dosub();
            }
        }
        subnewa = putline();
        a1->cur &= ~01;
        if (anymarks) {
            for (markp = names; markp < &names[26]; markp++)
                if (*markp == a1->cur)
                    *markp = subnewa;
        }
        a1->cur = subnewa;
        append(getsub, a1);
        nl = nline;
        a1 += nl;
        addr2 += nl;
    }
    if (inglob==0)
        error(35);
}

compsub()
{
    register seof, c;
    register char *p;
    static char remem[LBSIZE]={-1};

    if ((seof = getchr()) == '\n' || seof == ' ')
        error(36);
    compile((char *) 0, expbuf, &expbuf[ESIZE], seof);
    p = rhsbuf;
    for (;;) {
        c = getchr();
        if (c=='\\')
            c = getchr() | 0200;
        if (c=='\n') {
            if (nodelim == 1) {
                nodelim = 0;
                error(36);
            }
            if (globp && globp[0])
                c |= 0200;  /* insert '\' */
            else {
                UNGETC(c);
                pflag++;
                break;
            }
        }
        if (c==seof)
            break;
        *p++ = c;
        if (p >= &rhsbuf[LBSIZE])
            error(38);
    }
    *p++ = 0;
    if(rhsbuf[0] == '%' && rhsbuf[1] == 0)
        (remem[0]!=-1) ? strcpy(rhsbuf, remem) : error(55);
    else
        strcpy(remem, rhsbuf);
    if ((peekc = getchr()) == 'g') {
        peekc = 0;
        newline();
        return(1);
    }
    newline();
    return(0);
}

getsub()
{
    register char *p1, *p2;

    p1 = linebuf;
    if ((p2 = linebp) == 0)
        return(EOF);
    while (*p1++ = *p2++);
    linebp = 0;
    return(0);
}

dosub()
{
    register char *lp, *sp, *rp;
    int c;

    lp = linebuf;
    sp = genbuf;
    rp = rhsbuf;
    while (lp < loc1)
        *sp++ = *lp++;
    while (c = *rp++&0377) {
        if (c=='&') {
            sp = place(sp, loc1, loc2);
            continue;
        } else if(c & 0200) {
            c &= 0177;
            if(c >= '1' && c < nbra + '1') {
                sp = place(sp, braslist[c-'1'], braelist[c-'1']);
                continue;
            }
        }
        *sp++ = c;
        if (sp >= &genbuf[LBSIZE])
            error(27);
    }
    lp = loc2;
    loc2 = sp - genbuf + linebuf;
    while (*sp++ = *lp++)
        if (sp >= &genbuf[LBSIZE])
            error(27);
    lp = linebuf;
    sp = genbuf;
    while (*lp++ = *sp++);
}

char *place(sp, l1, l2)
register char *sp, *l1, *l2;
{

    while (l1 < l2) {
        *sp++ = *l1++;
        if (sp >= &genbuf[LBSIZE])
            error(27);
    }
    return(sp);
}

move(cflag)
{
    register LINE adt, ad1, ad2;
    int getcopy();

    setdot();
    nonzero();
    if ((adt = address())==0)
        error(39);
    newline();
    if (!globflg) save();
    if (cflag) {
        ad1 = dol;
        append(getcopy, ad1++);
        ad2 = dol;
    } else {
        ad2 = addr2;
        for (ad1 = addr1; ad1 <= ad2;)
            ad1++->cur &= ~01;
        ad1 = addr1;
    }
    ad2++;
    if (adt<ad1) {
        dot = adt + (ad2-ad1);
        if ((++adt)==ad1)
            return 0;
        reverse(adt, ad1);
        reverse(ad1, ad2);
        reverse(adt, ad2);
    } else if (adt >= ad2) {
        dot = adt++;
        reverse(ad1, ad2);
        reverse(ad2, adt);
        reverse(ad1, adt);
    } else
        error(39);
    fchange = 1;
}

reverse(a1, a2)
register LINE a1, a2;
{
    register int t;

    for (;;) {
        t = (--a2)->cur;
        if (a2 <= a1)
            return 0;
        a2->cur = a1->cur;
        a1++->cur = t;
    }
}

getcopy()
{

    if (addr1 > addr2)
        return(EOF);
    em_getline(addr1++->cur);
    return(0);
}


error1(code)
{
    expbuf[0] = 0;
    nbra = 0;
    error(code);
}

execute(gf, addr)
LINE addr;
{
    register char *p1, *p2, c;

    for (c=0; c<NBRA; c++) {
        braslist[c] = 0;
        braelist[c] = 0;
    }
    if (gf) {
        if (circf)
            return(0);
        p1 = linebuf;
        p2 = genbuf;
        while (*p1++ = *p2++);
        locs = p1 = loc2;
    } else {
        if (addr==zero)
            return(0);
        p1 = em_getline(addr->cur);
        locs = 0;
    }
    return(step(p1, expbuf));
}


putd()
{
    register r;

    r = (int)(count%10);
    count /= 10;
    if (count)
        putd();
    putchr(r + '0');
}

em_puts(register char *sp)
{
    int sz,i;
    if (fss.Ffill && (listf == 0)) {
        if ((i = expnd(sp,funny,&sz,&fss)) == -1) {
            write(1,funny,fss.Flim & 0377); putchr('\n');
            write(1,"too long",8);
        }
        else
            write(1,funny,sz);
        putchr('\n');
        if (i == -2) write(1,"tab count\n",10);
        return(0);
    }
    col = 0;
    while (*sp)
        putchr(*sp++);
    putchr('\n');
}

char    line[70];
char    *linp = line;

putchr(ac)
{
    register char *lp;
    register c;
    short len;

    lp = linp;
    c = ac;
    if ( listf ) {
        col++;
        if (col >= 72) {
            col = 0;
            *lp++ = '\\';
            *lp++ = '\n';
        }
        if (c=='\t') {
            c = '>';
            goto esc;
        }
        if (c=='\b') {
            c = '<';
        esc:
            *lp++ = '-';
            *lp++ = '\b';
            *lp++ = c;
            goto out;
        }
        if (c<' ' && c!= '\n') {
            *lp++ = '\\';
            *lp++ = (c>>3)+'0';
            *lp++ = (c&07)+'0';
            col += 2;
            goto out;
        }
    }
    *lp++ = c;
out:
    if(c == '\n' || lp >= &line[64]) {
        linp = line;
        len = lp - line;
        if(yflag & 01)
            write(1, &len, sizeof(len));
        write(1, line, len);
        return 0;
    }
    linp = lp;
}

globaln(k)
{
    register char *gp;
    register c;
    register LINE a1;
    int  nfirst;
    char globuf[GBSIZE];

    if (yflag)
        error(59);
    if (globp)
        error(33);
    setall();
    nonzero();
    if ((c=getchr())=='\n')
        error(19);
    save();
    compile((char *) 0, expbuf, &expbuf[ESIZE], c);
    for (a1=zero; a1<=dol; a1++) {
        a1->cur &= ~01;
        if (a1>=addr1 && a1<=addr2 && execute(0, a1)==k)
            a1->cur |= 01;
    }
    nfirst = 0;
    newline();
    for (a1=zero; a1<=dol; a1++) {
        if (a1->cur & 01) {
            a1->cur &= ~01;
            dot = a1;
            em_puts(em_getline(a1->cur));
            if ((c=getchr()) == EOF)
                error(52);
            if(c=='a' || c=='i' || c=='c')
                error(53);
            c &= 0177;
            if (c == '\n') {
                a1 = zero;
                continue;
            }
            if (c != '&') {
                gp = globuf;
                *gp++ = c;
                while ((c = getchr()) != '\n') {
                    if (c=='\\') {
                        c = getchr();
                        if (c!='\n')
                            *gp++ = '\\';
                    }
                    *gp++ = c;
                    if (gp >= &globuf[GBSIZE-2])
                        error(34);
                }
                *gp++ = '\n';
                *gp++ = 0;
                nfirst = 1;
            }
            else
                if ((c=getchr()) != '\n')
                    error(54);
            globp = globuf;
            if (nfirst) {
                globflg = 1;
                commands();
                globflg = 0;
            }
            else error(56);
            globp = 0;
            a1 = zero;
        }
    }
}
eopen(string, rw)
char *string;
{
#define w_or_r(a,b) (rw?a:b)
    int pf[2];
    int i;
    int io;
    int chcount;    /* # of char read. */
    int crflag;
    char *fp;

    crflag = 0;  /* Is file encrypted flag; 1=yes. */
    if (rflg) { /* restricted shell */
        if (Xqt) {
            Xqt = 0;
            error(6);
        }
    }
    if(!Xqt) {
        if((io=open(string, rw)) >= 0) {
            if (fflg) {
                chcount = read(io,funny,LBSIZE);
/* Verify that line just read IS an encrypted file. */
            fp = funny; /* Set fp to start of buffer. */
            while(fp < &funny[chcount])
                if(*fp++ & 0200)crflag = 1;
#ifdef FSPEC
                if (fspec(funny,&fss,0) < 0) {
                    fss.Ffill = 0;
                    fflg = 0;
                    error(4);
                }
#endif
                lseek(io,0L,0);
            }
        }
        fflg = 0;
        return(io);
    }
    if(pipe(pf) < 0)
xerr:       error(0);
    if((i = fork()) == 0) {
        sigset(SIGHUP, oldhup);
        sigset(SIGQUIT, oldquit);
        sigset(SIGPIPE, oldpipe);
        sigset(SIGINT, (int (*)()) 0);
        close(w_or_r(pf[1], pf[0]));
        close(w_or_r(0, 1));
        dup(w_or_r(pf[0], pf[1]));
        close(w_or_r(pf[0], pf[1]));
        execlp("/bin/sh", "sh", "-c", string, (char *) 0);
        exit(1);
    }
    if(i == -1)
        goto xerr;
    close(w_or_r(pf[0], pf[1]));
    return w_or_r(pf[1], pf[0]);
}

eclose(f)
{
    close(f);
    if(Xqt)
        Xqt = 0, wait((int *) 0);
}
mkfunny()
{
    register char *p, *p1, *p2;

    p2 = p1 = funny;
    p = efile;
    while(*p)
        p++;
    while(*--p  == '/') /* delete trailing slashes */
        *p = '\0';
    p = efile;
    while (*p1++ = *p)
        if (*p++ == '/') p2 = p1;
    p1 = &tfname[6];
    *p2 = '\007';   /* add unprintable char to make funny a unique name */
    while (p1 <= &tfname[11])
        *++p2 = *p1++;
}

getime() /* get modified time of file and save */
{
    if (stat(efile,&Fl) < 0)
        savtime = 0;
    else
        savtime = Fl.st_mtime;
}

chktime() /* check saved mod time against current mod time */
{
    if (savtime != 0 && Fl.st_mtime != 0) {
        if (savtime != Fl.st_mtime)
            error(58);
    }
}

newtime() /* get new mod time and save */
{
    stat(efile,&Fl);
    savtime = Fl.st_mtime;
}

red(op) /* restricted - check for '/' in name */
        /* and delete trailing '/' */
char *op;
{
    register char *p;

    p = op;
    while(*p)
        if(*p++ == '/'&& rflg) {
            *op = 0;
            error(6);
        }
    /* delete trailing '/' */
    while(p > op) {
        if (*--p == '/')
            *p = '\0';
        else break;
    }
}

char *fsp, fsprtn;

#ifdef FSPEC
fspec(line,f,up)
char line[];
struct Fspec *f;
int up;
{
    struct sgttyb arg;
    register int havespec, n;

    if(!up) clear(f);

    havespec = fsprtn = 0;
    for(fsp=line; *fsp && *fsp != '\n'; fsp++)
        switch(*fsp) {

            case '<':       if(havespec) return(-1);
                    if(*(fsp+1) == ':') {
                        havespec = 1;
                        clear(f);
                        if(!ioctl(1, TIOCGETP, &arg) &&
                          ((arg.sg_flags&XTABS) == XTABS))
                            f->Ffill = 1;
                        fsp++;
                        continue;
                    }

            case ' ':       continue;

            case 's':       if(havespec && (n=numb()) >= 0)
                        f->Flim = n;
                    continue;

            case 't':       if(havespec) targ(f);
                    continue;

            case 'd':       continue;

            case 'm':       if(havespec)  n = numb();
                    continue;

            case 'e':       continue;
            case ':':       if(!havespec) continue;
                    if(*(fsp+1) != '>') fsprtn = -1;
                    return(fsprtn);

            default:    if(!havespec) continue;
                    return(-1);
        }
    return(1);
}

numb()
{
    register int n;

    n = 0;
    while(*++fsp >= '0' && *fsp <= '9')
        n = 10*n + *fsp-'0';
    fsp--;
    return(n);
}

targ(f)
struct Fspec *f;
{
    if(*++fsp == '-') {
        if(*(fsp+1) >= '0' && *(fsp+1) <= '9') tincr(numb(),f);
        else tstd(f);
        return 0;
    }
    if(*fsp >= '0' && *fsp <= '9') {
        tlist(f);
        return 0;
    }
    fsprtn = -1;
    fsp--;
    return 0;
}


tincr(n,f)
int n;
struct Fspec *f;
{
    register int l, i;

    l = 1;
    for(i=0; i<20; i++)
        f->Ftabs[i] = l += n;
    f->Ftabs[i] = 0;
}


tstd(f)
struct Fspec *f;
{
    char std[3];

    std[0] = *++fsp;
    if (*(fsp+1) >= '0' && *(fsp+1) <= '9')  {
        std[1] = *++fsp;
        std[2] = '\0';
    }
    else std[1] = '\0';
    fsprtn = stdtab(std,f->Ftabs);
    return 0;
}

tlist(f)
struct Fspec *f;
{
    register int n, last, i;

    fsp--;
    last = i = 0;

    do {
        if((n=numb()) <= last || i >= 20) {
            fsprtn = -1;
            return 0;
        }
        f->Ftabs[i++] = last = n;
    } while(*++fsp == ',');

    f->Ftabs[i] = 0;
    fsp--;
}
#endif

expnd(line,buf,sz,f)
char line[], buf[];
int *sz;
struct Fspec *f;
{
    register char *l, *t;
    register int b;

    l = line - 1;
    b = 1;
    t = f->Ftabs;
    fsprtn = 0;

    while(*++l && *l != '\n' && b < 511) {
        if(*l == '\t') {
            while(*t && b >= *t) t++;
            if (*t == 0) fsprtn = -2;
            do buf[b-1] = ' '; while(++b < *t);
        }
        else buf[b++ - 1] = *l;
    }

    buf[b] = '\0';
    *sz = b;
    if(*l != '\0' && *l != '\n') {
        buf[b-1] = '\n';
        return(-1);
    }
    buf[b-1] = *l;
    if(f->Flim && b-1 > f->Flim) return(-1);
    return(fsprtn);
}


clear(f)
struct Fspec *f;
{
    f->Ftabs[0] = f->Fdel = f->Fmov = f->Ffill = 0;
    f->Flim = 0;
}
lenchk(line,f)
char line[];
struct Fspec *f;
{
    register char *l, *t;
    register int b;

    l = line - 1;
    b = 1;
    t = f->Ftabs;

    while(*++l && *l != '\n' && b < 511) {
        if(*l == '\t') {
            while(*t && b >= *t) t++;
            while(++b < *t);
        }
        else b++;
    }

    if((*l!='\0'&&*l!='\n') || (f->Flim&&b-1>f->Flim))
        return(-1);
    return(0);
}

#ifdef FSPEC
#define NTABS 21

/*      stdtabs: standard tabs table
    format: option code letter(s), null, tabs, null */
char stdtabs[] = {
'a',    0,1,10,16,36,72,0,              /* IBM 370 Assembler */
'a','2',0,1,10,16,40,72,0,              /* IBM Assembler alternative*/
'c',    0,1,8,12,16,20,55,0,            /* COBOL, normal */
'c','2',0,1,6,10,14,49,0,               /* COBOL, crunched*/
'c','3',0,1,6,10,14,18,22,26,30,34,38,42,46,50,54,58,62,67,0,
'f',    0,1,7,11,15,19,23,0,            /* FORTRAN */
'p',    0,1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61,0, /* PL/I */
's',    0,1,10,55,0,                /* SNOBOL */
'u',    0,1,12,20,44,0,             /* UNIVAC ASM */
0};
/*      stdtab: return tab list for any "canned" tab option.
    entry: option points to null-terminated option string
        tabvect points to vector to be filled in
    exit: return(0) if legal, tabvect filled, ending with zero
        return(-1) if unknown option
*/

stdtab(option,tabvect)
char option[], tabvect[NTABS];
{
    char *scan;
    tabvect[0] = 0;
    scan = stdtabs;
    while (*scan) {
        if (strequal(&scan,option))
            {strcopy(scan,tabvect);break;}
        else while(*scan++);    /* skip over tab specs */
    }
/*      later: look up code in /etc/something */
    return(tabvect[0]?0:-1);
}
#endif

/*      strequal: checks strings for equality
    entry: scan1 points to scan pointer, str points to string
    exit: return(1) if equal, return(0) if not
        *scan1 is advanced to next nonzero byte after null
*/
strequal(scan1,str)
char **scan1, *str;
{
    char c, *scan;
    scan = *scan1;
    while ((c = *scan++) == *str && c) str++;
    *scan1 = scan;
    if (c == 0 && *str == 0) return(1);
    if (c) while(*scan++);
    *scan1 = scan;
    return(0);
}

/*      strcopy: copy source to destination */

strcopy(source,dest)
char *source, *dest;
{
    while (*dest++ = *source++);
    return 0;
}

/* This is called before a buffer modifying command so that the */
/* current array of line ptrs is saved in sav and dot and dol are saved */
save() {
    LINE i;

    savdot = dot;
    savdol = dol;
    for (i=zero+1; i<=dol; i++)
        i->sav = i->cur;
    initflg = 0;
}

/* The undo command calls this to restore the previous ptr array sav */
/* and swap with cur - dot and dol are swapped also. This allows user to */
/* undo an undo */
undo() {
    int tmp;
    LINE i, tmpdot, tmpdol;

    tmpdot = dot; dot = savdot; savdot = tmpdot;
    tmpdol = dol; dol = savdol; savdol = tmpdol;
    /* swap arrays using the greater of dol or savdol as upper limit */
    for (i=zero+1; i<=((dol>savdol) ? dol : savdol); i++) {
        tmp = i->cur;
        i->cur = i->sav;
        i->sav = tmp;
    }
}
