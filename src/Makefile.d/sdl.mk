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

ifdef MACOS
sources+=$(call List,sdl/macosx/Sourcefile)
endif

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

ifndef NOTHREADS
opts+=-DHAVE_THREADS
sources+=i_threads.c
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
