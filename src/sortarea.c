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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#if !defined(UNIX) && !defined(SASC)
#include <io.h>
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if !defined(UNIX) && !defined(SASC)
#include <share.h>
#endif
#if !(defined(_MSC_VER) && (_MSC_VER >= 1200))
#include <unistd.h>
#endif

#include <smapi/compiler.h>
#include <smapi/prog.h>
#include <smapi/msgapi.h>
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>

#include <jam.h>
#include <squish.h>
#include <sortarea.h>

extern FILE *outfile;
extern int Open_File(char *name, word mode);
extern int CheckMsg(JAMHDRptr Hdr);

/* --------------------------SQUISH sector ON------------------------------- */

SQSORTptr AddSQSORT(SQSORTptr sqsort, SQIDXptr sqidx, PXMSG xmsg)
{
   SQSORTptr newsqsort, psqsort;
   struct tm tmTime;

   newsqsort = (SQSORTptr)malloc(sizeof(SQSORT));

   memmove(&(newsqsort->sqidx), sqidx, SQIDX_SIZE);
   memmove(&(newsqsort->xmsg), xmsg, XMSG_SIZE);
   DosDate_to_TmDate((union stamp_combo*)&(xmsg->date_written), &tmTime);
   newsqsort->msgtime = mktime(&tmTime);
   newsqsort->prev = newsqsort->next = NULL;

   if (sqsort == NULL) {
      psqsort = newsqsort;
   } else {
      psqsort = sqsort;
      if (newsqsort->msgtime > psqsort->msgtime) {
         while (newsqsort->msgtime > psqsort->msgtime && psqsort->next) {
            psqsort = psqsort->next;
         } /* endwhile */
         if (newsqsort->msgtime > psqsort->msgtime) {
            psqsort->next = newsqsort;
            newsqsort->prev = psqsort;
            psqsort = sqsort;
         } else {
            newsqsort->prev = psqsort->prev;
            newsqsort->next = psqsort;
            psqsort->prev->next = newsqsort;
            psqsort->prev = newsqsort;
            psqsort = sqsort;
         } /* endif */
      } else {
         newsqsort->next = psqsort;
         psqsort->prev = newsqsort;
         psqsort = newsqsort;
      } /* endif */
   } /* endif */

   return psqsort;
}

void freesqsort(SQSORTptr sqsort)
{

   if (sqsort == NULL) {
      return;
   } /* endif */

   while (sqsort->next) {
      sqsort = sqsort->next;
      free(sqsort->prev);
   } /* endwhile */
   free(sqsort);
}

void SortSquishArea(SQSORTptr sqsort, int SqdHandle, int SqiHandle, SQBASEptr sqbase, long idxPos, UMSGID umsgid)
{
   SQSORTptr psqsort = sqsort;
   int i;

   if (sqsort == NULL) {
      return;
   } /* endif */

   lseek(SqiHandle, idxPos, SEEK_SET);

   while (psqsort) {
      psqsort->xmsg.replyto = 0;
      for (i = 0; i < MAX_REPLY; i++) {
         psqsort->xmsg.replies[i] = 0;
      } /* endfor */
      lseek(SqdHandle, psqsort->sqidx.ofs+SQHDR_SIZE, SEEK_SET);
      write_xmsg(SqdHandle, &(psqsort->xmsg));
      psqsort->sqidx.umsgid = umsgid++;
      write_sqidx(SqiHandle, &(psqsort->sqidx), 1);
      psqsort = psqsort->next;
   } /* endwhile */
}

void SquishSortArea(s_area *area)
{
   int       SqdHandle;
   int       SqiHandle;
   int       SqlHandle;
   long      i, msgs, idxPos;

   SQBASE    sqbase;
//   SQHDR     sqhdr;
   XMSG      xmsg;
   SQIDX     sqidx;
   SQSORTptr sqsort = NULL;

   dword     lastread;
   UMSGID    umsgid, firstumsgid = 0;

   char      *sqd, *sqi, *sql, *ptr;

   sqd = (char*)malloc(strlen(area->fileName)+5);
   sqi = (char*)malloc(strlen(area->fileName)+5);
   sql = (char*)malloc(strlen(area->fileName)+5);

   sprintf(sqd, "%s%s", area->fileName, EXT_SQDFILE);
   sprintf(sqi, "%s%s", area->fileName, EXT_SQIFILE);
   sprintf(sql, "%s%s", area->fileName, EXT_SQLFILE);

   SqdHandle = Open_File(sqd, fop_rpb);
   free(sqd);

   if (SqdHandle == -1) {
      free(sqi);
      free(sql);
      return;
   } /* endif */
      
   SqiHandle = Open_File(sqi, fop_rpb);
   free(sqi);

   if (SqiHandle == -1) {
      free(sql);
      close(SqdHandle);
      return;
   } /* endif */

   SqlHandle = Open_File(sql, fop_rpb);
   free(sql);

   if (SqlHandle == -1) {
      umsgid = 0;
   } else {
      if (lseek(SqlHandle, 0L, SEEK_SET) == -1) {
         umsgid = 0;
      } else {
         ptr = (char*)calloc(sizeof(dword)+1, sizeof(char));
         farread(SqlHandle, ptr, sizeof(dword));
         umsgid = get_dword(ptr);
         free(ptr);
      } /* endif */
   } /* endif */

   if (lock(SqdHandle, 0, 1) == 0) {
      lseek(SqdHandle, 0L, SEEK_SET);
      read_sqbase(SqdHandle, &sqbase);

      lseek(SqiHandle, 0L, SEEK_END);
      msgs = tell(SqiHandle)/SQIDX_SIZE;
      lseek(SqiHandle, 0L, SEEK_SET);

      for (i = 0, lastread = 0; i < msgs && umsgid != 0; i++) {
         read_sqidx(SqiHandle, &sqidx, 1);
         if (sqidx.umsgid == umsgid) {
            lastread = i + 1;
            break;
         } else {
         } /* endif */
      } /* endfor */

      lseek(SqiHandle, lastread*SQIDX_SIZE, SEEK_SET);
      idxPos = tell(SqiHandle);

      for (i = lastread; i < msgs; i++) {
         read_sqidx(SqiHandle, &(sqidx), 1);
         if (i == lastread) {
            firstumsgid = sqidx.umsgid;
         } /* endif */
         lseek(SqdHandle, sqidx.ofs + SQHDR_SIZE, SEEK_SET);
         read_xmsg(SqdHandle, &xmsg);

//         if (sqhdr.id != SQHDRID) continue;

         sqsort = AddSQSORT(sqsort, &sqidx, &xmsg);

      } /* endfor */

      SortSquishArea(sqsort, SqdHandle, SqiHandle, &sqbase, idxPos, firstumsgid);

      freesqsort(sqsort);

   } else {
   } /* endif */

   close(SqdHandle);
   close(SqiHandle);
   if (SqlHandle != -1) {
      close(SqlHandle);
   } /* endif */

}
/* --------------------------SQUISH sector OFF------------------------------ */

/* --------------------------JAM sector ON---------------------------------- */

JAMSORTptr AddJAMSORT(JAMSORTptr jamsort, JAMIDXRECptr SortIdx, JAMHDRptr SortHdr)
{

   JAMSORTptr newjamsort, pjamsort;

   newjamsort = (JAMSORTptr)malloc(sizeof(JAMSORT));

   memmove(&(newjamsort->Idx), SortIdx, IDX_SIZE);
   memmove(&(newjamsort->Hdr), SortHdr, HDR_SIZE);
   newjamsort->prev = newjamsort->next = NULL;

   if (jamsort == NULL) {
      pjamsort = newjamsort;
   } else {
      pjamsort = jamsort;
      if (newjamsort->Hdr.DateWritten > pjamsort->Hdr.DateWritten) {
         while (newjamsort->Hdr.DateWritten > pjamsort->Hdr.DateWritten && pjamsort->next) {
            pjamsort = pjamsort->next;
         } /* endwhile */
         if (newjamsort->Hdr.DateWritten > pjamsort->Hdr.DateWritten) {
            pjamsort->next = newjamsort;
            newjamsort->prev = pjamsort;
            pjamsort = jamsort;
         } else {
            newjamsort->prev = pjamsort->prev;
            newjamsort->next = pjamsort;
            pjamsort->prev->next = newjamsort;
            pjamsort->prev = newjamsort;
            pjamsort = jamsort;
         } /* endif */
      } else {
         newjamsort->next = pjamsort;
         pjamsort->prev = newjamsort;
         pjamsort = newjamsort;
      } /* endif */
   } /* endif */

   return pjamsort;
}

void freejamsort(JAMSORTptr jamsort)
{
   if (jamsort == NULL) {
      return;
   } /* endif */

   while (jamsort->next) {
      jamsort = jamsort->next;
      free(jamsort->prev);
   } /* endwhile */
   free(jamsort);
}

void SortJamArea(JAMSORTptr jamsort, int IdxHandle, int HdrHandle, long idxPos, long firstnum)
{
   JAMSORTptr pjamsort = jamsort;

   lseek(IdxHandle, idxPos, SEEK_SET);
   while (pjamsort) {
      pjamsort->Hdr.MsgNum = firstnum++;
      pjamsort->Hdr.ReplyTo = pjamsort->Hdr.Reply1st = pjamsort->Hdr.ReplyNext = 0;
      lseek(HdrHandle, pjamsort->Idx.HdrOffset, SEEK_SET);
      write_hdr(HdrHandle, &(pjamsort->Hdr));
      write_idx(IdxHandle, &(pjamsort->Idx));
      pjamsort = pjamsort->next;
   } /* endwhile */
   chsize(IdxHandle, tell(IdxHandle));

}

void JamSortArea(s_area *area)
{
   int        IdxHandle;
   int        HdrHandle;
   int        LrdHandle;
   long       i, msgs, idxPos, firstnum;

   dword      lastread;

   char       *hdr, *idx, *lrd, *ptr;

   JAMHDRINFO HdrInfo;
   JAMHDR     SortHdr;
   JAMIDXREC  SortIdx;

   JAMSORTptr jamsort = NULL;

   hdr = (char*)malloc(strlen(area->fileName)+5);
   idx = (char*)malloc(strlen(area->fileName)+5);
   lrd = (char*)malloc(strlen(area->fileName)+5);

   sprintf(hdr, "%s%s", area->fileName, EXT_HDRFILE);
   sprintf(idx, "%s%s", area->fileName, EXT_IDXFILE);
   sprintf(lrd, "%s%s", area->fileName, EXT_LRDFILE);

   IdxHandle = Open_File(idx, fop_rpb);

   free(idx);

   if (IdxHandle == -1) {
      free(hdr);
      return;
   } /* endif */

   HdrHandle = Open_File(hdr, fop_rpb);

   free(hdr);

   if (HdrHandle == -1) {
      close(IdxHandle);
      return;
   } /* endif */

   LrdHandle = Open_File(lrd, fop_rpb);

   if (LrdHandle == -1) {
      lastread = 0;
   } else {
      if (lseek(LrdHandle, 2*sizeof(dword), SEEK_SET) == -1) {
         lastread = 0;
      } else {
         ptr = (char*)calloc(sizeof(dword)+1, sizeof(char));
         farread(LrdHandle, ptr, sizeof(dword));
         lastread = get_dword(ptr);
         free(ptr);
      } /* endif */
   } /* endif */

   if (lock(HdrHandle, 0, 1) == 0) {
      lseek(HdrHandle, 0L, SEEK_SET);
      read_hdrinfo(HdrHandle, &HdrInfo);

      lseek(IdxHandle, 0L, SEEK_END);
      msgs = tell(IdxHandle) / IDX_SIZE;
      lseek(IdxHandle, 0L, SEEK_SET);

      if (lastread) {
         i = lastread - HdrInfo.BaseMsgNum + 1;
      } else {
         i = 0;
      } /* endif */

      lseek(IdxHandle, i * IDX_SIZE, SEEK_SET);
      idxPos = tell(IdxHandle);

      for (firstnum = 0; i < msgs; i++) {
         read_idx(IdxHandle, &SortIdx);
         if (SortIdx.HdrOffset == 0xffffffff) {
            continue;
         } /* endif */
         if (lseek(HdrHandle, SortIdx.HdrOffset, SEEK_SET) == -1) continue;
         read_hdr(HdrHandle, &SortHdr);

         if (CheckMsg(&SortHdr) == 0) {
            continue;
         } /* endif */

         if (firstnum == 0) {
            firstnum = SortHdr.MsgNum;
         } /* endif */

         jamsort = AddJAMSORT(jamsort, &SortIdx, &SortHdr);


      } /* endfor */

      SortJamArea(jamsort, IdxHandle, HdrHandle, idxPos, firstnum);

      freejamsort(jamsort);

      unlock(HdrHandle, 0, 1);

   } else {
   } /* endif */

   close(HdrHandle);
   close(IdxHandle);
   if (LrdHandle != -1) {
      close(LrdHandle);
   } /* endif */

   return;
}

/* --------------------------JAM sector OFF--------------------------------- */


void sortArea(s_area *area)
{
   int make = 0;
   if ((area->msgbType & MSGTYPE_JAM) == MSGTYPE_JAM) {
      fprintf(outfile, "is JAM ... ");
      JamSortArea(area);
      make = 1;
   } else {
      if ((area->msgbType & MSGTYPE_SQUISH) == MSGTYPE_SQUISH) {
         fprintf(outfile, "is Squish ... ");
         SquishSortArea(area);
         make = 1;
      } else {
         if ((area->msgbType & MSGTYPE_SDM) == MSGTYPE_SDM) {
            fprintf(outfile, "is MSG ... ");
//            MsgSortArea(area);
            make = 1;
         } else {
            if ((area->msgbType & MSGTYPE_PASSTHROUGH) == MSGTYPE_PASSTHROUGH) {
               fprintf(outfile, "is PASSTHROUGH ... ");
            } else {
            } /* endif */
         } /* endif */
      } /* endif */
   } /* endif */

   if (make) {
      fprintf(outfile, "Done\n");
   } else {
      fprintf(outfile, "Ignore\n");
   } /* endif */
}



void sortAreas(s_fidoconfig *config)
{
   int    i;
   char   *areaname;

   FILE   *f;

   outfile=stdout;

   setbuf(outfile, NULL);

   fprintf(outfile, "\nSort areas begin\n");

   if (config->importlog) {

      f = fopen(config->importlog, "rt");

      if (f) {
         fprintf(outfile, "Sort from \'importlog\'\n");
         while ((areaname = readLine(f)) != NULL) {

            // EchoAreas
            for (i = 0; i < config->echoAreaCount; i++) {
               if (stricmp(config->echoAreas[i].areaName, areaname) == 0) {
                  fprintf(outfile, "EchoArea %s ", config->echoAreas[i].areaName);
                  sortArea(&(config->echoAreas[i]));
               } else {
               } /* endif */
            } /* endfor */

            // LocalAreas
            for (i = 0; i < config->localAreaCount; i++) {
               if (stricmp(config->localAreas[i].areaName, areaname) == 0) {
                  fprintf(outfile, "LocalArea %s ", config->localAreas[i].areaName);
                  sortArea(&(config->localAreas[i]));
               } else {
               } /* endif */
            } /* endfor */

            // NetAreas
            for (i = 0; i < config->netMailAreaCount; i++) {
               if (stricmp(config->netMailAreas[i].areaName, areaname) == 0) {
                  fprintf(outfile, "NetArea %s ", config->netMailAreas[i].areaName);
                  sortArea(&(config->netMailAreas[i]));
               } else {
               } /* endif */
            } /* endfor */

            free(areaname);

         } /* endwhile */

         fclose(f);
         fprintf(outfile, "Sort areas end\n");
         return;
      } /* endif */
   } else {
   } /* endif */

   // EchoAreas
   for (i = 0; i < config->echoAreaCount; i++) {
      fprintf(outfile, "EchoArea %s ", config->echoAreas[i].areaName);
      sortArea(&(config->echoAreas[i]));
   } /* endfor */

   // LocalAreas
   for (i = 0; i < config->localAreaCount; i++) {
      fprintf(outfile, "LocalArea %s ", config->localAreas[i].areaName);
      sortArea(&(config->localAreas[i]));
   } /* endfor */

   // NetAreas
   for (i = 0; i < config->netMailAreaCount; i++) {
      fprintf(outfile, "NetArea %s ", config->netMailAreas[i].areaName);
      sortArea(&(config->netMailAreas[i]));
   } /* endfor */

   fprintf(outfile, "Sort areas end\n");
}
