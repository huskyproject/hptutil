/*
 *      hptUtil - purge, pack, link and sort utility for HPT
 *      by Fedor Lizunkov 2:5020/960@Fidonet
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
#include <hptutil.h>
#include <sortarea.h>


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
      nfree(sqsort->prev);
   } /* endwhile */
   nfree(sqsort);
}

unsigned int SortSquishArea(SQSORTptr sqsort, int SqdHandle, int SqiHandle, SQBASEptr sqbase, long idxPos, UMSGID umsgid)
{
   SQSORTptr psqsort = sqsort;
   unsigned int i = 0;

   if (sqsort == NULL) {
      return i;
   } /* endif */

   lseek(SqiHandle, idxPos, SEEK_SET);

   while (psqsort) {
      psqsort->xmsg.replyto = 0;
      memset(psqsort->xmsg.replies, '\000', sizeof(UMSGID)*MAX_REPLY);
      psqsort->xmsg.umsgid = umsgid;
      lseek(SqdHandle, psqsort->sqidx.ofs+SQHDR_SIZE, SEEK_SET);
      write_xmsg(SqdHandle, &(psqsort->xmsg));
      psqsort->sqidx.umsgid = umsgid++;
      write_sqidx(SqiHandle, &(psqsort->sqidx), 1);
      psqsort = psqsort->next;
      i++;
   } /* endwhile */
   
   return i;
}

unsigned int SquishSortArea(s_area *area)
{
   int       SqdHandle;
   int       SqiHandle;
   int       SqlHandle;
   unsigned  int SortMsgs;
   long      i, msgs, idxPos, users;

   SQBASE    sqbase;
//   SQHDR     sqhdr;
   XMSG      xmsg;
   SQIDX     sqidx;
   SQSORTptr sqsort = NULL;

   dword     lastread;
   UMSGID    umsgid, umsgidTMP, firstumsgid = 0;

   char      *sqd, *sqi, *sql;

   sqd = (char*)malloc(strlen(area->fileName)+5);
   sqi = (char*)malloc(strlen(area->fileName)+5);
   sql = (char*)malloc(strlen(area->fileName)+5);

   sprintf(sqd, "%s%s", area->fileName, EXT_SQDFILE);
   sprintf(sqi, "%s%s", area->fileName, EXT_SQIFILE);
   sprintf(sql, "%s%s", area->fileName, EXT_SQLFILE);

   SqdHandle = Open_File(sqd, fop_rpb);
   nfree(sqd);

   if (SqdHandle == -1) {
      nfree(sqi);
      nfree(sql);
      return 0;
   } /* endif */
      
   SqiHandle = Open_File(sqi, fop_rpb);
   nfree(sqi);

   if (SqiHandle == -1) {
      nfree(sql);
      close(SqdHandle);
      return 0;
   } /* endif */

   SqlHandle = Open_File(sql, fop_rpb);
   nfree(sql);

   umsgid = 0;
   if (SqlHandle != -1) {
      if (lseek(SqlHandle, 0L, SEEK_END) != -1) {
         users = (tell(SqlHandle))/sizeof(dword);
	 lseek(SqlHandle, 0L, SEEK_SET);
	 for (i = 0; i < users; i++) {
            sql = (char*)calloc(sizeof(dword), sizeof(char));
            farread(SqlHandle, sql, sizeof(dword));
            umsgidTMP = get_dword(sql);
            nfree(sql);
	    if (umsgid < umsgidTMP) umsgid = umsgidTMP;
	 }
      }
   }

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

      idxPos = lastread*SQIDX_SIZE;
      lseek(SqiHandle, idxPos, SEEK_SET);

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

      SortMsgs = SortSquishArea(sqsort, SqdHandle, SqiHandle, &sqbase, idxPos, firstumsgid);

      freesqsort(sqsort);

   } else {
   } /* endif */

   close(SqdHandle);
   close(SqiHandle);
   if (SqlHandle != -1) {
      close(SqlHandle);
   } /* endif */
   
   return SortMsgs;

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
      nfree(jamsort->prev);
   } /* endwhile */
   nfree(jamsort);
}

unsigned int SortJamArea(JAMSORTptr jamsort, int IdxHandle, int HdrHandle, long idxPos, long firstnum)
{
   JAMSORTptr pjamsort = jamsort;
   unsigned int i = 0;

   lseek(IdxHandle, idxPos, SEEK_SET);
   while (pjamsort) {
      pjamsort->Hdr.MsgNum = firstnum++;
      pjamsort->Hdr.ReplyTo = pjamsort->Hdr.Reply1st = pjamsort->Hdr.ReplyNext = 0;
      lseek(HdrHandle, pjamsort->Idx.HdrOffset, SEEK_SET);
      write_hdr(HdrHandle, &(pjamsort->Hdr));
      write_idx(IdxHandle, &(pjamsort->Idx));
      pjamsort = pjamsort->next;
      i++;
   } /* endwhile */
   chsize(IdxHandle, tell(IdxHandle));

   return i;
}

unsigned int JamSortArea(s_area *area)
{
   int        IdxHandle;
   int        HdrHandle;
   int        LrdHandle;
   unsigned   int SortMsgs;
   long       i, msgs, idxPos, firstnum;

   dword      lastread, lstrdTMP, users;

   char       *hdr, *idx, *lrd;

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
   nfree(idx);

   if (IdxHandle == -1) {
      nfree(hdr);
      nfree(lrd);
      return 0;
   } /* endif */

   HdrHandle = Open_File(hdr, fop_rpb);
   nfree(hdr);

   if (HdrHandle == -1) {
      close(IdxHandle);
      nfree(lrd);
      return 0;
   } /* endif */

   LrdHandle = Open_File(lrd, fop_rpb);
   nfree(lrd);

   lastread = 0;
   if (LrdHandle != -1) {
      if (lseek(LrdHandle, 0L, SEEK_END) != -1) {
         users = (tell(LrdHandle))/sizeof(JAMLREAD);
	 lseek(LrdHandle, 0L, SEEK_SET);
	 for (i = 0; i < users; i++) {
            lrd = (char*)calloc(sizeof(JAMLREAD), sizeof(char));
            farread(LrdHandle, lrd, sizeof(JAMLREAD));
            lstrdTMP = get_dword(lrd+8);
            nfree(lrd);
	    if (lastread < lstrdTMP) lastread = lstrdTMP;
	 }
      }
   }

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

      SortMsgs = SortJamArea(jamsort, IdxHandle, HdrHandle, idxPos, firstnum);

      freejamsort(jamsort);

      unlock(HdrHandle, 0, 1);

   } else {
   } /* endif */

   close(HdrHandle);
   close(IdxHandle);
   if (LrdHandle != -1) {
      close(LrdHandle);
   } /* endif */

   return SortMsgs;
}

/* --------------------------JAM sector OFF--------------------------------- */


void sortArea(s_area *area)
{
   int make = 0;
   unsigned int SortMsgs = 0;
   
   if ((area->msgbType & MSGTYPE_JAM) == MSGTYPE_JAM) {
      OutScreen("is JAM ... ");
      SortMsgs = JamSortArea(area);
      make = 1;
   } else {
      if ((area->msgbType & MSGTYPE_SQUISH) == MSGTYPE_SQUISH) {
         OutScreen("is Squish ... ");
         SortMsgs = SquishSortArea(area);
         make = 1;
      } else {
         if ((area->msgbType & MSGTYPE_SDM) == MSGTYPE_SDM) {
            OutScreen("is MSG ... ");
//            MsgSortArea(area);
            make = 1;
         } else {
            if ((area->msgbType & MSGTYPE_PASSTHROUGH) == MSGTYPE_PASSTHROUGH) {
               OutScreen("is PASSTHROUGH ... ");
            } else {
            } /* endif */
         } /* endif */
      } /* endif */
   } /* endif */

   if (make) {
      if (SortMsgs) OutScreen("%d msgs sorted\n", SortMsgs);
      else OutScreen("Nothing to sort\n");
   } else {
      OutScreen("Ignore\n");
   } /* endif */
}



void sortAreas(s_fidoconfig *config)
{
   int    i;
   char   *areaname;

   FILE   *f;

   OutScreen("\nSort areas begin\n");

   if (config->importlog) {

      f = fopen(config->importlog, "rt");

      if (f) {
         OutScreen("Sort from \'%s\' importlog file\n", config->importlog);
         while ((areaname = readLine(f)) != NULL) {

            // EchoAreas
            for (i = 0; i < config->echoAreaCount; i++) {
               if (stricmp(config->echoAreas[i].areaName, areaname) == 0) {
                  OutScreen("EchoArea %s ", config->echoAreas[i].areaName);
                  sortArea(&(config->echoAreas[i]));
               } else {
               } /* endif */
            } /* endfor */

            // LocalAreas
            for (i = 0; i < config->localAreaCount; i++) {
               if (stricmp(config->localAreas[i].areaName, areaname) == 0) {
                  OutScreen("LocalArea %s ", config->localAreas[i].areaName);
                  sortArea(&(config->localAreas[i]));
               } else {
               } /* endif */
            } /* endfor */

            // NetAreas
            for (i = 0; i < config->netMailAreaCount; i++) {
               if (stricmp(config->netMailAreas[i].areaName, areaname) == 0) {
                  OutScreen("NetArea %s ", config->netMailAreas[i].areaName);
                  sortArea(&(config->netMailAreas[i]));
               } else {
               } /* endif */
            } /* endfor */

            nfree(areaname);

         } /* endwhile */

         fclose(f);
         OutScreen("Sort areas end\n\n");
         return;
      } else {
      }
      
   } else {
   } /* endif */

   // EchoAreas
   for (i = 0; i < config->echoAreaCount; i++) {
      OutScreen("EchoArea %s ", config->echoAreas[i].areaName);
      sortArea(&(config->echoAreas[i]));
   } /* endfor */

   // LocalAreas
   for (i = 0; i < config->localAreaCount; i++) {
      OutScreen("LocalArea %s ", config->localAreas[i].areaName);
      sortArea(&(config->localAreas[i]));
   } /* endfor */

   // NetAreas
   for (i = 0; i < config->netMailAreaCount; i++) {
      OutScreen("NetArea %s ", config->netMailAreas[i].areaName);
      sortArea(&(config->netMailAreas[i]));
   } /* endfor */

   OutScreen("Sort areas end\n\n");
}
