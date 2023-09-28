# hptutil/Makefile
#
# This file is part of hptutil, part of the Husky fidonet software project
# Use with GNU version of make v.3.82 or later
# Requires: husky enviroment
#

hptutil_LIBS := $(fidoconf_TARGET_BLD) $(smapi_TARGET_BLD) $(huskylib_TARGET_BLD)

hptutil_CDEFS := $(CDEFS) \
                 -I$(hptutil_ROOTDIR)$(hptutil_H_DIR) \
                 -I$(fidoconf_ROOTDIR) \
                 -I$(smapi_ROOTDIR) \
                 -I$(huskylib_ROOTDIR)

hptutil_SRC  = $(wildcard $(hptutil_SRCDIR)*$(_C))
hptutil_OBJS = $(addprefix $(hptutil_OBJDIR),$(notdir $(hptutil_SRC:.c=$(_OBJ))))

hptutil_TARGET     = hptutil$(_EXE)
hptutil_TARGET_BLD = $(hptutil_BUILDDIR)$(hptutil_TARGET)
hptutil_TARGET_DST = $(BINDIR_DST)$(hptutil_TARGET)

ifdef MAN1DIR
    hptutil_MAN1PAGES := hptutil.1
    hptutil_MAN1BLD := $(hptutil_BUILDDIR)$(hptutil_MAN1PAGES).gz
    hptutil_MAN1DST := $(DESTDIR)$(MAN1DIR)$(DIRSEP)$(hptutil_MAN1PAGES).gz
endif

.PHONY: hptutil_build hptutil_install hptutil_uninstall hptutil_clean \
        hptutil_distclean hptutil_depend hptutil_rmdir_DEP hptutil_rm_DEPS \
        hptutil_clean_OBJ hptutil_main_distclean

hptutil_build: $(hptutil_TARGET_BLD) $(hptutil_MAN1BLD)

ifneq ($(MAKECMDGOALS), depend)
    ifneq ($(MAKECMDGOALS), distclean)
        ifneq ($(MAKECMDGOALS), uninstall)
            include $(hptutil_DEPS)
        endif
    endif
endif


# Build application
$(hptutil_TARGET_BLD): $(hptutil_OBJS) $(hptutil_LIBS) | do_not_run_make_as_root
	$(CC) $(LFLAGS) $(EXENAMEFLAG) $@ $^

# Compile .c files
$(hptutil_OBJS): $(hptutil_OBJDIR)%$(_OBJ): $(hptutil_SRCDIR)%$(_C) | $(hptutil_OBJDIR) do_not_run_make_as_root
	$(CC) $(hptutil_CFLAGS) $(hptutil_CDEFS) -o $@ $<

$(hptutil_OBJDIR): | $(hptutil_BUILDDIR) do_not_run_make_as_root
	[ -d $@ ] || $(MKDIR) $(MKDIROPT) $@


# Build man pages
ifdef MAN1DIR
    $(hptutil_MAN1BLD): $(hptutil_MANDIR)$(hptutil_MAN1PAGES) | do_not_run_make_as_root
	gzip -c $< > $@
else
    $(hptutil_MAN1BLD): ;
endif


# Install
ifneq ($(MAKECMDGOALS), install)
    hptutil_install: ;
else
    hptutil_install: $(hptutil_TARGET_DST) hptutil_install_man ;
endif

$(hptutil_TARGET_DST): $(hptutil_TARGET_BLD) | $(DESTDIR)$(BINDIR)
	$(INSTALL) $(IBOPT) $< $(DESTDIR)$(BINDIR); \
	$(TOUCH) "$@"

ifndef MAN1DIR
    hptutil_install_man: ;
else
    hptutil_install_man: $(hptutil_MAN1DST)

    $(hptutil_MAN1DST): $(hptutil_MAN1BLD) | $(DESTDIR)$(MAN1DIR)
	$(INSTALL) $(IMOPT) $< $(DESTDIR)$(MAN1DIR); $(TOUCH) "$@"
endif


# Clean
hptutil_clean: hptutil_clean_OBJ
	-[ -d "$(hptutil_OBJDIR)" ] && $(RMDIR) $(hptutil_OBJDIR) || true

hptutil_clean_OBJ:
	-$(RM) $(RMOPT) $(hptutil_OBJDIR)*

# Distclean
hptutil_distclean: hptutil_main_distclean hptutil_rmdir_DEP
	-[ -d "$(hptutil_BUILDDIR)" ] && $(RMDIR) $(hptutil_BUILDDIR) || true

hptutil_rmdir_DEP: hptutil_rm_DEPS
	-[ -d "$(hptutil_DEPDIR)" ] && $(RMDIR) $(hptutil_DEPDIR) || true

hptutil_rm_DEPS:
	-$(RM) $(RMOPT) $(hptutil_DEPDIR)*

hptutil_main_distclean: hptutil_clean
	-$(RM) $(RMOPT) $(hptutil_TARGET_BLD)
ifdef MAN1DIR
	-$(RM) $(RMOPT) $(hptutil_MAN1BLD)
endif


# Uninstall
hptutil_uninstall:
	-$(RM) $(RMOPT) $(hptutil_TARGET_DST)
ifdef MAN1DIR
	-$(RM) $(RMOPT) $(hptutil_MAN1DST)
endif
