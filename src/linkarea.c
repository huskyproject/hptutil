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

#include <smapi/prog.h>
#include <smapi/compiler.h>
#include <smapi/msgapi.h>
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>

#include <jam.h>
#include <squish.h>
#include <linkarea.h>


FILE *outfile;
int  linkmsgs;
extern char keepImportLog;

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

   ptr = Id+strlen(Id)-1;
   while (*ptr == ' ') *(ptr--) = '\0';

   if (*Id == '\0') {
      return Id;
   } /* endif */

   ptr = strrchr(Id, ' ');
   if (ptr) {
      str = strdup(ptr);
      free(Id);
      return str;
   } else {
      return Id;
   } /* endif */
}

/* --------------------------SQUISH sector ON------------------------------- */

void SquishLinkArea(char *areaName)
{
   int        SqdHandle;
   int        SqiHandle;
   long       i, ii, msgs;
   FOFS       PosSqd;

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
         read_xmsg(SqdHandle, &xmsg);

         ctrl = (char*)calloc(sqhdr.clen+1, sizeof(char));

         farread(SqdHandle, ctrl, sqhdr.clen);

         msginfo[i] = (SQINFOptr)calloc(1, sizeof(SQINFO));


         msginfo[i]->msgid = GetCtrlToken(ctrl, "MSGID: ");
         msginfo[i]->reply = GetCtrlToken(ctrl, "REPLY: ");

         msginfo[i]->msgid = GetOnlyId(msginfo[i]->msgid);
         msginfo[i]->reply = GetOnlyId(msginfo[i]->reply);

         msginfo[i]->msgnum = sqidx.umsgid;

         free(ctrl);

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
                           msginfo[i]->repliesCount++;
                           msginfo[i]->rewrite = 1;
                        } else {
                        } /* endif */
                        msginfo[ii]->replyto = msginfo[i]->msgnum;
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

         if (msginfo[i]->rewrite == 0) {
            lseek(SqiHandle, SQIDX_SIZE, SEEK_CUR);
            continue;
         } else {
         } /* endif */
         read_sqidx(SqiHandle, &sqidx, 1);
         if (sqidx.ofs == 0 || sqidx.ofs == -1) continue;
         if (lseek(SqdHandle, sqidx.ofs, SEEK_SET) == -1) continue;

         read_sqhdr(SqdHandle, &sqhdr);
         if (sqhdr.id != SQHDRID) continue;
         if (sqhdr.frame_type != FRAME_normal) continue;

         PosSqd = tell(SqdHandle);
         read_xmsg(SqdHandle, &xmsg);
         xmsg.replyto = msginfo[i]->replyto;
         for (ii = 0; ii < MAX_REPLY; ii++) {
            xmsg.replies[ii] = msginfo[i]->replies[ii];
         } /* endfor */
         lseek(SqdHandle, PosSqd, SEEK_SET);
         write_xmsg(SqdHandle, &xmsg);
         linkmsgs++;

         free(msginfo[i]->msgid);
         free(msginfo[i]->reply);
         free(msginfo[i]);

      } /* endfor */

      unlock(SqdHandle, 0, 1);
   } else {
   } /* endif */

   free(msginfo);

   close(SqdHandle);
   close(SqiHandle);
}

/* --------------------------SQUISH sector OFF------------------------------ */


/* --------------------------JAM sector ON---------------------------------- */

JAMSUBFIELDptr GetSubField(JAMSUBFIELDptr subf, dword what, dword len)
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

   char       *hdr, *idx;

   JAMHDRINFO HdrInfo;
   JAMHDR     LinkHdr;
   JAMIDXREC  LinkIdx;
   JAMSUBFIELDptr Subf, MsgIdSub, ReplySub;


   JAMINFOpptr msginfo = NULL;

   hdr = (char*)malloc(strlen(areaName)+5);
   idx = (char*)malloc(strlen(areaName)+5);

   sprintf(hdr, "%s%s", areaName, EXT_HDRFILE);
   sprintf(idx, "%s%s", areaName, EXT_IDXFILE);

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

         msginfo[i] = (JAMINFOptr)calloc(1, sizeof(JAMINFO));

         if (!MsgIdSub) {
            free(Subf);
            continue;
         } /* endif */

         msginfo[i]->msgid = (char*)calloc(MsgIdSub->DatLen+1, sizeof(char));
         strncpy(msginfo[i]->msgid, MsgIdSub->Buffer, MsgIdSub->DatLen);

         msginfo[i]->msgid = GetOnlyId(msginfo[i]->msgid);

         if (ReplySub) {
            msginfo[i]->reply = (char*)calloc(ReplySub->DatLen+1, sizeof(char));
            strncpy(msginfo[i]->reply, ReplySub->Buffer, ReplySub->DatLen);
            msginfo[i]->reply = GetOnlyId(msginfo[i]->reply);
         } /* endif */

         msginfo[i]->msgnum = LinkHdr.MsgNum;

         free(Subf);
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
                           msginfo[i]->rewrite = 1;
                        } /* endif */
                        msginfo[ii]->ReplyTo = msginfo[i]->msgnum;
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

         if (msginfo[i]->rewrite == 0) {
            lseek(IdxHandle, IDX_SIZE, SEEK_CUR);
            continue;
         } /* endif */

         read_idx(IdxHandle, &LinkIdx);
         if (LinkIdx.HdrOffset == 0xffffffff) {
            continue;
         } /* endif */
         lseek(HdrHandle, LinkIdx.HdrOffset, SEEK_SET);
         read_hdr(HdrHandle, &LinkHdr);

         if (CheckMsg(&LinkHdr) == 0) {
            continue;
         } /* endif */

         LinkHdr.ReplyTo = msginfo[i]->ReplyTo;
         LinkHdr.Reply1st = msginfo[i]->Reply1st;
         LinkHdr.ReplyNext = msginfo[i]->ReplyNext;

         lseek(HdrHandle, LinkIdx.HdrOffset, SEEK_SET);
         write_hdr(HdrHandle, &LinkHdr);

         HdrInfo.ModCounter++;
         linkmsgs++;

         free(msginfo[i]->msgid);
         free(msginfo[i]->reply);
         free(msginfo[i]);

      } /* endfor */

      lseek(HdrHandle, 0L, SEEK_SET);
      write_hdrinfo(HdrHandle, &HdrInfo);

      unlock(HdrHandle, 0, 1);

   } else {
   } /* endif */

   free(msginfo);

   close(IdxHandle);
   close(HdrHandle);
}

/* --------------------------JAM sector OFF--------------------------------- */

void linkArea(s_area *area)
{
   int make = 0;
   if (area->nolink) {
      fprintf(outfile, "has nolink option ... ");
   } else {
      if ((area->msgbType & MSGTYPE_JAM) == MSGTYPE_JAM) {
         fprintf(outfile, "is JAM ... ");
         JamLinkArea(area->fileName);
         make = 1;
      } else {
         if ((area->msgbType & MSGTYPE_SQUISH) == MSGTYPE_SQUISH) {
            fprintf(outfile, "is Squish ... ");
            SquishLinkArea(area->fileName);
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
   }

   if (make) {
      fprintf(outfile, "Linked %d msgs\n", linkmsgs);
   } else {
      fprintf(outfile, "Ignore\n");
   } /* endif */
}

void linkAreas(s_fidoconfig *config)
{
   int    i;
   char   *areaname;

   FILE   *f;

   outfile=stdout;

   setbuf(outfile, NULL);

   fprintf(outfile, "\nLink areas begin\n");
   if (config->importlog) {

      if ((config->LinkWithImportlog != NULL) && (stricmp(config->LinkWithImportlog, "no")!=0)){
         f = fopen(config->importlog, "rt");
      } else {
         f = NULL;
      }
      if (f) {
         fprintf(outfile, "Link from \'importlog\'\n");
         while ((areaname = readLine(f)) != NULL) {

            // EchoAreas
            for (i = 0; i < config->echoAreaCount; i++) {
               if (stricmp(config->echoAreas[i].areaName, areaname) == 0) {
                  fprintf(outfile, "EchoArea %s ", config->echoAreas[i].areaName);
                  linkArea(&(config->echoAreas[i]));
               } else {
               } /* endif */
            } /* endfor */

            // LocalAreas
            for (i = 0; i < config->localAreaCount; i++) {
               if (stricmp(config->localAreas[i].areaName, areaname) == 0) {
                  fprintf(outfile, "LocalArea %s ", config->localAreas[i].areaName);
                  linkArea(&(config->localAreas[i]));
               } else {
               } /* endif */
            } /* endfor */

            // NetAreas
            for (i = 0; i < config->netMailAreaCount; i++) {
               if (stricmp(config->netMailAreas[i].areaName, areaname) == 0) {
                  fprintf(outfile, "NetArea %s ", config->netMailAreas[i].areaName);
                  linkArea(&(config->netMailAreas[i]));
               } else {
               } /* endif */
            } /* endfor */

            free(areaname);

         } /* endwhile */

         fclose(f);
         if (stricmp(config->LinkWithImportlog, "kill")==0 && keepImportLog == 0) remove(config->importlog);
         fprintf(outfile, "Link areas end\n");
         return;
      } /* endif */
   } else {
   } /* endif */

   // EchoAreas
   for (i = 0; i < config->echoAreaCount; i++) {
      fprintf(outfile, "EchoArea %s ", config->echoAreas[i].areaName);
      linkArea(&(config->echoAreas[i]));
   } /* endfor */

   // LocalAreas
   for (i = 0; i < config->localAreaCount; i++) {
      fprintf(outfile, "LocalArea %s ", config->localAreas[i].areaName);
      linkArea(&(config->localAreas[i]));
   } /* endfor */

   // NetAreas
   for (i = 0; i < config->netMailAreaCount; i++) {
      fprintf(outfile, "NetArea %s ", config->netMailAreas[i].areaName);
      linkArea(&(config->netMailAreas[i]));
   } /* endfor */

   fprintf(outfile, "Sort areas end\n");
}
