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
//#include <stdlib.h>
#include <string.h>

#include <msgapi.h>
#include <fidoconfig.h>
#include <common.h>

#include <linkarea.h>
#include <packarea.h>
#include <purgearea.h>
#include <sortarea.h>

#define VER_MAJOR 1
#define VER_MINOR 1

char keepImportLog = 0;

void processCommandLine(int argc, char *argv[], int *what)
{
   int i = 0;

   if (argc == 1) {
      printf("\nUsage:\n\n");
      printf("      hptUtil purge - purge areas\n");
      printf("      hptUtil pack  - pack areas\n");
      printf("      hptUtil link  - reply-link messages\n");
      printf("      hptUtil sort  - sort unread messages by time and date\n");
   } /* endif */

   while (i < argc-1) {
      i++;
      if (stricmp(argv[i], "purge") == 0) *what |= 1;
      if (stricmp(argv[i], "pack") == 0) *what |= 2;
      if (stricmp(argv[i], "sort") == 0) *what |= 4;
      if (stricmp(argv[i], "link") == 0) *what |= 8;
      if (argv[i][0] == '-' && argv[i][1] && (argv[i][1] == 'k' || argv[i][1] == 'K')) {
         keepImportLog = 1;
      } /* endif */
   } /* endwhile */
}

int main(int argc, char *argv[])
{
   s_fidoconfig *config;
   int what = 0;
   int ret = 0;

#ifdef __linux__
   printf("hptUtil v%u.%02u/LNX\n", VER_MAJOR, VER_MINOR);
#elif __freebsd__
   printf("hptUtil v%u.%02u/BSD\n", VER_MAJOR, VER_MINOR);
#elif __OS2__
   printf("hptUtil v%u.%02u/OS2\n", VER_MAJOR, VER_MINOR);
#elif __NT__
   printf("hptUtil v%u.%02u/NT\n", VER_MAJOR, VER_MINOR);
#elif __sun__
   printf("hptUtil v%u.%02u/SUN\n", VER_MAJOR, VER_MINOR);
#else
   printf("hptUtil v%u.%02u\n", VER_MAJOR, VER_MINOR);
#endif

   processCommandLine(argc, argv, &what);

   if (what) {
      config = readConfig();
      if (config) {
         if (what & 1) purgeAreas(config);
         if (what & 2) packAreas(config);
         if (what & 4) sortAreas(config);
         if (what & 8) linkAreas(config);
         disposeConfig(config);

      } else {
         printf("Could not read fido config\n");
         ret = 1;
      } /* endif */
   } else {
   } /* endif */
   
   return ret;
}

