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

#include <huskylib/compiler.h>
#include <smapi/msgapi.h>
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>

#include <jam.h>
#include <squish.h>
#include <hptutil.h>
#include <linkarea.h>


int  linkmsgs;

int Open_File(char *name, word mode)
{
   int handle;

#ifdef UNIX
   handle = sopen(name, mode, SH_DENYNONE, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#else
   handle = sopen(name, mode, SH_DENYNONE, S_IREAD | S_IWRITE);
#endif

   return handle;
}

char *GetOnlyId(char *Id)
{
   char *str, *ptr;

   if (Id == NULL) {
      return NULL;
   } /* endif */
   
   ptr = strchr(Id, ' ');
   if (ptr) {
      str = strdup(ptr);
      nfree(Id);
      return str;
   } else {
      return Id;
   }

}

/* --------------------------SQUISH sector ON------------------------------- */

void SquishLinkArea(char *areaName)
{
   int        SqdHandle;
   int        SqiHandle;
   long       i, ii, msgs;

   SQBASE     sqbase;
   SQHDR      sqhdr;
   XMSG       xmsg;
   SQIDX      sqidx;

   SQINFOpptr msginfo = NULL;

   char       *sqd, *sqi, *ctrl;

   sqd = (char*)malloc(strlen(areaName)+5);
   sqi = (char*)malloc(strlen(areaName)+5);

   sprintf(sqd, "%s%s", areaName, EXT_SQDFILE);
   sprintf(sqi, "%s%s", areaName, EXT_SQIFILE);

   SqdHandle = Open_File(sqd, fop_rpb);
   nfree(sqd);

   if (SqdHandle == -1) {
      nfree(sqi);
      return;
   } /* endif */
      
   SqiHandle = Open_File(sqi, fop_rpb);
   nfree(sqi);

   if (SqiHandle == -1) {
      close(SqdHandle);
      return;
   } /* endif */

   if (lock(SqdHandle, 0, 1) == 0) {
      lseek(SqdHandle, 0L, SEEK_SET);
      read_sqbase(SqdHandle, &sqbase);

      lseek(SqiHandle, 0L, SEEK_END);
      msgs = tell(SqiHandle)/SQIDX_SIZE;
      lseek(SqiHandle, 0L, SEEK_SET);

      msginfo = (SQINFOpptr)calloc(msgs, sizeof(SQINFOptr));

      for (i = 0; i < msgs; i++) {
         read_sqidx(SqiHandle, &sqidx, 1);
         if (sqidx.ofs == 0 || sqidx.ofs == -1) continue;
         if (lseek(SqdHandle, sqidx.ofs, SEEK_SET) == -1) continue;

         read_sqhdr(SqdHandle, &sqhdr);
         if (sqhdr.id != SQHDRID) continue;
         if (sqhdr.frame_type != FRAME_normal) continue;
	 
         msginfo[i] = (SQINFOptr)calloc(1, sizeof(SQINFO));
	 
	 msginfo[i]->PosSqd = tell(SqdHandle);
	 
	 read_xmsg(SqdHandle, &xmsg);

	 msginfo[i]->replytoOld = xmsg.replyto;	 
	 memmove(msginfo[i]->repliesOld, xmsg.replies, sizeof(UMSGID)*MAX_REPLY);
	 
         ctrl = (char*)calloc(sqhdr.clen+1, sizeof(char));

         farread(SqdHandle, ctrl, sqhdr.clen);

         msginfo[i]->msgid = GetCtrlToken(ctrl, "MSGID: ");
         msginfo[i]->reply = GetCtrlToken(ctrl, "REPLY: ");
	 

         msginfo[i]->msgid = GetOnlyId(msginfo[i]->msgid);
         msginfo[i]->reply = GetOnlyId(msginfo[i]->reply);

         msginfo[i]->msgnum = sqidx.umsgid;

         nfree(ctrl);

      } /* endfor */
      
      for (i = 0; i < msgs; i++) {
         for (ii = 0; ii < msgs && msginfo[i]; ii++) {
            if (ii == i) {
               continue;
            } /* endif */
            if (msginfo[ii]) {
               if (msginfo[i]->msgid) {
                  if (msginfo[ii]->reply) {
                     if (strcmp(msginfo[i]->msgid, msginfo[ii]->reply) == 0) {
                        if (msginfo[i]->repliesCount < (MAX_REPLY - 1)) {
                           msginfo[i]->replies[msginfo[i]->repliesCount] = msginfo[ii]->msgnum;
			   if (msginfo[i]->replies[msginfo[i]->repliesCount] != 
			   msginfo[i]->repliesOld[msginfo[i]->repliesCount])
                              msginfo[i]->rewrite = 1;
                           msginfo[i]->repliesCount++;
                        } else {
                        } /* endif */
                        msginfo[ii]->replyto = msginfo[i]->msgnum;
			if (msginfo[ii]->replyto != msginfo[ii]->replytoOld)
                           msginfo[ii]->rewrite = 1;
                     } else {
                     } /* endif */
                  } else {
                  } /* endif */
               } else {
               } /* endif */
            } else {
            } /* endif */
         } /* endfor */
      } /* endfor */

      lseek(SqiHandle, 0L, SEEK_SET);

      for (i = 0, linkmsgs = 0; i < msgs; i++) {
         if (!msginfo[i]) continue;
	 
	 if (msginfo[i]->rewrite == 1) {
	 
            if (lseek(SqdHandle, msginfo[i]->PosSqd, SEEK_SET) != -1) {
	 
		read_xmsg(SqdHandle, &xmsg);
	       
		xmsg.replyto = msginfo[i]->replyto;
		memmove(xmsg.replies, msginfo[i]->replies, sizeof(UMSGID)*MAX_REPLY);
		  
		lseek(SqdHandle, msginfo[i]->PosSqd, SEEK_SET);
		write_xmsg(SqdHandle, &xmsg);
		  
		linkmsgs++;
	    } else {
	       fprintf(fileserr, "loop - %ld, Not seek SqdHanle in pos - %lx\n", i, msginfo[i]->PosSqd);
	    }
	 
	 }

         nfree(msginfo[i]->msgid);
         nfree(msginfo[i]->reply);
         nfree(msginfo[i]);

      } /* endfor */

      unlock(SqdHandle, 0, 1);
   } else {
   } /* endif */

   nfree(msginfo);

   close(SqdHandle);
   close(SqiHandle);
}

/* --------------------------SQUISH sector OFF------------------------------ */


/* --------------------------JAM sector ON---------------------------------- */

JAMSUBFIELDptr GetSubField(JAMSUBFIELDptr subf, word what, dword len)
{
   JAMSUBFIELDptr SubField;
   dword SubPos = 0;

   while (SubPos < len) {
      SubField = (JAMSUBFIELDptr)((char*)(subf)+SubPos);
      SubPos += (SubField->DatLen+sizeof(JAMBINSUBFIELD));
      if (SubField->LoID == what) {
         return SubField;
      } /* endif */
   } /* endwhile */

   return NULL;
}

int CheckMsg(JAMHDRptr Hdr)
{
   if (stricmp(Hdr->Signature, "JAM") != 0) {
      return 0;
   } /* endif */

   if ((Hdr->Attribute & JMSG_DELETED) == JMSG_DELETED) {
      return 0;
   } /* endif */

//   if (Hdr->TxtLen == 0) {
//      return 0;
//   } /* endif */

   return 1;
}

void JamLinkArea(char *areaName)
{
   int        IdxHandle;
   int        HdrHandle;
   long       i, ii, msgs;
   int        x;

   char       *hdr, *idx;

   JAMHDRINFO HdrInfo;
   JAMHDR     LinkHdr;
   JAMIDXREC  LinkIdx;
   JAMSUBFIELDptr Subf, MsgIdSub, ReplySub;
   
   JAMINFOpptr msginfo = NULL;

   hdr = (char*)malloc(x=strlen(areaName)+5);
   idx = (char*)malloc(x);

   sprintf(hdr, "%s%s", areaName, EXT_HDRFILE);
   sprintf(idx, "%s%s", areaName, EXT_IDXFILE);

   IdxHandle = Open_File(idx, fop_rpb);
   nfree(idx);

   if (IdxHandle == -1) {
      nfree(hdr);
      return;
   } /* endif */

   HdrHandle = Open_File(hdr, fop_rpb);
   nfree(hdr);

   if (HdrHandle == -1) {
      close(IdxHandle);
      return;
   } /* endif */

   if (lock(HdrHandle, 0, 1) == 0) {

      lseek(HdrHandle, 0L, SEEK_SET);
      read_hdrinfo(HdrHandle, &HdrInfo);

      lseek(IdxHandle, 0L, SEEK_END);
      msgs = tell(IdxHandle) / IDX_SIZE;
      lseek(IdxHandle, 0L, SEEK_SET);

      msginfo = (JAMINFOpptr)calloc(msgs, sizeof(JAMINFOptr));

      for (i = 0; i < msgs; i++) {
      
         read_idx(IdxHandle, &LinkIdx);
         if (LinkIdx.HdrOffset == 0xffffffff) {
            continue;
         } /* endif */
         lseek(HdrHandle, LinkIdx.HdrOffset, SEEK_SET);
         read_hdr(HdrHandle, &LinkHdr);

         if (CheckMsg(&LinkHdr) == 0) {
            continue;
         } /* endif */
	 

         Subf = (JAMSUBFIELDptr)malloc(LinkHdr.SubfieldLen);
         read_subfield(HdrHandle, &Subf, &(LinkHdr.SubfieldLen));
         MsgIdSub = GetSubField(Subf, JAMSFLD_MSGID, LinkHdr.SubfieldLen);
         ReplySub = GetSubField(Subf, JAMSFLD_REPLYID, LinkHdr.SubfieldLen);

         if (!MsgIdSub) {
            nfree(Subf);
            continue;
         } /* endif */

         msginfo[i] = (JAMINFOptr)calloc(1, sizeof(JAMINFO));
	 
	 msginfo[i]->ReplyToOld = LinkHdr.ReplyTo;
	 msginfo[i]->Reply1stOld = LinkHdr.Reply1st;
	 msginfo[i]->ReplyNextOld = LinkHdr.ReplyNext;
	 
	 msginfo[i]->HdrOffset = LinkIdx.HdrOffset;

         msginfo[i]->msgid = (char*)calloc(MsgIdSub->DatLen+1, sizeof(char));
         strncpy(msginfo[i]->msgid, MsgIdSub->Buffer, MsgIdSub->DatLen);

         msginfo[i]->msgid = GetOnlyId(msginfo[i]->msgid);

         if (ReplySub) {
            msginfo[i]->reply = (char*)calloc(ReplySub->DatLen+1, sizeof(char));
            strncpy(msginfo[i]->reply, ReplySub->Buffer, ReplySub->DatLen);
            msginfo[i]->reply = GetOnlyId(msginfo[i]->reply);
         } /* endif */

         msginfo[i]->msgnum = LinkHdr.MsgNum;

         nfree(Subf);
      } /* endfor */

      for (i = 0; i < msgs; i++) {
         for (ii = 0; ii < msgs && msginfo[i]; ii++) {
            if (ii == i) {
               continue;
            } /* endif */
            if (msginfo[ii]) {
               if (msginfo[i]->msgid) {
                  if (msginfo[ii]->reply) {
                     if (strcmp(msginfo[i]->msgid, msginfo[ii]->reply) == 0) {
                        if (msginfo[i]->Reply1st == 0) {
                           msginfo[i]->Reply1st = msginfo[ii]->msgnum;
			   if (msginfo[i]->Reply1st != msginfo[i]->Reply1stOld)
                              msginfo[i]->rewrite = 1;
                        } /* endif */
                        msginfo[ii]->ReplyTo = msginfo[i]->msgnum;
			if (msginfo[ii]->ReplyTo != msginfo[ii]->ReplyToOld)
                           msginfo[ii]->rewrite = 1;
                     } else {
                     } /* endif */
                  } else {
                  } /* endif */
               } else {
               } /* endif */
               if (msginfo[ii]->ReplyTo == msginfo[i]->ReplyTo && 
                   msginfo[ii]->ReplyTo != 0 && msginfo[i]->ReplyTo != 0 && 
                   ii > i && msginfo[i]->ReplyNext == 0) {
                  msginfo[i]->ReplyNext = msginfo[ii]->msgnum;
		  if (msginfo[i]->ReplyNext != msginfo[i]->ReplyNextOld)
                     msginfo[i]->rewrite = 1;
               } else {
               } /* endif */
            } else {
            } /* endif */
         } /* endfor */
      } /* endfor */


      lseek(IdxHandle, 0L, SEEK_SET);

      for (i = 0, linkmsgs = 0; i < msgs; i++) {
         if (!msginfo[i]) continue;
	 
	 if (msginfo[i]->rewrite == 1) {
            lseek(HdrHandle, msginfo[i]->HdrOffset, SEEK_SET);
            read_hdr(HdrHandle, &LinkHdr);
	    if (CheckMsg(&LinkHdr) == 1) {
	       LinkHdr.ReplyTo = msginfo[i]->ReplyTo;
               LinkHdr.Reply1st = msginfo[i]->Reply1st;
               LinkHdr.ReplyNext = msginfo[i]->ReplyNext;

               lseek(HdrHandle, msginfo[i]->HdrOffset, SEEK_SET);
               write_hdr(HdrHandle, &LinkHdr);

               HdrInfo.ModCounter++;
               linkmsgs++;
	    }
	 }

         nfree(msginfo[i]->msgid);
         nfree(msginfo[i]->reply);
         nfree(msginfo[i]);

      } /* endfor */

      lseek(HdrHandle, 0L, SEEK_SET);
      write_hdrinfo(HdrHandle, &HdrInfo);

      unlock(HdrHandle, 0, 1);

   } else {
   } /* endif */

   nfree(msginfo);

   close(IdxHandle);
   close(HdrHandle);
}

void JamLinkAreaByCRC(char *areaName)
{
   int        IdxHandle;
   int        HdrHandle;
   long       i, ii, msgs;
   int        x;

   char       *hdr, *idx;

   JAMHDRINFO HdrInfo;
   JAMHDR     LinkHdr;
   JAMIDXREC  LinkIdx;
   
   JAMINFOpptr_CRC msginfo = NULL;

   hdr = (char*)malloc(x=strlen(areaName)+5);
   idx = (char*)malloc(x);

   sprintf(hdr, "%s%s", areaName, EXT_HDRFILE);
   sprintf(idx, "%s%s", areaName, EXT_IDXFILE);

   IdxHandle = Open_File(idx, fop_rpb);
   nfree(idx);

   if (IdxHandle == -1) {
      nfree(hdr);
      return;
   } /* endif */

   HdrHandle = Open_File(hdr, fop_rpb);
   nfree(hdr);

   if (HdrHandle == -1) {
      close(IdxHandle);
      return;
   } /* endif */

   if (lock(HdrHandle, 0, 1) == 0) {

      lseek(HdrHandle, 0L, SEEK_SET);
      read_hdrinfo(HdrHandle, &HdrInfo);

      lseek(IdxHandle, 0L, SEEK_END);
      msgs = tell(IdxHandle) / IDX_SIZE;
      lseek(IdxHandle, 0L, SEEK_SET);

      msginfo = (JAMINFOpptr_CRC)calloc(msgs, sizeof(JAMINFOptr_CRC));

      for (i = 0; i < msgs; i++) {
      
         read_idx(IdxHandle, &LinkIdx);
         if (LinkIdx.HdrOffset == 0xffffffff) {
            continue;
         } /* endif */
         lseek(HdrHandle, LinkIdx.HdrOffset, SEEK_SET);
         read_hdr(HdrHandle, &LinkHdr);

         if (CheckMsg(&LinkHdr) == 0) {
            continue;
         } /* endif */

         msginfo[i] = (JAMINFOptr_CRC)calloc(1, sizeof(JAMINFO_CRC));
	 
	 msginfo[i]->ReplyToOld   = LinkHdr.ReplyTo;
	 msginfo[i]->Reply1stOld  = LinkHdr.Reply1st;
	 msginfo[i]->ReplyNextOld = LinkHdr.ReplyNext;
	 msginfo[i]->HdrOffset    = LinkIdx.HdrOffset;
         msginfo[i]->msgnum       = LinkHdr.MsgNum;
         msginfo[i]->msgidCRC     = LinkHdr.MsgIdCRC;
         msginfo[i]->replyCRC     = LinkHdr.ReplyCRC;
      } /* endfor */

      for (i = 0; i < msgs; i++) {
         for (ii = 0; ii < msgs && msginfo[i]; ii++) {
            // don't link self
            if (ii == i) continue;
            // second message
            if (msginfo[ii]) {
               // ...is reply to first message
               if (msginfo[i]->msgidCRC != -1L &&
                   msginfo[i]->msgidCRC == msginfo[ii]->replyCRC) {
                        // set to me if unset
                        if (msginfo[i]->Reply1st == 0)
                           msginfo[i]->Reply1st = msginfo[ii]->msgnum;
                        msginfo[ii]->ReplyTo = msginfo[i]->msgnum;
                     } /* endif */
               // ...is reply to the same message as the first one
               if (ii > i && msginfo[ii]->ReplyTo != 0 && 
                   msginfo[i]->ReplyNext == 0 && 
                   msginfo[ii]->ReplyTo == msginfo[i]->ReplyTo)
                  msginfo[i]->ReplyNext = msginfo[ii]->msgnum;
            } /* endif */
         } /* endfor */
      } /* endfor */


      lseek(IdxHandle, 0L, SEEK_SET);

      for (i = 0, linkmsgs = 0; i < msgs; i++) {
         if (!msginfo[i]) continue;
	 // rewrite check - moved here
	 if ( (msginfo[i]->Reply1st  != msginfo[i]->Reply1stOld ) ||
              (msginfo[i]->ReplyTo   != msginfo[i]->ReplyToOld  ) ||
              (msginfo[i]->ReplyNext != msginfo[i]->ReplyNextOld) )
         {
            lseek(HdrHandle, msginfo[i]->HdrOffset, SEEK_SET);
            read_hdr(HdrHandle, &LinkHdr);
	    if (CheckMsg(&LinkHdr) == 1) {
	       LinkHdr.ReplyTo = msginfo[i]->ReplyTo;
               LinkHdr.Reply1st = msginfo[i]->Reply1st;
               LinkHdr.ReplyNext = msginfo[i]->ReplyNext;

               lseek(HdrHandle, msginfo[i]->HdrOffset, SEEK_SET);
               write_hdr(HdrHandle, &LinkHdr);

               HdrInfo.ModCounter++;
               linkmsgs++;
	    }
	 }

         nfree(msginfo[i]);

      } /* endfor */

      lseek(HdrHandle, 0L, SEEK_SET);
      write_hdrinfo(HdrHandle, &HdrInfo);

      unlock(HdrHandle, 0, 1);

   } else {
   } /* endif */

   nfree(msginfo);

   close(IdxHandle);
   close(HdrHandle);
}

/* --------------------------JAM sector OFF--------------------------------- */

void linkArea(s_area *area)
{
   int make = 0;
   if (area->nolink) {
      OutScreen("has nolink option ... ");
   } else {
      if ((area->msgbType & MSGTYPE_JAM) == MSGTYPE_JAM) {
         OutScreen("is JAM ... ");
         if (jam_by_crc) JamLinkAreaByCRC(area->fileName);
           else JamLinkArea(area->fileName);
         make = (jam_by_crc) ? 2 : 1;
      } else {
         if ((area->msgbType & MSGTYPE_SQUISH) == MSGTYPE_SQUISH) {
            OutScreen("is Squish ... ");
            SquishLinkArea(area->fileName);
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
      OutScreen("Linked %d msgs%s\n", linkmsgs, make == 2 ? " (by CRC)" : "");
   } else {
      OutScreen("Ignore\n");
   } /* endif */
}

void linkAreas(s_fidoconfig *config)
{
   int    i;
   char   *areaname;

   FILE   *f;

   OutScreen("Link areas begin\n");
   if (config->importlog) {

      if (config->LinkWithImportlog != lwiNo) {
         f = fopen(config->importlog, "rt");
      } else {
         f = NULL;
      }
      if (f) {
         OutScreen("Link from \'%s\' importlog file\n", config->importlog);
         while ((areaname = readLine(f)) != NULL) {

            // EchoAreas
            for (i = 0; i < config->echoAreaCount; i++) {
               if (stricmp(config->echoAreas[i].areaName, areaname) == 0) {
                  OutScreen("EchoArea %s ", config->echoAreas[i].areaName);
                  linkArea(&(config->echoAreas[i]));
               } else {
               } /* endif */
            } /* endfor */

            // LocalAreas
            for (i = 0; i < config->localAreaCount; i++) {
               if (stricmp(config->localAreas[i].areaName, areaname) == 0) {
                  OutScreen("LocalArea %s ", config->localAreas[i].areaName);
                  linkArea(&(config->localAreas[i]));
               } else {
               } /* endif */
            } /* endfor */

            // NetAreas
            for (i = 0; i < config->netMailAreaCount; i++) {
               if (stricmp(config->netMailAreas[i].areaName, areaname) == 0) {
                  OutScreen("NetArea %s ", config->netMailAreas[i].areaName);
                  linkArea(&(config->netMailAreas[i]));
               } else {
               } /* endif */
            } /* endfor */

            nfree(areaname);

         } /* endwhile */

         fclose(f);
         if ((config->LinkWithImportlog == lwiKill) && (keepImportLog == 0)) remove(config->importlog);
         OutScreen("Link areas end\n\n");
         return;
      } /* endif */
   } else {
   } /* endif */

   // EchoAreas
   for (i = 0; i < config->echoAreaCount; i++) {
      OutScreen("EchoArea %s ", config->echoAreas[i].areaName);
      linkArea(&(config->echoAreas[i]));
   } /* endfor */

   // LocalAreas
   for (i = 0; i < config->localAreaCount; i++) {
      OutScreen("LocalArea %s ", config->localAreas[i].areaName);
      linkArea(&(config->localAreas[i]));
   } /* endfor */

   // NetAreas
   for (i = 0; i < config->netMailAreaCount; i++) {
      OutScreen("NetArea %s ", config->netMailAreas[i].areaName);
      linkArea(&(config->netMailAreas[i]));
   } /* endfor */

   OutScreen("Link areas end\n\n");
}
