# include Husky-Makefile-Config
include ../huskymak.cfg

OBJS    = sortarea$(_OBJ) purgearea$(_OBJ) packarea$(_OBJ) linkarea$(_OBJ) \
          fixarea$(_OBJ) undelete$(_OBJ) hptutil$(_OBJ)
SRC_DIR = src/

ifeq ($(DEBUG), 1)
  CFLAGS  = $(DEBCFLAGS) $(WARNFLAGS) $(ADDCDEFS) -Ih -I$(INCDIR) -D$(OSTYPE)
  LFLAGS  = $(DEBLFLAGS) $(ADDLDEFS)
else
  CFLAGS  = $(OPTCFLAGS) $(WARNFLAGS) $(ADDCDEFS) -Ih -I$(INCDIR) -D$(OSTYPE)
  LFLAGS  = $(OPTLFLAGS) $(ADDLDEFS)
endif

ifeq ($(SHORTNAME), 1)
  LIBS=-L$(LIBDIR) -lsmapi -lfidoconf -lhusky 
else
  LIBS=-L$(LIBDIR) -lsmapi -lfidoconfig -lhusky 
endif


all: $(OBJS) hptutil$(_EXE)

%$(_OBJ): $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $(SRC_DIR)$*.c

hptutil$(_EXE): $(OBJS)
	$(CC) $(LFLAGS) -o hptutil$(_EXE) $(OBJS) $(LIBS)

clean:
	-$(RM) $(RMOPT) *$(_OBJ)
	-$(RM) $(RMOPT) *~
	-$(RM) $(RMOPT) core

distclean: clean
	-$(RM) $(RMOPT) hptutil$(_EXE)

install: hptutil$(_EXE)
	$(INSTALL) $(IBOPT) hptutil$(_EXE) $(BINDIR)

uninstall:
	-$(RM) $(RMOPT) $(BINDIR)$(DIRSEP)hptutil$(_EXE)

