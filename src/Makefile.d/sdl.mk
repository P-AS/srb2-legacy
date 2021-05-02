#
# Makefile options for SDL2 backend.
#

#
# SDL...., *looks at Alam*, THIS IS A MESS!
#
# ...a little bird flexes its muscles...
#

makedir:=$(makedir)/SDL

sources+=$(call List,sdl/Sourcefile)
opts+=-DDIRECTFULLSCREEN -DHAVE_SDL

# FIXME
#ifdef PANDORA
#include sdl/SRB2Pandora/Makefile.cfg
#endif #ifdef PANDORA

# FIXME
#ifdef CYGWIN32
#include sdl/MakeCYG.cfg
#endif #ifdef CYGWIN32

ifndef NOHW
sources+=sdl/ogl_sdl.c
endif

ifndef NOHS
ifdef OPENAL
	OBJS+=$(OBJDIR)/s_openal.o
	OPTS+=-DSTATIC3DS
	STATICHS=1
else
ifdef FMOD
	OBJS+=$(OBJDIR)/s_fmod.o
	OPTS+=-DSTATIC3DS
	STATICHS=1
else
ifdef MINGW
ifdef DS3D
	OBJS+=$(OBJDIR)/s_ds3d.o
	OPTS+=-DSTATIC3DS
	STATICHS=1
endif
endif
endif
endif
endif

ifdef NOMIXER
sources+=sdl/sdl_sound.c
else
opts+=-DHAVE_MIXER
sources+=sdl/mixer_sound.c

  libs+=-lSDL2_mixer
endif

ifdef SDL_PKGCONFIG
$(eval $(call Use_pkg_config,SDL))
else
SDL_CONFIG?=$(call Prefix,sdl2-config)
SDL_CFLAGS?=$(shell $(SDL_CONFIG) --cflags)
SDL_LDFLAGS?=$(shell $(SDL_CONFIG) \
		$(if $(STATIC),--static-libs,--libs))
$(eval $(call Propogate_flags,SDL))
endif

# use the x86 asm code
ifndef CYGWIN32
ifndef NOASM
USEASM=1
endif
endif

# FIXME
#ifdef SDL_TTF
#	OPTS+=-DHAVE_TTF
#	SDL_LDFLAGS+=-lSDL2_ttf -lfreetype -lz
#	OBJS+=$(OBJDIR)/i_ttf.o
#endif

# FIXME
#ifdef SDL_IMAGE
#	OPTS+=-DHAVE_IMAGE
#	SDL_LDFLAGS+=-lSDL2_image
#endif

# FIXME
#ifdef SDL_NET
#	OPTS+=-DHAVE_SDLNET
#	SDL_LDFLAGS+=-lSDL2_net
#endif

ifdef MINGW
ifndef NOSDLMAIN
SDLMAIN=1
endif
endif

ifdef SDLMAIN
opts+=-DSDLMAIN
else
ifdef MINGW
opts+=-Umain
libs+=-mconsole
endif
endif
