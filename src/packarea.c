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

#include <compiler.h>
#include <msgapi.h>
#include <fidoconfig.h>
#include <common.h>

#include <jam.h>
#include <squish.h>
#include <packarea.h>

extern FILE *outfile;
extern int Open_File(char *name, word mode);
extern int CheckMsg(JAMHDRptr Hdr);

long oldmsgs;

/* --------------------------SQUISH sector ON------------------------------- */

void SquishPackArea(char *areaName)
{
   int SqdHandle, SqiHandle;
   int NewSqdHandle, NewSqiHandle;
   long i;

   char *sqd, *sqi, *newsqd, *newsqi, firstmsg, *text;

   SQBASE     sqbase;
   SQHDR      sqhdr;
   XMSG       xmsg;
   SQIDX      sqidx;

   sqd = (char*)malloc(strlen(areaName)+5);
   sqi = (char*)malloc(strlen(areaName)+5);

   newsqd = (char*)malloc(strlen(areaName)+5);
   newsqi = (char*)malloc(strlen(areaName)+5);

   sprintf(sqd, "%s%s", areaName, EXT_SQDFILE);
   sprintf(sqi, "%s%s", areaName, EXT_SQIFILE);

   sprintf(newsqd, "%s.tmd", areaName);
   sprintf(newsqi, "%s.tmi", areaName);

   oldmsgs = 0;

   NewSqdHandle = Open_File(newsqd, fop_wpb);
   if (NewSqdHandle == -1) {
      free(sqd);
      free(sqi);
      free(newsqd);
      free(newsqi);
   } /* endif */
   NewSqiHandle = Open_File(newsqi, fop_wpb);
   if (NewSqiHandle == -1) {
      free(sqd);
      free(sqi);
      free(newsqd);
      free(newsqi);
      close(NewSqdHandle);
   } /* endif */

   SqdHandle = Open_File(sqd, fop_rpb);
   if (SqdHandle == -1) {
      free(sqd);
      free(sqi);
      free(newsqd);
      free(newsqi);
      close(NewSqdHandle);
      close(NewSqiHandle);
   } /* endif */
   SqiHandle = Open_File(sqi, fop_rpb);
   if (SqiHandle == -1) {
      free(sqd);
      free(sqi);
      free(newsqd);
      free(newsqi);
      close(NewSqdHandle);
      close(NewSqiHandle);
      close(SqdHandle);
   } /* endif */

   if (lock(SqdHandle, 0, 1) == 0) {
      lseek(SqiHandle, 0L, SEEK_END);
      oldmsgs = tell(SqiHandle)/SQIDX_SIZE;
      lseek(SqiHandle, 0L, SEEK_SET);

      read_sqbase(SqdHandle, &sqbase);
      sqbase.num_msg = 0;
      sqbase.high_msg = 0;
      sqbase.skip_msg = 0;
      sqbase.begin_frame = SQBASE_SIZE;
      sqbase.last_frame = sqbase.begin_frame;
      sqbase.free_frame = sqbase.last_free_frame = 0;
      sqbase.end_frame = sqbase.begin_frame;
      write_sqbase(NewSqdHandle, &sqbase);

      for (i = 0, firstmsg = 0; i < oldmsgs; i++) {
         read_sqidx(SqiHandle, &sqidx, 1);
         if (sqidx.ofs == 0 || sqidx.ofs == -1) continue;
         if (lseek(SqdHandle, sqidx.ofs, SEEK_SET) == -1) continue;

         read_sqhdr(SqdHandle, &sqhdr);
         if ((sqhdr.frame_type & FRAME_free) == FRAME_free) {
            continue;
         } else {
         } /* endif */

         read_xmsg(SqdHandle, &xmsg);

         text = (char*)calloc(sqhdr.frame_length, sizeof(char*));
         farread(SqdHandle, text, (sqhdr.frame_length - XMSG_SIZE));

         if (firstmsg == 0) {
            sqhdr.prev_frame = 0;
            firstmsg++;
         } else {
            sqhdr.prev_frame = sqbase.last_frame;
         } /* endif */

         sqhdr.next_frame = tell(NewSqdHandle) + SQHDR_SIZE + sqhdr.frame_length;

         sqbase.last_frame = tell(NewSqdHandle);
         sqidx.ofs = sqbase.last_frame;


         write_sqhdr(NewSqdHandle, &sqhdr);
         write_xmsg(NewSqdHandle, &xmsg);
         farwrite(NewSqdHandle, text, (sqhdr.frame_length - XMSG_SIZE));

         sqbase.end_frame = tell(NewSqdHandle);

         write_sqidx(NewSqiHandle, &sqidx, 1);
         free(text);

         sqbase.num_msg++;
         sqbase.high_msg = sqbase.num_msg;

      } /* endfor */

      lseek(NewSqiHandle, SQIDX_SIZE, SEEK_END);
      read_sqidx(NewSqiHandle, &sqidx, 1);
      lseek(NewSqdHandle, sqidx.ofs, SEEK_SET);
      read_sqhdr(NewSqdHandle, &sqhdr);
      sqhdr.next_frame = 0;
      lseek(NewSqdHandle, sqidx.ofs, SEEK_SET);
      write_sqhdr(NewSqdHandle, &sqhdr);
      lseek(NewSqdHandle, 0L, SEEK_SET);
      write_sqbase(NewSqdHandle, &sqbase);

      unlock(SqdHandle, 0, 1);

      close(NewSqdHandle);
      close(NewSqiHandle);
      close(SqdHandle);
      close(SqiHandle);

      remove(sqd);
      remove(sqi);
      rename(newsqd, sqd);
      rename(newsqi, sqi);

   } else {
      close(NewSqdHandle);
      close(NewSqiHandle);
      close(SqdHandle);
      close(SqiHandle);

   } /* endif */

   free(sqd);
   free(sqi);
   free(newsqd);
   free(newsqi);
}

/* --------------------------SQUISH sector OFF------------------------------ */


/* --------------------------JAM sector ON---------------------------------- */

void JamPackArea(char *areaName)
{
   int        IdxHandle, HdrHandle, TxtHandle;
   int        NewIdxHandle, NewHdrHandle, NewTxtHandle;
   int        firstnum;
   long       i, msgnum = 0;

   char       *hdr, *idx, *txt, *text;
   char       *newhdr, *newidx, *newtxt;

   dword      *numinfo/*, oldBaseMsgNum*/;
   JAMPACKptr replyinfo = NULL;
   JAMHDRINFO HdrInfo;
   JAMHDR     PackHdr;
   JAMIDXREC  PackIdx;
   JAMSUBFIELDptr Subf;

   hdr = (char*)malloc(strlen(areaName)+5);
   idx = (char*)malloc(strlen(areaName)+5);
   txt = (char*)malloc(strlen(areaName)+5);

   newhdr = (char*)malloc(strlen(areaName)+5);
   newidx = (char*)malloc(strlen(areaName)+5);
   newtxt = (char*)malloc(strlen(areaName)+5);

   sprintf(hdr, "%s%s", areaName, EXT_HDRFILE);
   sprintf(idx, "%s%s", areaName, EXT_IDXFILE);
   sprintf(txt, "%s%s", areaName, EXT_TXTFILE);

   sprintf(newhdr, "%s.tmh", areaName);
   sprintf(newidx, "%s.tmi", areaName);
   sprintf(newtxt, "%s.tmt", areaName);

   oldmsgs = 0;

   NewHdrHandle = Open_File(newhdr, fop_wpb);
   if (NewHdrHandle == -1) {
      free(hdr);
      free(idx);
      free(txt);
      free(newhdr);
      free(newidx);
      free(newtxt);
      return;
   } /* endif */
   NewIdxHandle = Open_File(newidx, fop_wpb);
   if (NewIdxHandle == -1) {
      free(hdr);
      free(idx);
      free(txt);
      free(newhdr);
      free(newidx);
      free(newtxt);
      close(NewHdrHandle);
      return;
   } /* endif */
   NewTxtHandle = Open_File(newtxt, fop_wpb);
   if (NewTxtHandle == -1) {
      free(hdr);
      free(idx);
      free(txt);
      free(newhdr);
      free(newidx);
      free(newtxt);
      close(NewHdrHandle);
      close(NewIdxHandle);
      return;
   } /* endif */

   HdrHandle = Open_File(hdr, fop_rpb);
   if (NewHdrHandle == -1) {
      free(hdr);
      free(idx);
      free(txt);
      free(newhdr);
      free(newidx);
      free(newtxt);
      close(NewHdrHandle);
      close(NewIdxHandle);
      close(NewTxtHandle);
      return;
   } /* endif */
   IdxHandle = Open_File(idx, fop_rpb);
   if (NewIdxHandle == -1) {
      free(hdr);
      free(idx);
      free(txt);
      free(newhdr);
      free(newidx);
      free(newtxt);
      close(NewHdrHandle);
      close(NewIdxHandle);
      close(NewTxtHandle);
      close(HdrHandle);
      return;
   } /* endif */
   TxtHandle = Open_File(txt, fop_rpb);
   if (NewTxtHandle == -1) {
      free(hdr);
      free(idx);
      free(txt);
      free(newhdr);
      free(newidx);
      free(newtxt);
      close(NewHdrHandle);
      close(NewIdxHandle);
      close(NewTxtHandle);
      close(HdrHandle);
      close(IdxHandle);
      return;
   } /* endif */


   if (lock(HdrHandle, 0, 1) == 0) {
      lseek(IdxHandle, 0L, SEEK_END);
      oldmsgs = tell(IdxHandle) / IDX_SIZE;
      lseek(IdxHandle, 0L, SEEK_SET);
      read_hdrinfo(HdrHandle, &HdrInfo);
      write_hdrinfo(NewHdrHandle, &HdrInfo);

      HdrInfo.ActiveMsgs = 0;

      numinfo = (dword*)calloc(oldmsgs, sizeof(dword));

      for (i = 0, firstnum = 0; i < oldmsgs; i++) {
         read_idx(IdxHandle, &PackIdx);
         if (PackIdx.HdrOffset == 0xffffffff) {
            HdrInfo.highwater--;
            continue;
         } /* endif */
         lseek(HdrHandle, PackIdx.HdrOffset, SEEK_SET);
         read_hdr(HdrHandle, &PackHdr);

         if (CheckMsg(&PackHdr) == 0) {
            continue;
         } /* endif */

         if (firstnum == 0) {
            HdrInfo.BaseMsgNum = PackHdr.MsgNum;
            msgnum = PackHdr.MsgNum;
            firstnum++;
         } else {
         } /* endif */

         PackHdr.MsgNum = msgnum;
         numinfo[i] = msgnum;
         replyinfo = (JAMPACKptr)realloc(replyinfo, (HdrInfo.ActiveMsgs+1)*sizeof(JAMPACK));
         replyinfo[HdrInfo.ActiveMsgs].Reply1st  = PackHdr.Reply1st;
         replyinfo[HdrInfo.ActiveMsgs].ReplyTo   = PackHdr.ReplyTo;
         replyinfo[HdrInfo.ActiveMsgs].ReplyNext = PackHdr.ReplyNext;

         Subf = (JAMSUBFIELDptr)malloc(PackHdr.SubfieldLen);
         read_subfield(HdrHandle, &Subf, PackHdr.SubfieldLen);

         text = (char*)calloc(PackHdr.TxtLen + 1, sizeof(char));
         lseek(TxtHandle, PackHdr.TxtOffset, SEEK_SET);
         farread(TxtHandle, text, PackHdr.TxtLen);

         PackHdr.TxtOffset = tell(NewTxtHandle);
         PackIdx.HdrOffset = tell(NewHdrHandle);

         write_hdr(NewHdrHandle, &PackHdr);
         write_subfield(NewHdrHandle, &Subf, PackHdr.SubfieldLen);
         free(Subf);

         farwrite(NewTxtHandle, text, PackHdr.TxtLen);
         free(text);

         write_idx(NewIdxHandle, &PackIdx);

         HdrInfo.ActiveMsgs++;
         HdrInfo.ModCounter++;
         msgnum++;

      } /* endfor */

      lseek(NewIdxHandle, 0L, SEEK_SET);
      for (i = 0; i < HdrInfo.ActiveMsgs; i++) {
         read_idx(NewIdxHandle, &PackIdx);
         lseek(NewHdrHandle, PackIdx.HdrOffset, SEEK_SET);
         read_hdr(NewHdrHandle, &PackHdr);
         if (replyinfo[i].Reply1st != 0 && replyinfo[i].Reply1st < oldmsgs) {
            PackHdr.Reply1st = numinfo[replyinfo[i].Reply1st];
         } /* endif */
         if (replyinfo[i].ReplyTo != 0 && replyinfo[i].ReplyTo < oldmsgs) {
            PackHdr.ReplyTo = numinfo[replyinfo[i].ReplyTo];
         } /* endif */
         if (replyinfo[i].ReplyNext != 0 && replyinfo[i].ReplyNext < oldmsgs) {
            PackHdr.ReplyNext = numinfo[replyinfo[i].ReplyNext];
         } /* endif */
         lseek(NewHdrHandle, PackIdx.HdrOffset, SEEK_SET);
         write_hdr(NewHdrHandle, &PackHdr);
         HdrInfo.ModCounter++;
      } /* endfor */

      lseek(NewHdrHandle, 0L, SEEK_SET);
      write_hdrinfo(NewHdrHandle, &HdrInfo);

      free(numinfo);
      free(replyinfo);

      unlock(HdrHandle, 0, 1);

      close(NewHdrHandle);
      close(NewIdxHandle);
      close(NewTxtHandle);
      close(HdrHandle);
      close(IdxHandle);
      close(TxtHandle);


      remove(hdr);
      remove(idx);
      remove(txt);
      rename(newhdr, hdr);
      rename(newidx, idx);
      rename(newtxt, txt);

   } else {
      close(NewHdrHandle);
      close(NewIdxHandle);
      close(NewTxtHandle);
      close(HdrHandle);
      close(IdxHandle);
      close(TxtHandle);

   } /* endif */

   free(newhdr);
   free(newidx);
   free(newtxt);
   free(hdr);
   free(idx);
   free(txt);

   return;
}

/* --------------------------JAM sector OFF--------------------------------- */

void packArea(s_area *area)
{
   int make = 0;
   if ((area->msgbType & MSGTYPE_JAM) == MSGTYPE_JAM) {
      fprintf(outfile, "is JAM ... ");
      JamPackArea(area->fileName);
      make = 1;
   } else {
      if ((area->msgbType & MSGTYPE_SQUISH) == MSGTYPE_SQUISH) {
         fprintf(outfile, "is Squish ... ");
         SquishPackArea(area->fileName);
         make = 1;
      } else {
         if ((area->msgbType & MSGTYPE_SDM) == MSGTYPE_SDM) {
            fprintf(outfile, "is MSG ... ");
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

void packAreas(s_fidoconfig *config)
{
   int  i;

   outfile=stdout;

   setbuf(outfile, NULL);

   fprintf(outfile, "\nPack areas begin\n");

   // EchoAreas
   for (i = 0; i < config->echoAreaCount; i++) {
      fprintf(outfile, "EchoArea %s ", config->echoAreas[i].areaName);
      packArea(&(config->echoAreas[i]));
   } /* endfor */

   // LocalAreas
   for (i = 0; i < config->localAreaCount; i++) {
      fprintf(outfile, "LocalArea %s ", config->localAreas[i].areaName);
      packArea(&(config->localAreas[i]));
   } /* endfor */

   // NetAreas
   for (i = 0; i < config->netMailAreaCount; i++) {
      fprintf(outfile, "NetArea %s ", config->netMailAreas[i].areaName);
      packArea(&(config->netMailAreas[i]));
   } /* endfor */

   fprintf(outfile, "Pack areas end\n");
}
