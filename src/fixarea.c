/*
 *      hptUtil - purge, pack, link and sort utility for HPT
 *      by Fedor Lizunkov 2:5020/960@Fidonet
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

#if !defined (UNIX) && !defined (SASC)
#  include <io.h>
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if !defined (UNIX) && !defined (SASC)
#  include <share.h>
#endif
#if !(defined (_MSC_VER) && (_MSC_VER >= 1200))
#  include <unistd.h>
#endif

#include <huskylib/compiler.h>
#include <smapi/msgapi.h>
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>

#include <jam.h>
#include <squish.h>
#include <hptutil.h>
#include <fixarea.h>
#include "version.h"

FOFS findHdrId(int SqdHandle)
{
    char buf[4], * pbuf = buf;
    dword hdrId = 0, rc;
    FOFS ops;

    for( ;; )
    {
        ops = tell(SqdHandle);
        rc  = farread(SqdHandle, (byte * far) buf, sizeof(dword));

        if(rc == -1 || rc != sizeof(dword))
        {
            ops = -1;
            break;
        }

        hdrId = get_dword(pbuf);

        if(hdrId == SQHDRID)
        {
            lseek(SqdHandle, ops, SEEK_SET);
            break;
        }

        ops++;
        lseek(SqdHandle, ops, SEEK_SET);
    }
    return ops;
} /* findHdrId */

char * formatMsg(char ** text, int len)
{
    char * ptext, * ctrl, * ptr;
    char * buf;
    int i, cl;

    ctrl  = (char *)calloc(1, sizeof(char));
    buf   = (char *)calloc(len + 1, sizeof(char));
    ptext = *text;

    for(i = cl = 0; (ptext - *text) < len; i++)
    {
        if(*ptext == '\001')
        {
            ptr = strchr(ptext, '\r');

            if(ptr)
            {
                ctrl = (char *)realloc(ctrl, ptr - ptext + 2);
                strncat(ctrl, ptext, ptr - ptext + 1);
                ptext = ptr + 1;
            }
            else
            {
                ctrl = (char *)realloc(ctrl, strlen(ptext) + 1);
                strcat(ctrl, ptext);
                ptext += strlen(ptext) + 1;
            }
        }
        else
        {
            strcat(buf, ptext);
            ptext += len;
        }
    }
    nfree(*text);
    *text = buf;
    return ctrl;
} /* formatMsg */

int SquishFixArea()
{
    int SqdHandle;
    int fixmsg;
    FOFS SqdPos1, SqdPos2, EndPos;
    char * sqd, * ctlbuf, * text, * NewNameBase, * ptr;
    HAREA harea;
    HMSG hmsg;
    SQHDR sqhdr;
    XMSG xmsg;

    NewNameBase = (char *)calloc(strlen(basefilename) + 10, sizeof(char));
    strcpy(NewNameBase, basefilename);
    ptr = strrchr(NewNameBase, PATH_DELIM);

    if(ptr)
    {
        *(ptr + 1) = '\000';
    }
    else
    {
        NewNameBase[0] = '\000';
    }

    sprintf(NewNameBase + strlen(NewNameBase), "FixV%d_%02d", hptutil_VER_MAJOR, hptutil_VER_MINOR);
    sqd = (char *)malloc(strlen(basefilename) + 5);
    sprintf(sqd, "%s%s", basefilename, EXT_SQDFILE);
    SqdHandle = Open_File(sqd, fop_rob);

    if(SqdHandle == -1)
    {
        OutScreen("\nError!\n\tCan't open \'%s\' fix file\n", sqd);
        nfree(sqd);
        nfree(NewNameBase);
        return 1;
    } /* endif */

    OutScreen("Open \'%s\' fix file\n", sqd);
    harea = MsgOpenArea((UCHAR *)NewNameBase, MSGAREA_CREATE, MSGTYPE_SQUISH | MSGTYPE_NOTH);

    if(!harea)
    {
        OutScreen("\nError!\n\tMsgOpenArea error [\'%s\' file]\n", NewNameBase);
    }

    OutScreen("Open \'%s\' new base\n", NewNameBase);
    OutScreen("Check old base. Please wait ... ");
    lseek(SqdHandle, 0L, SEEK_END);
    EndPos = tell(SqdHandle);
    lseek(SqdHandle, 0L, SEEK_SET);

    for(fixmsg = 0; tell(SqdHandle) < EndPos; )
    {
        if(findHdrId(SqdHandle) != -1)
        {
            if(read_sqhdr(SqdHandle, &sqhdr) == 0)
            {
                break;
            }

            if(read_xmsg(SqdHandle, &xmsg) == 0)
            {
                break;
            }

            xmsg.attr    = 0;
            xmsg.replyto = 0;
            memset(xmsg.replies, '\000', sizeof(UMSGID) * MAX_REPLY);
            SqdPos1 = tell(SqdHandle);
            findHdrId(SqdHandle);
            SqdPos2 = tell(SqdHandle);
            text    = (char *)calloc(SqdPos2 - SqdPos1 + 2, sizeof(char));
            lseek(SqdHandle, SqdPos1, SEEK_SET);
            farread(SqdHandle, (byte far *)text, SqdPos2 - SqdPos1);
            lseek(SqdHandle, SqdPos1, SEEK_SET);
            ctlbuf = formatMsg(&text, SqdPos2 - SqdPos1 + 1);
            hmsg   = MsgOpenMsg(harea, MOPEN_CREATE, 0);

            if(hmsg)
            {
                MsgWriteMsg(hmsg,
                            0,
                            &xmsg,
                            (byte *)text,
                            (dword)strlen(text),
                            (dword)strlen(text),
                            (dword)strlen(ctlbuf),
                            (byte *)ctlbuf);
                fixmsg++;
            }

            MsgCloseMsg(hmsg);
            nfree(text);
            nfree(ctlbuf);
        }
    }
    OutScreen("Done\n");

    if(fixmsg)
    {
        OutScreen("%u msgs fixed\n", (unsigned)fixmsg);
    }

    MsgCloseArea(harea);

    if(fixmsg == 0)
    {
        MsgDeleteBase(NewNameBase, MSGTYPE_SQUISH);
        OutScreen("Nothing to fix. Delete \'%s.*\' new base\n", NewNameBase);
    }

    close(SqdHandle);
    nfree(NewNameBase);
    nfree(sqd);
    return 0;
} /* SquishFixArea */

int InitSmapi(s_fidoconfig * config)
{
    static struct _minf m;
    UINT16 defzone;

    if(config == NULL)
    {
        defzone = 2;
    }
    else
    {
        defzone = (UINT16)config->addr[0].zone;
    }

    m.req_version = 0;
    m.def_zone    = defzone;

    if(MsgOpenApi(&m) == 0)
    {
        return 0;
    }

    return 1;
}

int fixArea(s_fidoconfig * config)
{
    int rc = 0;

    OutScreen("Fix area begin\n");

    if(InitSmapi(config) == 0)
    {
        if(typebase == 1)
        {
            rc = SquishFixArea();
        }

        /* if (typebase == 2) JamFixArea(); */
    }
    else
    {
        rc = 5;
        OutScreen("\nError!\n\tMsgOpenApi open error\n\n");
    }

    nfree(basefilename);
    OutScreen("Fix area end\n\n");
    return rc;
}
