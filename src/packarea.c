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
#include <smapi/msgapi.h>
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>

#include <jam.h>
#include <squish.h>
#include <hptutil.h>
#include <packarea.h>

long oldmsgs;
unsigned long areaOldSize, areaNewSize;
unsigned long allOldSize, allNewSize;

unsigned long areaSize(int h1, int h2, int h3)
{
    unsigned long size = 0;
    long ofs;
    
    if (h1 != -1) {
	ofs = tell(h1);
	lseek(h1, 0L, SEEK_END);
	size += tell(h1);
	lseek(h1, ofs, SEEK_SET);
    }
    if (h2 != -1) {
	ofs = tell(h2);
	lseek(h2, 0L, SEEK_END);
	size += tell(h2);
	lseek(h2, ofs, SEEK_SET);
    }
    if (h3 != -1) {
	ofs = tell(h3);
	lseek(h3, 0L, SEEK_END);
	size += tell(h3);
	lseek(h3, ofs, SEEK_SET);
    }
    
    return size;
    
}

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
   
   areaOldSize=areaNewSize = 0;

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
      return;
   } /* endif */
   
   NewSqiHandle = Open_File(newsqi, fop_wpb);
   if (NewSqiHandle == -1) {
      free(sqd);
      free(sqi);
      free(newsqd);
      free(newsqi);
      close(NewSqdHandle);
      return;
   } /* endif */

   SqdHandle = Open_File(sqd, fop_rpb);
   if (SqdHandle == -1) {
      free(sqd);
      free(sqi);
      free(newsqd);
      free(newsqi);
      close(NewSqdHandle);
      close(NewSqiHandle);
      return;
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
      return;
   } /* endif */

   if (lock(SqdHandle, 0, 1) == 0) {
   
      areaOldSize = areaSize(SqdHandle, SqiHandle, (int)-1);
      
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
      
      areaNewSize = areaSize(NewSqdHandle, NewSqiHandle, (int)-1);

      close(NewSqdHandle);
      close(NewSqiHandle);
      
      close(SqdHandle);
      close(SqiHandle);

      remove(sqd);
      remove(sqi);
      
      rename(newsqd, sqd);
      rename(newsqi, sqi);

   } else {
      areaOldSize = areaSize(SqdHandle, SqiHandle, (int)-1);
      areaNewSize = areaSize(NewSqdHandle, NewSqiHandle, (int)-1);
      
      close(NewSqdHandle);
      close(NewSqiHandle);
      
      close(SqdHandle);
      close(SqiHandle);
      
      remove(newsqd);
      remove(newsqi);

   } /* endif */

   free(sqd);
   free(sqi);
   
   free(newsqd);
   free(newsqi);
   
   return;
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
   
   areaOldSize = areaNewSize = 0;

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
   
      areaOldSize = areaSize(HdrHandle, IdxHandle, TxtHandle);
   
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
         read_subfield(HdrHandle, &Subf, &(PackHdr.SubfieldLen));

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
      
      areaNewSize = areaSize(NewHdrHandle, NewIdxHandle, NewTxtHandle);

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
      areaOldSize = areaSize(HdrHandle, IdxHandle, TxtHandle);
      areaNewSize = areaSize(NewHdrHandle, NewIdxHandle, NewTxtHandle);
      
      close(NewHdrHandle);
      close(NewIdxHandle);
      close(NewTxtHandle);
      
      close(HdrHandle);
      close(IdxHandle);
      close(TxtHandle);
      
      remove(newhdr);
      remove(newidx);
      remove(newtxt);

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
   if (area->nopack) OutScreen("has nopack option ... ");
   else {
	   if ((area->msgbType & MSGTYPE_JAM) == MSGTYPE_JAM) {
		   OutScreen("is JAM ... ");
		   JamPackArea(area->fileName);
		   make = 1;
	   } else {
		   if ((area->msgbType & MSGTYPE_SQUISH) == MSGTYPE_SQUISH) {
			   OutScreen("is Squish ... ");
			   SquishPackArea(area->fileName);
			   make = 1;
		   } else {
			   if ((area->msgbType & MSGTYPE_SDM) == MSGTYPE_SDM) {
				   OutScreen("is MSG ... ");
			   } else {
				   if ((area->msgbType & MSGTYPE_PASSTHROUGH) == MSGTYPE_PASSTHROUGH) {
					   OutScreen("is PASSTHROUGH ... ");
				   } else {
				   } /* endif */
			   } /* endif */
		   } /* endif */
	   } /* endif */
   }   

   if (make) {
	   OutScreen("%01lu.%lu Kb [%01lu.%lu Kb]\n", areaNewSize/1000, areaNewSize%1000,
	   areaOldSize/1000, areaOldSize%1000);
   } else {
	   OutScreen("Ignore\n");
   } /* endif */
}

void packAreas(s_fidoconfig *config)
{
   int  i;
   char *areaname;

   FILE *f;
   
   allOldSize = allNewSize = 0;

   OutScreen("Pack areas begin\n");
   
   if (altImportLog) {

      f = fopen(altImportLog, "rt");
      if (f) {
         OutScreen("Purge from \'%s\' alternative importlog file\n", altImportLog);
         while ((areaname = readLine(f)) != NULL) {

	    // EchoAreas
	    for (i = 0; i < config->echoAreaCount; i++) {
		if (stricmp(config->echoAreas[i].areaName, areaname) == 0) {
		    OutScreen("EchoArea %s ", config->echoAreas[i].areaName);
		    packArea(&(config->echoAreas[i]));
		    allOldSize += areaOldSize;
		    allNewSize += areaNewSize;
		}
	    } /* endfor */

	    // LocalAreas
	    for (i = 0; i < config->localAreaCount; i++) {
		if (stricmp(config->localAreas[i].areaName, areaname) == 0) {
		    OutScreen("LocalArea %s ", config->localAreas[i].areaName);
		    packArea(&(config->localAreas[i]));
		    allOldSize += areaOldSize;
		    allNewSize += areaNewSize;
		}
	    } /* endfor */

	    // NetAreas
	    for (i = 0; i < config->netMailAreaCount; i++) {
		if (stricmp(config->netMailAreas[i].areaName, areaname) == 0) {
		    OutScreen("NetArea %s ", config->netMailAreas[i].areaName);
		    packArea(&(config->netMailAreas[i]));
		    allOldSize += areaOldSize;
		    allNewSize += areaNewSize;
		}
	    }
        
	    free(areaname);

	 } /* endwhile */

         fclose(f);
         if (stricmp(config->LinkWithImportlog, "kill")==0 && keepImportLog == 0) remove(altImportLog);
	 OutScreen("Pack areas end\n");
         OutScreen("Old base size - %01lu.%lu Kb, new base size - %01lu.%lu Kb\n\n",
                    allOldSize/1000, allOldSize%1000,
	            allNewSize/1000, allNewSize%1000);
         return;
      } /* endif */
   } else {
   } /* endif */
   


   // EchoAreas
   for (i = 0; i < config->echoAreaCount; i++) {
      OutScreen("EchoArea %s ", config->echoAreas[i].areaName);
      packArea(&(config->echoAreas[i]));
      allOldSize += areaOldSize;
      allNewSize += areaNewSize;
   } /* endfor */

   // LocalAreas
   for (i = 0; i < config->localAreaCount; i++) {
      OutScreen("LocalArea %s ", config->localAreas[i].areaName);
      packArea(&(config->localAreas[i]));
      allOldSize += areaOldSize;
      allNewSize += areaNewSize;
   } /* endfor */

   // NetAreas
   for (i = 0; i < config->netMailAreaCount; i++) {
      OutScreen("NetArea %s ", config->netMailAreas[i].areaName);
      packArea(&(config->netMailAreas[i]));
      allOldSize += areaOldSize;
      allNewSize += areaNewSize;
   } /* endfor */

   OutScreen("Pack areas end\n");
   OutScreen("Old base size - %01lu.%lu Kb, new base size - %01lu.%lu Kb\n\n",
              allOldSize/1000, allOldSize%1000,
	      allNewSize/1000, allNewSize%1000);
}
