# this nice line comes from the linux kernel makefile
BASE_ARCH:=$(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc/ -e s/sparc64/sparc/ -e s/arm.*/arm/ -e s/sa110/arm/ -e s/alpha/axp/)
OS=$(shell uname)

ARCH=$(BASE_ARCH)

#####################################################################

DEBUG_BUILD?=YES

CC?=gcc
LD=gcc
RM?=rm

BUILDDIR=build$(ARCH)
BINDIR=bin/unix
DEPENDFILE_MM=$(BUILDDIR)/Makefile.d
MM_EXE=matchmaker.$(ARCH)

################
# CFLAGS
################
LOCALBASE?=/usr/local
X11BASE?=/usr/X11R6

CFLAGS_MM=$(CFLAGS) -pipe -I. -I$(LOCALBASE)/include -I$(X11BASE)/include -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -Wall

ifeq ($(DEBUG_BUILD),YES)
CFLAGS_MM += -O0 -ggdb -D_DEBUG
else
CFLAGS_MM += -O2 -fno-strict-aliasing -ffast-math -funroll-loops -DNDEBUG
endif

CFLAGS_MM+=-DDEDICATED_ONLY -DMATCHMAKER -DC_ONLY

################
# LDFLAGS
################
LDFLAGS_MM=-L/usr/local/lib -ldl -lm -lz

######################################################################################################
# Helper functions
######################################################################################################
DO_CC=echo "  > Compiling $<" && \
	$(CC) $(CFLAGS_MM) -c $< -o $@

######################################################################################################
# Files
######################################################################################################
CFILES_MM  = $(wildcard ./*.c)
CFILES_MM += ../null/cl_null.c ../null/cm_null.c ../null/irc_null.c ../null/sv_null.c ../null/ascript_null.c
CFILES_MM += ../qcommon/cmd.c ../qcommon/common.c ../qcommon/cvar.c ../qcommon/dynvar.c ../qcommon/files.c ../qcommon/glob.c ../qcommon/library.c ../qcommon/md5.c ../qcommon/sha1.c ../qcommon/mem.c ../qcommon/msg.c ../qcommon/net.c ../qcommon/net_chan.c ../qcommon/trie.c ../qcommon/svnrev.c ../qcommon/bignum.c ../qcommon/rsa.c
CFILES_MM += ../gameshared/q_math.c ../gameshared/q_shared.c
CFILES_MM += ../unix/unix_fs.c ../unix/unix_net.c ../unix/unix_lib.c ../unix/unix_sys.c

CFILES_MM_WITHOUT_PATH= $(notdir  $(CFILES_MM))
OFILES_MM=$(CFILES_MM_WITHOUT_PATH:.c=.o)

OBJS_MM = $(addprefix $(BUILDDIR)/, $(OFILES_MM) )
######################################################################################################
# Building Rules
######################################################################################################
.PHONY: mm mm-msg mm-compile mm-link \
	clean clean-msg clean-depend clean-mm

mm: $(BUILDDIR) mm-msg mm-compile mm-link
clean: clean-msg clean-depend clean-mm

################
# CLEAN
################
clean-msg:
	@echo "> *********************************************************"
	@echo "> * Cleaning build files"
	@echo "> *********************************************************"

clean-depend:
	@echo "  > Removing depend file" && \
	$(RM) $(DEPENDFILE_MM)

################
# MATCHMAKER
################
mm-msg:
	@echo "> *********************************************************"
	@echo "> * Building matchmaker"
	@echo "> *********************************************************"

mm-compile: $(OBJS_MM)
mm-link: $(BINDIR) $(BINDIR)/$(MM_EXE)

clean-mm:
	@echo "  > Removing matchmaker objects" && \
	$(RM) $(OBJS_MM)


$(BUILDDIR) $(BINDIR):
	@mkdir -p $@

######################################################################################################
# Linking Rules
######################################################################################################
$(BINDIR)/$(MM_EXE): $(OBJS_MM)
	@echo "  > Linking $@" && \
	$(LD) -o $@ $^ $(LDFLAGS_MM)

######################################################################################################
# Meta Rules
######################################################################################################
$(BUILDDIR)/%.o: ./%.c
	@$(DO_CC)

$(BUILDDIR)/%.o: ../null/%.c
	@$(DO_CC)

$(BUILDDIR)/%.o: ../qcommon/%.c
	@$(DO_CC)

$(BUILDDIR)/%.o: ../game/%.c
	@$(DO_CC)

$(BUILDDIR)/%.o: ../unix/%.c
	@$(DO_CC)

######################################################################################################
# Depends
######################################################################################################
$(DEPENDFILE_MM): $(BUILDDIR) $(CFILES_MM)
	@makedepend -f- -- $(CFLAGS_MM) -- $(CFILES_MM) 2> /dev/null | awk '/^[a-zA-Z_]/{ sub(/^([a-zA-Z_]*\/)+/, "$(BUILDDIR)/"); print $$0, "Makefile"; sub(/^.*\:/, "$(DEPENDFILE_MM):"); print $$0, "Makefile" }' > $(DEPENDFILE_MM)

-include $(DEPENDFILE_MM)
