#ifndef __SORTAREA_H__
#define __SORTAREA_H__

#include <jam.h>
#include <squish.h>
#include <time.h>

typedef struct _jamsort {
   JAMHDR    Hdr;
   JAMIDXREC Idx;
   struct    _jamsort *prev;
   struct    _jamsort *next;
   } JAMSORT, *JAMSORTptr;

typedef struct _sqsort {
   SQIDX    sqidx;
   XMSG     xmsg;
   time_t   msgtime;
   struct   _sqsort *prev;
   struct   _sqsort *next;
   } SQSORT, *SQSORTptr;

void sortAreas(s_fidoconfig *config);

#endif
