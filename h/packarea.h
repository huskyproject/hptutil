#ifndef __PACKAREA_H__
#define __PACKAREA_H__

typedef struct {
   dword Reply1st;
   dword ReplyTo;
   dword ReplyNext;
   } JAMPACK, *JAMPACKptr;

void packAreas(s_fidoconfig *config);

#endif
