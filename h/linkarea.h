#ifndef __LINKAREA_H__
#define __LINKAREA_H__

typedef struct {
   char  *reply;
   char  *msgid;
   dword ReplyTo;
   dword Reply1st;
   dword ReplyNext;
   dword msgnum;
   char  rewrite;
} JAMINFO, *JAMINFOptr, **JAMINFOpptr;

typedef struct {
   char   *reply;
   char   *msgid;
   UMSGID replyto;
   int    repliesCount;
   UMSGID replies[MAX_REPLY];
   UMSGID msgnum;
   char   rewrite;
} SQINFO, *SQINFOptr, **SQINFOpptr;

void linkAreas(s_fidoconfig *config);

#endif
