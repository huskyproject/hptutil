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
#include <hptutil.h>
#include <undelete.h>


int undeleteMsgs(s_fidoconfig *config, char *altImportLog)
{
    int rc = 0;
    
    return rc;
}