# hptutil
[![Build Status](https://travis-ci.org/huskyproject/hptutil.svg?branch=master)](https://travis-ci.org/huskyproject/hptutil)
[![Build status](https://ci.appveyor.com/api/projects/status/5g8658gfok478bnw/branch/master?svg=true)](https://ci.appveyor.com/project/dukelsky/hptutil/branch/master)


## What this program can do

**hptutil purge** - checked all keywords from fidoconfig (maybe I've forgoten
something, but I tried). From fidoconfig - keepUnread, killRead, purge, max.
No packing, messages only "marked" as deleted. Reply chains not killed.

**hptutil pack** - packing msgbase, used function of renaming files, thats why
Golded can't stay in reading areas, in this case may be problems (1st warning!).
Squish packing didn't set umsgid=1 on first letter. No lastread changeing, but
if umsgid = FFFFFFFF - this is will be wrong - not checked (2nd warning!). In
Jam base first letter also non 1, no lastread changing, but if BaseMsgNum will
be FFFFFFFF and then 0 lastread's will be damaged - not checked (3rd warning!).
Reply chains not killed.

**hptutil sort** - if an importlog file exist, sorting only areas in this file,
sorting all areas if importlog not exist. File importlog not deleted. Sorting
started from lastread, condition - date of the message. Reply chains is killed.

**hptutil link** - if exist importlog file, linking only areas in this file,
linking all areas if importlog not exist. The deletion of importlog subscribed
in fidoconfig (and no deleted at all, if you use non documented option -k).
Linking all active messages in area.

**hptutil** can do one function at a time, for example

**hptutil purge**

**hptutil pack** (attention! bugz!)

or you may use several command line options, for example

**hptutil sort link**

If you set several command line options, work will be not in the order you have put it,
but first will be purge, then pack, sort, link.

MSG bases are not supported yet, but this is not difficult to implement.

This is like a "fish" - template which needs to be checked, better solutions in
algorythm, etc.

## Warning
Before running this tool, change sources to work with one
area, and make a backup of your message base. The program was tested in OS/2 only (watcom 11b).

With best regards Fedor Lizunkov, 2:5020/960
