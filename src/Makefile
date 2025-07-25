# GNU Makefile for SRB2
# the poly3 Makefile adapted over and over...
#
# Copyright 1998-2000 DooM Legacy Team.
# Copyright 2020-2021 James R.
# Copyright 2003-2021 Sonic Team Junior.
#
# This program is free software distributed under the
# terms of the GNU General Public License, version 2.
# See the 'LICENSE' file for more details.
#
# Special targets:
#
# clean - remove executables and objects for this build
# cleandep - remove dependency files for this build
# distclean - remove entire executable, object and
#             dependency file directory structure.
# dump - disassemble executable
# info - print settings
#
# This Makefile can automatically detect the host system
# as well as the compiler version. If system or compiler
# version cannot be detected, you may need to set a flag
# manually.
#
# On Windows machines, 32-bit Windows is always targetted.
#
# Platform/system flags:
#
# LINUX=1, LINUX64=1
# MINGW=1, MINGW64=1 - Windows (MinGW toolchain)
# UNIX=1 - Generic Unix like system
# FREEBSD=1
# SDL=1 - Use SDL backend. SDL is the only backend though
#         and thus, always enabled.
#
# A list of supported GCC versions can be found in
# Makefile.d/detect.mk -- search 'gcc_versions'.
#
# Feature flags:
#
# Safe to use online
# ------------------
# NO_IPV6=1 - Disable IPv6 address support.
# NOHW=1 - Disable OpenGL renderer.
# ZDEBUG=1 - Enable more detailed memory debugging
# HAVE_MINIUPNPC=1 - Enable automated port forwarding.
#                    Already enabled by default for 32-bit
#                    Windows.
# NOPNG=1 - Disable PNG graphics support. (TODO: double
#           check netplay compatible.)
# NOCURL=1 - Disable libcurl--HTTP capability.
# NOGME=1 - Disable game music emu, retro VGM support.
# NOOPENMPT=1 - Disable module (tracker) music support.
# NOMIXER=1 - Disable SDL Mixer (audio playback).
# NOTHREADS=1 - Disable multithreading.
# NOEXECINFO=1 - Disable stack trace dump support
# NOLTO=1 - Disable link time optimizations
#
# Netplay incompatible
# --------------------
# NONET=1 - Disable online capability.
# NOMD5=1 - Disable MD5 checksum (validation tool).
# NOPOSTPROCESSING=1 - ?
# MOBJCONSISTANCY=1 - ??
# PACKETDROP=1 - ??
# DEBUGMODE=1 - Enable various debugging capabilities.
#               Also disables optimizations.
# NOZLIB=1 - Disable some compression capability. Implies
#            NOPNG=1.
#
# Development flags:
#
# VALGRIND=1 - Enable Valgrind memory debugging support.
# PROFILEMODE=1 - Enable performance profiling (gprof).
#
# General flags for building:
#
# STATIC=1 - Use static linking.
# DISTCC=1
# CCACHE=1
# UPX= - UPX command to use for compressing final
#        executable.
# WINDOWSHELL=1 - Use Windows commands.
# PREFIX= - Prefix to many commands, for cross compiling.
# STABS=1 - ?
# ECHO=1 - Print out each command in the build process.
# NOECHOFILENAMES=1 - Don't print out each that is being
#                     worked on.
# SILENT=1 - Print absolutely nothing except errors.
# RELAXWARNINGS=1 - Use less compiler warnings/errors.
# ERRORMODE=1 - Treat most compiler warnings as errors.
# NOCASTALIGNWARN=1 - ?
# NOLDWARNING=1 - ?
# NOSDLMAIN=1 - ?
# SDLMAIN=1 - ?
#
# Library configuration flags:
# Everything here is an override.
#
# PNG_PKGCONFIG= - libpng-config command.
# PNG_CFLAGS=, PNG_LDFLAGS=
#
# CURLCONFIG= - curl-config command.
# CURL_CFLAGS=, CURL_LDFLAGS=
#
# VALGRIND_PKGCONFIG= - pkg-config package name.
# VALGRIND_CFLAGS=, VALGRIND_LDFLAGS=
#
# LIBGME_PKGCONFIG=, LIBGME_CFLAGS=, LIBGME_LDFLAGS=

# LIBOPENMPT_PKGCONFIG=
# LIBOPENMPT_CFLAGS=, LIBOPENMPT_LDFLAGS=
#
# ZLIB_PKGCONFIG=, ZLIB_CFLAGS=, ZLIB_LDFLAGS=
#
# SDL_PKGCONFIG=
# SDL_CONFIG= - sdl-config command.
# SDL_CFLAGS=, SDL_LDFLAGS=

clean_targets=cleandep clean distclean info

.PHONY : $(clean_targets) all

goals:=$(or $(MAKECMDGOALS),all)
cleanonly:=$(filter $(clean_targets),$(goals))
destructive:=$(filter-out info,$(cleanonly))

ifndef cleanonly
include Makefile.d/old.mk
endif

include Makefile.d/util.mk

ifdef PREFIX
CC:=$(PREFIX)-gcc
endif

OBJDUMP_OPTS?=--wide --source --line-numbers

OBJCOPY:=$(call Prefix,objcopy)
OBJDUMP:=$(call Prefix,objdump)
WINDRES:=$(call Prefix,windres)

GZIP?=gzip
GZIP_OPTS?=-9 -f -n
ifdef WINDOWSHELL
GZIP_OPTS+=--rsyncable
endif

UPX_OPTS?=--best --preserve-build-id
ifndef ECHO
UPX_OPTS+=-qq
endif

include Makefile.d/detect.mk

# make would try to remove the implicitly made directories
.PRECIOUS : %/ comptime.c

sources:=
makedir:=../make

# -DCOMPVERSION: flag to use comptime.h
opts:=-DCOMPVERSION -g
libs:=

# This is a list of variables names, of which if defined,
# also defines the name as a macro to the compiler.
passthru_opts:=

include Makefile.d/platform.mk
include Makefile.d/features.mk
include Makefile.d/versions.mk

ifdef DEBUGMODE
makedir:=$(makedir)/debug
endif

depdir:=$(makedir)/deps
objdir:=$(makedir)/objs

# very sophisticated dependency
sources+=\
	$(call List,Sourcefile)\
	$(call List,blua/Sourcefile)\

depends:=$(basename $(filter %.c %.s,$(sources)))
objects:=$(basename $(filter %.c %.s %.nas,$(sources)))

depends:=$(depends:%=$(depdir)/%.d)

# comptime.o added directly to objects instead of thru
# sources because comptime.c includes comptime.h, but
# comptime.h may not exist yet. It's a headache so this is
# easier.
objects:=$(objects:=.o) comptime.o

# windows resource file
rc_file:=$(basename $(filter %.rc,$(sources)))
ifdef rc_file
objects+=$(rc_file:=.res)
endif

objects:=$(addprefix $(objdir)/,$(objects))

ifdef DEBUGMODE
bin:=../bin/debug
else
bin:=../bin
endif

# default EXENAME (usually set by platform)
EXENAME?=srb2legacy
DBGNAME?=$(EXENAME).debug

exe:=$(bin)/$(EXENAME)
dbg:=$(bin)/$(DBGNAME)

build_done=Build is done, please look for $(<F) in $(<D)

all : $(exe)
	$(call Echo,$(build_done))

ifndef VALGRIND
dump : $(dbg).txt
endif

ifdef STATIC
libs+=-static
endif

# build with profiling information
ifdef PROFILEMODE
opts+=-pg
libs+=-pg
endif

ifdef DEBUGMODE
debug_opts=-D_DEBUG
else # build a normal optimized version
debug_opts=-DNDEBUG
opts+=-O3
ifndef NOLTO
opts+=-flto=auto
libs+=-flto=auto
endif
endif

# debug_opts also get passed to windres
opts+=$(debug_opts)

opts+=$(foreach v,$(passthru_opts),$(if $($(v)),-D$(v)))

opts+=$(WFLAGS) $(CPPFLAGS) $(CFLAGS)
libs+=$(LDFLAGS)
asflags:=$(ASFLAGS) -x assembler-with-cpp

cc=$(CC)

ifdef DISTCC
cc=distcc $(CC)
endif

ifdef CCACHE
cc=ccache $(CC)
endif

ifndef SILENT
# makefile will 'restart' when it finishes including the
# dependencies.
ifndef MAKE_RESTARTS
ifndef destructive
$(shell $(CC) -v)
define flags =

CC ........ $(cc)

CFLAGS .... $(opts)

LDFLAGS ... $(libs)

endef
$(info $(flags))
endif
# don't generate dependency files if only cleaning
ifndef cleanonly
$(info Checking dependency files...)
include $(depends)
endif
endif
endif

LD:=$(CC)
cc:=$(cc) $(opts)
ifdef UPX
upx=$(UPX) $(UPX_OPTS)
endif
windres=$(WINDRES) $(WINDRESFLAGS)\
	$(debug_opts) --include-dir=win32 -O coff

%/ :
	$(.)$(mkdir) $(call Windows_path,$@)

# this is needed so the target can be referenced in the
# prerequisites
.SECONDEXPANSION :

# 'UPX' is also recognized in the environment by upx
unexport UPX

# executable stripped of debugging symbols
$(exe) : $(dbg) | $$(@D)/
	$(.)$(OBJCOPY) --strip-debug $< $@
	$(.)-$(OBJCOPY) --add-gnu-debuglink=$< $@
ifdef UPX
	$(call Echo,Compressing final executable...)
	$(.)-$(upx) $@
endif

# original executable with debugging symbols
$(dbg) : $(objects) | $$(@D)/
	$(call Echo,Linking $(@F)...)
	$(.)$(LD) -o $@ $^ $(libs)

# disassembly of executable
$(dbg).txt : $(dbg)
	$(call Echo,Dumping debugging info...)
	$(.)$(OBJDUMP) $(OBJDUMP_OPTS) $< > $@
	$(.)$(GZIP) $(GZIP_OPTS) $@

# '::' means run unconditionally
# this really updates comptime.h
comptime.c ::
ifdef WINDOWSHELL
	-$(.)..\comptime.bat .
else
	-$(.)../comptime.sh .
endif

# I wish I could make dependencies out of rc files :(
$(objdir)/win32/Srb2win.res : \
	win32/afxres.h win32/resource.h

# dependency recipe template
# 1: source file suffix
# 2: extra flags to gcc
define _recipe =
$(depdir)/%.d : %.$(1) | $$$$(@D)/
ifndef WINDOWSHELL
ifdef Echo_name
	@printf '%-20.20s\r' $$<
endif
endif
	$(.)$(cc) -MM -MF $$@ -MT $(objdir)/$$*.o $(2) $$<
endef

$(eval $(call _recipe,c))
$(eval $(call _recipe,s,$(asflags)))

# compiling recipe template
# 1: target file suffix
# 2: source file suffix
# 3: compile command
define _recipe =
$(objdir)/%.$(1) : %.$(2) | $$$$(@D)/
	$(call Echo_name,$$<)
	$(.)$(3)
endef

$(eval $(call _recipe,o,c,$(cc) -c -o $$@ $$<))
$(eval $(call _recipe,o,s,$(cc) $(asflags) -c -o $$@ $$<))
$(eval $(call _recipe,res,rc,$(windres) -i $$< -o $$@))

_rm=$(.)$(rmrf) $(call Windows_path,$(1))

cleandep :
	$(call _rm,$(depends) comptime.h)

clean :
	$(call _rm,$(exe) $(dbg) $(dbg).txt $(objects))

distclean :
	$(call _rm,../bin ../objs ../deps ../make comptime.h)

info:
ifdef WINDOWSHELL
	@REM
else
	@:
endif
