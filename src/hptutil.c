/* $Id$ */
/*
 *      hptUtil - purge, pack, link and sort utility for HPT
 *      by Fedor Lizunkov 2:5020/960@Fidonet and val khokhlov 2:550/180@fidonet
 *
 * This file is part of HPT.
 *
 * HPT is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * HPT is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPT; see the file COPYING.  If not, write to the free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <smapi/msgapi.h>
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>
#include <huskylib/xstr.h>

#include <hptutil.h>
#include <linkarea.h>
#include <sortarea.h>
#include <purgearea.h>
#include <packarea.h>
#include <fixarea.h>
#include <undelete.h>

#include "version.h"

char * versionStr;
FILE * filesout;
FILE * fileserr;
#define LogFileName "hptutil.log"
FILE * hptutil_log = NULL;
char quiet         = 0;
char jam_by_crc    = 0;
char keepImportLog = 0;
char typebase;
char * basefilename;
char * altImportLog     = NULL;
unsigned int debugLevel = 0;

void OutScreen(char * str, ...)
{
    char buf[256], dt[20];
    char * pos;
    int cnt = 0, cnt2;
    static int cont = 0;
    va_list par;

    va_start(par, str);

    if(hptutil_log != NULL)
    {
        time_t t       = time(NULL);
        struct tm * tm = localtime(&t);
        cnt = vsprintf(buf, str, par);

        if(cnt >= sizeof(buf))
        {
            exit(1);
        }

        cnt2 = sprintf(dt,
                       "%2d.%02d.%02d %02d:%02d:%02d ",
                       tm->tm_mday,
                       tm->tm_mon + 1,
                       tm->tm_year % 100,
                       tm->tm_hour,
                       tm->tm_min,
                       tm->tm_sec);

        if(cnt2 >= sizeof(dt))
        {
            exit(1);
        }

        buf[cnt] = dt[cnt2] = 0;
    }

    if(quiet == 0)
    {
        vfprintf(filesout, str, par);
    }

    va_end(par);

    if(hptutil_log != NULL)
    {
        char * cur = buf;

        do
        {
            if((pos = strchr(cur, '\n')) != NULL)
            {
                *pos = 0;
            }

            if(!cont && *cur)
            {
                fprintf(hptutil_log, "%s", dt);
            }

            if(pos == NULL)
            {
                fprintf(hptutil_log, "%s", cur);
            }
            else
            {
                fprintf(hptutil_log, "%s\n", cur);
            }

            cont = 0;
            cur  = (pos != NULL) ? pos + 1 : buf + cnt;
        }
        while(*cur);
        cont = (pos == NULL);
        fflush(hptutil_log);
    }
} /* OutScreen */

/*
   void OutScreen(char *str, ...)
   {
   va_list par;

   va_start (par, str);
   if (quiet == 0) vfprintf(filesout, str, par);
   va_end (par);
   }
*/

void wait_d()
{
    char form[]  = "-\\|/";
    static int i = 0;

    if(i == 4)
    {
        i = 0;
    }

    OutScreen("\b%c", form[i++]);
}

void printFixHelp()
{
    fprintf(filesout,
            "Usage: hptutil fix [options]\n"
            "Options:  -s <%cpath%cfilename>\t- fix squish area file\n"
            "\t  -j <%cpath%cfilename>\t- fix jam area file\n\n",
            PATH_DELIM,
            PATH_DELIM,
            PATH_DELIM,
            PATH_DELIM);
    exit(0);
}

char FixBase(char c)
{
    switch(c)
    {
        case 's':
            return (char)1;

            break;

        case 'S':
            return (char)1;

        case 'j':
            return (char)2;

        case 'J':
            return (char)2;

        default:
            fprintf(fileserr, "Don't known type base. See ...\n");
            printFixHelp();
            break;
    }
    return 0;
}

void parseFixLine(int argc, char * argv[], int * i, int * what)
{
    if(*i < argc - 1)
    {
        (*i)++;

        if(stricmp(argv[*i], "-?") == 0)
        {
            printFixHelp();
        }

        if(argv[*i][0] != '-')
        {
            fprintf(fileserr, "Error options. See ...\n");
            printFixHelp();
        }
        else if(stricmp(argv[*i], "-s") == 0 || stricmp(argv[*i], "-j") == 0)
        {
            /* typebase   */
            /* 0 - error  */
            /* 1 - Squish */
            /* 2 - Jam    */
            typebase = FixBase(argv[*i][1]);

            if(*i < argc - 1)
            {
                (*i)++;
                xstrcat(&basefilename, argv[*i]);
                *what |= 0x10;
                return;
            }
            else
            {
                fprintf(fileserr, "Error options. See ...\n");
                printFixHelp();
            }
        }
        else
        {
            typebase = FixBase(argv[*i][1]);
            xstrcat(&basefilename, argv[*i] + 2);
        }
    }
    else
    {
        fprintf(fileserr, "Error options. See ...\n");
        printFixHelp();
    }
} /* parseFixLine */

static void printMyTitle()
{
    static int already = 0;

    if(!already)
    {
        if(filesout)
        {
            fprintf(filesout, "%s\n\n", versionStr);
        }

        already = 1;
    }
}

void processCommandLine(int argc, char * argv[], int * what)
{
    int i      = 0;
    char * tmp = NULL;

    if(argc == 1)
    {
        printMyTitle();
        printf(
            "Usage: hptutil [options]\n"
            "Options:  sort\t\t- sort unread messages by time and date\n"
            "\t  link\t\t- reply-link messages\n"
            "\t  purge\t\t- purge areas\n"
            "\t  pack\t\t- pack areas\n"
            "\t  fix\t\t- fix base (hptutil fix -? for more help)\n"
            "\t  -j\t\t- link Jam areas by CRC (great speed-up)\n"
            "\t  -k\t\t- keep import.log file\n"
            "\t  -q\t\t- quiet mode (no screen output)\n"
            "\t  -i <filename>\t- alternative import.log\n\n");
        exit(1);
    } /* endif */

    while(i < argc - 1)
    {
        i++;

        if(stricmp(argv[i], "purge") == 0)
        {
            *what |= 0x01;
        }
        else if(stricmp(argv[i], "pack") == 0)
        {
            *what |= 0x02;
        }
        else if(stricmp(argv[i], "sort") == 0)
        {
            *what |= 0x04;
        }
        else if(stricmp(argv[i], "link") == 0)
        {
            *what |= 0x08;
        }
        else if(stricmp(argv[i], "fix") == 0)
        {
            parseFixLine(argc, argv, &i, what);
        }
        else if(stricmp(argv[i], "undel") == 0)
        {
            *what |= 0x20;
        }
        else if(stricmp(argv[i], "-k") == 0)
        {
            keepImportLog = 1;
        }
        else if(stricmp(argv[i], "-q") == 0)
        {
            quiet = 1;
        }
        else if(stricmp(argv[i], "-j") == 0)
        {
            jam_by_crc = 1;
        }
        else if(stricmp(argv[i], "-i") == 0)
        {
            if(i < argc - 1)
            {
                i++;
                xstrcat(&altImportLog, argv[i]);
            }
            else
            {
                printMyTitle();
                fprintf(fileserr, "Parameter is required for '-i'\n\n");
                exit(5);
            }
        }
        else if(argv[i][0] == '-' && (argv[i][1] == 'i' || argv[i][1] == 'I'))
        {
            xstrcat(&altImportLog, argv[i] + 2);
        }
        else if(stricmp(argv[i], "-d") == 0)
        {
            if(i < argc - 1)
            {
                i++;
                xstrcat(&tmp, argv[i]);
                debugLevel = (unsigned int)(atoi(tmp));
                nfree(tmp);
            }
            else
            {
                printMyTitle();
                fprintf(fileserr, "Parameter is required for '-d'\n\n");
                exit(5);
            }
        }
        else if(argv[i][0] == '-' && (argv[i][1] == 'd' || argv[i][1] == 'D'))
        {
            xstrcat(&tmp, argv[i] + 2);
            debugLevel = (unsigned int)(atoi(tmp));
            nfree(tmp);
        }
        else
        {
            printMyTitle();
            fprintf(fileserr, "Unknown option '%s'\n\n", argv[i]);
        }
    } /* endwhile */
} /* processCommandLine */

int main(int argc, char * argv[])
{
    s_fidoconfig * config;
    char * keepOrigImportLog = NULL;
    char * buff = NULL;
    int what    = 0;
    int ret     = 0;

    filesout = stdout;
    fileserr = stderr;
    setbuf(filesout, NULL);
    setbuf(fileserr, NULL);
    versionStr = GenVersionStr("hptutil", VER_MAJOR, VER_MINOR, VER_PATCH, VER_BRANCH, cvs_date);
    processCommandLine(argc, argv, &what);

    if(quiet)
    {
        filesout = NULL;
    }

    printMyTitle();

    if(what)
    {
        setvar("module", "hptutil");
        config = readConfig(NULL);

        if(config)
        {
            /* init log */
            if(config->logFileDir)
            {
                xstrscat(&buff, config->logFileDir, LogFileName, NULL);
                hptutil_log = fopen(buff, "a");
                nfree(buff);
            }

            if(altImportLog)
            {
                keepOrigImportLog = config->importlog;
                config->importlog = altImportLog;
            }

            if(what & 0x10)
            {
                ret = fixArea(config);
            }
            else if(what & 0x20)
            {
                ret = undeleteMsgs(config, altImportLog);
            }
            else
            {
                if(what & 0x04)
                {
                    sortAreas(config);
                }

                if(what & 0x08)
                {
                    linkAreas(config);
                }

                if(what & 0x01)
                {
                    purgeAreas(config);
                }

                if(what & 0x02)
                {
                    packAreas(config);
                }
            }

            if(altImportLog)
            {
                config->importlog = keepOrigImportLog;
                nfree(altImportLog);
            }

            disposeConfig(config);
        }
        else
        {
            fprintf(fileserr, "Could not read fido config\n");
            ret = 1;
        } /* endif */
    }
    else
    {
        if(argc > 1)
        {
            OutScreen("Nothing to do ...\n\n");
        }
    } /* endif */

    if(hptutil_log != NULL)
    {
        fclose(hptutil_log);
    }

    return ret;
} /* main */
