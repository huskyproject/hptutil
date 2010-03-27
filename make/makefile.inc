# $Id$
#
# Common makefiles part (include-file) for hptutil

# Use defines before including this file:
# _OBJ	object files suffix begins with point (".o", ".obj", ... )
# _LIB	library files suffix begins with point (".a", ".lib", ... )
# _DLL	dinamic linking library files suffix begins with point (".dll", ".so", ...)
# LIBPREFIX  library filename prefix ("lib", "", ...)
# LIBSUFFIX  library filename suffix (compiler ID, three characters, somewhere empty)
# DLLPREFIX  dinamic linking library prefix (usually empty)
# DLLSUFFIX  dinamic linking library suffix (compiler ID or empty)
# O, B, L, H prefixes to use with names of objects, binaries, libraries and headers respectively

# Module name
LIBNAME = hptutil

# Version
VERMAJOR= 1
VERMINOR= 9
VERPATCH= 0
VERH	= $(VERMAJOR).$(VERMINOR)
VER	= $(VERH).$(VERPATCH)

# Sources directory
_SRC_DIR= ../src/
# Headers directory
H_DIR	= ../h/

# Header files
HEADERS	= $(H)cvsdate.h $(H)fixarea.h $(H)hptutil.h $(H)jam.h $(H)linkarea.h \
	  $(H)packarea.h $(H)purgearea.h $(H)sortarea.h $(H)squish.h \
	  $(H)undelete.h

# Object files (please sort list to easy check by human)
OBJS	= $(O)sortarea$(_OBJ) $(O)purgearea$(_OBJ) $(O)packarea$(_OBJ) \
	  $(O)linkarea$(_OBJ) $(O)fixarea$(_OBJ) $(O)undelete$(_OBJ) \
	  $(O)hptutil$(_OBJ)


# Binary file(s) to build from sources
PROGRAMS= $(B)$(LIBNAME)$(_EXE)

# List of libraries required to build binary file(s)
LIBS	=
