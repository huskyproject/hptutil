/*
 *      hptUtil - purge, pack, link and sort utility for HPT
 *      by Fedor Lizunkov 2:5020/960@Fidonet
 *
 * This file is part of HPT.
 *
 * HPT is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * HPT is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPT; see the file COPYING.  If not, write to the Free
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
#include <fidoconf/xstr.h>

#include <linkarea.h>
#include <packarea.h>
#include <purgearea.h>
#include <sortarea.h>

#define VER_MAJOR 1
#define VER_MINOR 2

FILE *filesout;
FILE *fileserr;

char quiet = 0;
char keepImportLog = 0;
char *altImportLog = NULL;
unsigned int debugLevel = 0;

void OutScreen(char *str, ...)
{
   va_list par;
   
   va_start (par, str);
   if (quiet == 0) vfprintf(filesout, str, par);
   va_end (par);
}

void printVer()
{
#if defined(__linux__)
   OutScreen("\nhptutil v%u.%02u/lnx\n\n", VER_MAJOR, VER_MINOR);
#elif defined(__freebsd__)
   OutScreen("\nhptutil v%u.%02u/bsd\n\n", VER_MAJOR, VER_MINOR);
#elif defined(__OS2__)
   OutScreen("\nhptutil v%u.%02u/os2\n\n", VER_MAJOR, VER_MINOR);
#elif defined(__NT__)
   OutScreen("\nhptutil v%u.%02u/w32\n\n", VER_MAJOR, VER_MINOR);
#elif defined(__sun__)
   OutScreen("\nhptutil v%u.%02u/sun\n\n", VER_MAJOR, VER_MINOR);
#else
   OutScreen("\nhptutil v%u.%02u\n\n", VER_MAJOR, VER_MINOR);
#endif

}

void processCommandLine(int argc, char *argv[], int *what)
{
   int i = 0;
   char *tmp = NULL;

   if (argc == 1) {
      printVer();
      fprintf(filesout, "Usage:\n\n");
      OutScreen("      hptutil sort  - sort unread messages by time and date\n");
      OutScreen("      hptutil link  - reply-link messages\n");
      OutScreen("      hptutil purge - purge areas\n");
      OutScreen("      hptutil pack  - pack areas\n");
      OutScreen("      hptutil -k    - keep import.log file\n");
      OutScreen("      hptutil -q    - quiet mode (no screen output)\n");
      OutScreen("      hptutil -i <filename> - alternative import.log\n\n");
      exit(1);
   } /* endif */

   while (i < argc-1) {
      i++;
      if (stricmp(argv[i], "purge") == 0) *what |= 1;
      else if (stricmp(argv[i], "pack") == 0) *what |= 2;
      else if (stricmp(argv[i], "sort") == 0) *what |= 4;
      else if (stricmp(argv[i], "link") == 0) *what |= 8;
      else if (stricmp(argv[i], "-k") == 0) keepImportLog = 1;
      else if (stricmp(argv[i], "-q") == 0) quiet = 1;
      else if (stricmp(argv[i], "-i") == 0) {
	  if (i < argc-1) {
              i++;
	      xstrcat(&altImportLog, argv[i]);
	  } else {
	      fprintf(fileserr, "Not found option for \'-i\' key!\n\n");
	      exit (5);
	  }
      }
      else if (argv[i][0] == '-' && (argv[i][1] == 'i' || argv[i][1] == 'I')) {
          xstrcat(&altImportLog, argv[i]+2);
      }
      else if (stricmp(argv[i], "-d") == 0) {
          if (i < argc-1) {
              i++;
	      xstrcat(&tmp, argv[i]);
	      debugLevel = (unsigned int)(atoi(tmp));
	      free(tmp);
          } else {
	      fprintf(fileserr, "Not found option for \'-d\' key!\n\n");
	      exit (5);
	  }
      }
      else if (argv[i][0] == '-' && (argv[i][1] == 'd' || argv[i][1] == 'D')) {
          xstrcat(&tmp, argv[i]+2);
	  debugLevel = (unsigned int)(atoi(tmp));
	  free(tmp);
      } else {
          fprintf(fileserr, "Don't known \'%s\' option\n\n", argv[i]);
      }
   } /* endwhile */
}

int main(int argc, char *argv[])
{
   s_fidoconfig *config;
   char *keepOrigImportLog;
   int what = 0;
   int ret = 0;

   if (quiet) filesout=NULL;
   else filesout=stdout;
   fileserr=stderr;
   
   setbuf(filesout, NULL);
   setbuf(fileserr, NULL);

   

   processCommandLine(argc, argv, &what);
   
   printVer();

   if (what) {
      setvar("module", "hptutil");
      config = readConfig(NULL);
      if (config) {
         if (altImportLog) {
            keepOrigImportLog = config->importlog;
	    config->importlog = altImportLog;
	 }
	 
         if (what & 4) sortAreas(config);
         if (what & 8) linkAreas(config);
         if (what & 1) purgeAreas(config);
         if (what & 2) packAreas(config);
	 
	 if (altImportLog) {
	    config->importlog = keepOrigImportLog;
	    free(altImportLog);
	 }
	 
         disposeConfig(config);
      } else {
         fprintf(fileserr, "Could not read fido config\n");
         ret = 1;
      } /* endif */
   } else {
      if (argc > 1) OutScreen("Nothing to do ...\n\n");
   } /* endif */
   
   return ret;
}

