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
#include <unistd.h>

#include <smapi/compiler.h>
#include <smapi/prog.h>
#include <smapi/msgapi.h>
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>

#include <jam.h>
#include <squish.h>
#include <linkarea.h>

extern FILE *outfile;
extern int Open_File(char *name, word mode);
extern int CheckMsg(JAMHDRptr Hdr);

/* --------------------------SQUISH sector ON------------------------------- */

void SquishDelMsg(int SqdHandle, int SqiHandle, SQBASEptr psqbase, SQHDRptr psqhdr, SQIDXptr psqidx, long SqdPos, long *pmsgs, long *pi)
{
   SQHDR sqhdr;

   if (psqhdr->prev_frame) {
      lseek(SqdHandle, psqhdr->prev_frame, SEEK_SET);
      read_sqhdr(SqdHandle, &sqhdr);
      sqhdr.next_frame = psqhdr->next_frame;
      lseek(SqdHandle, psqhdr->prev_frame, SEEK_SET);
      write_sqhdr(SqdHandle, &sqhdr);
      if (psqhdr->next_frame == 0) {
         psqbase->last_frame = psqhdr->prev_frame;
      } else {
      } /* endif */
   } else {
      psqbase->begin_frame = psqhdr->next_frame;
   } /* endif */

   if (psqhdr->next_frame) {
      lseek(SqdHandle, psqhdr->next_frame, SEEK_SET);
      read_sqhdr(SqdHandle, &sqhdr);
      sqhdr.prev_frame = psqhdr->prev_frame;
      lseek(SqdHandle, psqhdr->next_frame, SEEK_SET);
      write_sqhdr(SqdHandle, &sqhdr);
      if (psqhdr->prev_frame == 0) {
         psqbase->begin_frame = psqhdr->next_frame;
      } else {
      } /* endif */
   } else {
      psqbase->last_frame = psqhdr->prev_frame;
   } /* endif */

   if (psqbase->free_frame) {
      psqbase->last_free_frame = SqdPos;
   } else {
      psqbase->free_frame = SqdPos;
   } /* endif */

   psqhdr->frame_type |= FRAME_free;

   lseek(SqdHandle, SqdPos, SEEK_SET);
   write_sqhdr(SqdHandle, psqhdr);

   psqbase->num_msg--;
   psqbase->high_msg--;

   memmove(psqidx + *pi, psqidx + *pi + 1, ((*pmsgs - *pi - 1) * SQIDX_SIZE)); 
   (*pi)--;
   (*pmsgs)--;

}

void SquishPurgeArea(s_area *area, long *oldmsgs, long *purged)
{
   int      SqdHandle;
   int      SqiHandle;
   int      SqlHandle;
   long     i, msgs;

   SQBASE   sqbase;
   SQHDR    sqhdr;
   XMSG     xmsg;
   SQIDXptr sqidx;

   UMSGID   lastread;
   UMSGID   umsgid;
   FOFS     frame_prev;
   FOFS     frame_next;
   FOFS     SqdPos;

   char       *sqd, *sqi, *sql, *ptr;

   struct tm tmTime;
   time_t curtime, msgtime;

   curtime = time(NULL);

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
      return;
   } /* endif */
      
   SqiHandle = Open_File(sqi, fop_rpb);
   free(sqi);

   if (SqiHandle == -1) {
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
      *oldmsgs = msgs = tell(SqiHandle)/SQIDX_SIZE;
      lseek(SqiHandle, 0L, SEEK_SET);

      if (msgs) {
         sqidx = (SQIDXptr)malloc(msgs * SQIDX_SIZE);
         read_sqidx(SqiHandle, sqidx, msgs);

         for (i = 0, lastread = 0; i < msgs && umsgid != 0; i++) {
            if (sqidx[i].umsgid == umsgid) {
               lastread = i + 1;
               break;
            } else {
            } /* endif */
         } /* endfor */

         frame_prev = frame_next = 0;

         for (i = 0; i < msgs; i++) {
            lseek(SqdHandle, sqidx[i].ofs, SEEK_SET);
            SqdPos = tell(SqdHandle);
            read_sqhdr(SqdHandle, &sqhdr);
            read_xmsg(SqdHandle, &xmsg);

            if (sqhdr.id != SQHDRID) continue;

            if ((xmsg.attr & MSGLOCAL) == MSGLOCAL && (xmsg.attr & MSGSENT) != MSGSENT) {
               continue;
            } /* endif */

            if (area->keepUnread) {
               if (lastread < sqbase.num_msg) {
                  continue;
               } /* endif */
            } /* endif */

            if (area->killRead) {
               if (lastread >= sqbase.num_msg) {
                  SquishDelMsg(SqdHandle, SqiHandle, &sqbase, &sqhdr, sqidx, SqdPos, &msgs, &i);
                  (*purged)++;
                  continue;
               } /* endif */
            } /* endif */

            if (area->purge) {
               if ((xmsg.attr & MSGLOCAL) == MSGLOCAL) {
                  DosDate_to_TmDate((union stamp_combo*)&(xmsg.date_written), &tmTime);
               } else {
                  DosDate_to_TmDate((union stamp_combo*)&(xmsg.date_arrived), &tmTime);
               } /* endif */
               msgtime = mktime(&tmTime);
               if ((curtime - msgtime) > (area->purge * 24 * 60 * 60) ) {
                  SquishDelMsg(SqdHandle, SqiHandle, &sqbase, &sqhdr, sqidx, SqdPos, &msgs, &i);
                  (*purged)++;
                  continue;
               } /* endif */
            } /* endif */

            if (area->max) {
               if (sqbase.num_msg > area->max) {
                  SquishDelMsg(SqdHandle, SqiHandle, &sqbase, &sqhdr, sqidx, SqdPos, &msgs, &i);
                  (*purged)++;
               } else {
               } /* endif */
            } else {
            } /* endif */


         } /* endfor */

         lseek(SqdHandle, 0L, SEEK_SET);
         write_sqbase(SqdHandle, &sqbase);

         lseek(SqiHandle, 0L, SEEK_SET);
         write_sqidx(SqiHandle, sqidx, msgs);
         chsize(SqiHandle, tell(SqiHandle));

         free(sqidx);
      } else {
      } /* endif */

      unlock(SqdHandle, 0, 1);
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

void JamDelMsg(int HdrHandle, int IdxHandle, JAMHDRINFOptr HdrInfo, JAMHDRptr Hdr, JAMIDXRECptr Idx, long idxPos)
{

   lseek(HdrHandle, Idx->HdrOffset, SEEK_SET);
   lseek(IdxHandle, idxPos, SEEK_SET);

   HdrInfo->ActiveMsgs--;
   HdrInfo->ModCounter++;
   Hdr->TxtLen = 0;
   Hdr->Attribute |= JMSG_DELETED;
   Idx->UserCRC = 0xFFFFFFFFL;
   Idx->HdrOffset = 0xFFFFFFFFL;

   write_hdr(HdrHandle, Hdr);
   write_idx(IdxHandle, Idx);

   lseek(HdrHandle, 0L, SEEK_SET);
   write_hdrinfo(HdrHandle, HdrInfo);

}

void JamPurgeArea(s_area *area, long *oldmsgs, long *purged)
{
   int        IdxHandle;
   int        HdrHandle;
   int        LrdHandle;
   long       i, msgs, idxPos;

   dword      lastread;

   char       *hdr, *idx, *lrd, *ptr;

   JAMHDRINFO HdrInfo;
   JAMHDR     PurgeHdr;
   JAMIDXREC  PurgeIdx;

   time_t msgtime, curtime;

   curtime = time(NULL);

   hdr = (char*)malloc(strlen(area->fileName)+5);
   idx = (char*)malloc(strlen(area->fileName)+5);
   lrd = (char*)malloc(strlen(area->fileName)+5);

   sprintf(hdr, "%s%s", area->fileName, EXT_HDRFILE);
   sprintf(idx, "%s%s", area->fileName, EXT_IDXFILE);
   sprintf(lrd, "%s%s", area->fileName, EXT_LRDFILE);

//   oldmsgs = newmsgs = 0;

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
      *oldmsgs = msgs = tell(IdxHandle) / IDX_SIZE;
      lseek(IdxHandle, 0L, SEEK_SET);

      for (i = 0; i < msgs; i++) {
         idxPos = tell(IdxHandle);
         read_idx(IdxHandle, &PurgeIdx);
         if (PurgeIdx.HdrOffset == 0xffffffff) {
            continue;
         } /* endif */
         if (lseek(HdrHandle, PurgeIdx.HdrOffset, SEEK_SET) == -1) continue;
         read_hdr(HdrHandle, &PurgeHdr);

         if (CheckMsg(&PurgeHdr) == 0) {
            continue;
         } /* endif */

         if ((PurgeHdr.Attribute & JMSG_LOCAL) == JMSG_LOCAL && (PurgeHdr.Attribute & JMSG_SENT) != JMSG_SENT) {
            continue;
         } else {
         } /* endif */

         if (area->keepUnread) {
            if (lastread < PurgeHdr.MsgNum) {
               continue;
            } /* endif */
         } /* endif */

         if (area->killRead) {
            if (lastread >= PurgeHdr.MsgNum) {
               JamDelMsg(HdrHandle, IdxHandle, &HdrInfo, &PurgeHdr, &PurgeIdx, idxPos);
               (*purged)++;
               continue;
            } /* endif */
         } /* endif */

         if (area->purge) {
            if ((PurgeHdr.Attribute & JMSG_LOCAL) == JMSG_LOCAL) {
               msgtime = PurgeHdr.DateWritten;
            } else {
               msgtime = PurgeHdr.DateProcessed;
            } /* endif */
            if (abs(curtime - msgtime) > (area->purge * 24 * 60 * 60) ) {
               JamDelMsg(HdrHandle, IdxHandle, &HdrInfo, &PurgeHdr, &PurgeIdx, idxPos);
               (*purged)++;
               continue;
            } /* endif */
         } /* endif */

         if (area->max) {
            if (HdrInfo.ActiveMsgs > area->max) {
               JamDelMsg(HdrHandle, IdxHandle, &HdrInfo, &PurgeHdr, &PurgeIdx, idxPos);
               (*purged)++;
            } else {
            } /* endif */
         } else {
         } /* endif */

      } /* endfor */

      unlock(HdrHandle, 0, 1);

   } else {
   } /* endif */

   close(HdrHandle);
   close(IdxHandle);
   if (LrdHandle != -1) {
      close(LrdHandle);
   } /* endif */

   free(hdr);
   free(idx);
   free(lrd);

   return;
}
/* --------------------------JAM sector OFF--------------------------------- */

void purgeArea(s_area *area, long *oldmsgs, long *purged)
{
   int make = 0;
   *purged = *oldmsgs = 0;

   if ((area->msgbType & MSGTYPE_JAM) == MSGTYPE_JAM) {
      fprintf(outfile, "is JAM ... ");
      JamPurgeArea(area, oldmsgs, purged);
      make = 1;
   } else {
      if ((area->msgbType & MSGTYPE_SQUISH) == MSGTYPE_SQUISH) {
         fprintf(outfile, "is Squish ... ");
         SquishPurgeArea(area, oldmsgs, purged);
         make = 1;
      } else {
         if ((area->msgbType & MSGTYPE_SDM) == MSGTYPE_SDM) {
            fprintf(outfile, "is MSG ... ");
//            MsgPurgeArea(area, oldmsgs, purged);
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
      fprintf(outfile, "%lu-%lu=%lu, ", *oldmsgs, *purged, *oldmsgs-*purged);
      fprintf(outfile, "Done\n");
   } else {
      fprintf(outfile, "Ignore\n");
   } /* endif */
}

void purgeAreas(s_fidoconfig *config)
{
   int  i;
   long totoldmsgs = 0, totpurged = 0;
   long areaoldmsgs, areapurged;

   outfile=stdout;

   setbuf(outfile, NULL);

   fprintf(outfile, "\nPurge areas begin\n");
   // EchoAreas
   for (i = 0; i < config->echoAreaCount; i++) {
      fprintf(outfile, "EchoArea %s ", config->echoAreas[i].areaName);
      purgeArea(&(config->echoAreas[i]), &areaoldmsgs, &areapurged);
      totoldmsgs+=areaoldmsgs;
      totpurged+=areapurged;
   } /* endfor */

   // LocalAreas
   for (i = 0; i < config->localAreaCount; i++) {
      fprintf(outfile, "LocalArea %s ", config->localAreas[i].areaName);
      purgeArea(&(config->localAreas[i]), &areaoldmsgs, &areapurged);
      totoldmsgs+=areaoldmsgs;
      totpurged+=areapurged;
   } /* endfor */

   // NetAreas
   for (i = 0; i < config->netMailAreaCount; i++) {
      fprintf(outfile, "NetArea %s ", config->netMailAreas[i].areaName);
      purgeArea(&(config->netMailAreas[i]), &areaoldmsgs, &areapurged);
      totoldmsgs+=areaoldmsgs;
      totpurged+=areapurged;
   } /* endfor */

   fprintf(outfile, "Purge areas end\n");
   fprintf(outfile, "Total old msgs:%lu Total purged:%lu Total new msgs:%lu\n", 
           totoldmsgs, totpurged, totoldmsgs-totpurged);
}

