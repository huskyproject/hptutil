#ifndef __HPTUTIL_H__
#define __HPTUTIL_H__

#include <stdio.h>

extern FILE *filesout;
extern FILE *fileserr;

extern char keepImportLog;
extern char *altImportLog;
extern unsigned int debugLevel;

extern int Open_File(char *name, word mode);
extern int CheckMsg(JAMHDRptr Hdr);

void OutScreen(char *str, ...);

#endif
