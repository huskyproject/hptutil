# $Id$
# Common part of makefiles: object files list & etc.
# (Include file for other makefiles)
# Written by Stas Degteff 2:5080/102; g@grumbler.org

OBJS    = fixarea$(OBJ) sortarea$(OBJ) undelete$(OBJ) hptutil$(OBJ)

default: all

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) $(TARGETFLAG)$(TARGET) $(OBJS) $(LIBS)

commonclean:
	-$(RM) $(RMOPT) *$(OBJ)

commondistclean:
	-$(RM) $(RMOPT) $(TARGET)
