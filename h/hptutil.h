#ifndef __HPTUTIL_H__
#define __HPTUTIL_H__

#include <jam.h>
//#include <stdio.h>

#define VER_MAJOR 1
#define VER_MINOR 2

extern FILE *filesout;
extern FILE *fileserr;

extern char keepImportLog;
extern char typebase;
extern char *basefilename;
extern char *altImportLog;
extern unsigned int debugLevel;

extern int Open_File(char *name, word mode);
extern int CheckMsg(JAMHDRptr Hdr);
extern int InitSmapi(s_fidoconfig *config);

void OutScreen(char *str, ...);
void wait_d();

#endif
