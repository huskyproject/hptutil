# include Husky-Makefile-Config
include ../huskymak.cfg

OBJS    = sortarea$(OBJ) purgearea$(OBJ) packarea$(OBJ) linkarea$(OBJ) \
          fixarea$(OBJ) undelete$(OBJ) hptutil$(OBJ)
SRC_DIR = src/

ifeq ($(DEBUG), 1)
  CFLAGS  = $(DEBCFLAGS) $(WARNFLAGS) $(ADDCDEFS) -Ih -I$(INCDIR) -D$(OSTYPE)
  LFLAGS  = -L$(LIBDIR) $(DEBLFLAGS) $(ADDLDEFS)
else
  CFLAGS  = $(OPTCFLAGS) $(WARNFLAGS) $(ADDCDEFS) -Ih -I$(INCDIR) -D$(OSTYPE)
  LFLAGS  = -L$(LIBDIR) $(OPTLFLAGS) $(ADDLDEFS)
endif


all: $(OBJS) hptutil$(EXE)

%$(OBJ): $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $(SRC_DIR)$*.c

hptutil$(EXE): $(OBJS)
	$(CC) $(LFLAGS) -o hptutil$(EXE) $(OBJS) -lfidoconf -lsmapi -lhusky

clean:
	-$(RM) $(RMOPT) *$(OBJ)
	-$(RM) $(RMOPT) *~
	-$(RM) $(RMOPT) core

distclean: clean
	-$(RM) $(RMOPT) hptutil$(EXE)

install: hptutil$(EXE)
	$(INSTALL) $(IBOPT) hptutil$(EXE) $(BINDIR)

uninstall:
	-$(RM) $(RMOPT) $(BINDIR)$(DIRSEP)hptutil$(EXE)

