#ifndef __LINKAREA_H__
#define __LINKAREA_H__

#include <squish.h>

typedef struct {
   char  *reply;
   char  *msgid;
   dword ReplyTo;
   dword Reply1st;
   dword ReplyNext;
   dword msgnum;
   FOFS  HdrOffset;
   char  rewrite;
   dword ReplyToOld;
   dword Reply1stOld;
   dword ReplyNextOld;
} JAMINFO, *JAMINFOptr, **JAMINFOpptr;

typedef struct {
   char   *reply;
   char   *msgid;
   UMSGID replyto;
   dword  repliesCount;
   UMSGID replies[MAX_REPLY];
   UMSGID msgnum;
   FOFS   PosSqd;
   byte   rewrite;
   UMSGID replytoOld;
   UMSGID repliesOld[MAX_REPLY];
} SQINFO, *SQINFOptr, **SQINFOpptr;

void linkAreas(s_fidoconfig *config);

#endif
