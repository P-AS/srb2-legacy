// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------
/// \file
/// \brief SDL specific part of the OpenGL API for SRB2

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#ifdef HAVE_SDL
#define _MATH_DEFINES_DEFINED

#include "SDL.h"

#include "sdlmain.h"

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#include "../doomdef.h"
#include "../d_main.h"

#ifdef HWRENDER
#include "../hardware/r_opengl/r_opengl.h"
#include "../hardware/hw_main.h"
#include "ogl_sdl.h"
#include "../i_system.h"
#include "hwsym_sdl.h"
#include "../m_argv.h"

#ifdef DEBUG_TO_FILE
#include <stdarg.h>
#if defined (_WIN32) && !defined (__CYGWIN__)
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef USE_WGL_SWAP
PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT = NULL;
#else
typedef int (*PFNGLXSWAPINTERVALPROC) (int);
PFNGLXSWAPINTERVALPROC glXSwapIntervalSGIEXT = NULL;
#endif

#ifndef STATIC_OPENGL
PFNglClear pglClear;
PFNglGetIntegerv pglGetIntegerv;
PFNglGetString pglGetString;
#endif

/**	\brief SDL video display surface
*/
INT32 oglflags = 0;
SDL_GLContext sdlglcontext = 0;

void *GetGLFunc(const char *proc)
{
	return SDL_GL_GetProcAddress(proc);
}

boolean LoadGL(void)
{
#ifndef STATIC_OPENGL
	const char *OGLLibname = NULL;

	if (M_CheckParm ("-OGLlib") && M_IsNextParm())
		OGLLibname = M_GetNextParm();

	if (SDL_GL_LoadLibrary(OGLLibname) != 0)
	{
		CONS_Alert(CONS_ERROR, "Could not load OpenGL Library: %s\n"
					"Falling back to Software mode.\n", SDL_GetError());
		if (!M_CheckParm ("-OGLlib"))
			CONS_Alert(CONS_ERROR, "If you know what is the OpenGL library's name, use -OGLlib\n");
		return 0;
	}
#endif
	return SetupGLfunc();
}

/**	\brief	The OglSdlSurface function

	\param	w	width
	\param	h	height
	\param	isFullscreen	if true, go fullscreen

	\return	if true, changed video mode
*/
boolean OglSdlSurface(INT32 w, INT32 h)
{
	INT32 cbpp = cv_scr_depth.value < 16 ? 16 : cv_scr_depth.value;
	static int majorGL = 0, minorGL = 0;
	static boolean first_init = false;
	const char *gllogdir = NULL;


	if (!gllogstream)
	{
		gllogdir = D_Home();

#ifdef DEBUG_TO_FILE
#ifdef DEFAULTDIR
		if (gllogdir)
				gllogstream = fopen(va("%s/"DEFAULTDIR"/ogllog.txt",gllogdir), "wt");
		else
#endif
			gllogstream = fopen("./ogllog.txt", "wt");
#endif
	}

	if (!first_init)
	{
		// Get info and extensions.
		//BP: why don't we make it earlier ?
		//Hurdler: we cannot do that before intialising gl context
		gl_renderer = pglGetString(GL_RENDERER);
		gl_version = pglGetString(GL_VERSION);
		gl_extensions = pglGetString(GL_EXTENSIONS);

		GL_DBG_Printf("Renderer   : %s\n", gl_renderer);
		GL_DBG_Printf("Version    : %s\n", gl_version);
		GL_DBG_Printf("Extensions : %s\n", gl_extensions);
		oglflags = 0;

		if (strcmp((const char*)gl_renderer, "GDI Generic") == 0 &&
			strcmp((const char*)gl_version, "1.1.0") == 0)
		{
			// Oh no... Windows gave us the GDI Generic rasterizer, so something is wrong...
			// The game will crash later on when unsupported OpenGL commands are encountered.
			// Instead of a nondescript crash, show a more informative error message.
			// Also set the renderer variable back to software so the next launch won't
			// repeat this error.
			CV_StealthSet(&cv_renderer, "Software");
			I_Error("OpenGL Error: Failed to access the GPU. Possible reasons include:\n"
					"- GPU vendor has dropped OpenGL support on your GPU and OS. (Old GPU?)\n"
					"- GPU drivers are missing or broken. You may need to update your drivers.");
		}
	}
	first_init = true;

	if (isExtAvailable("GL_EXT_texture_filter_anisotropic", gl_extensions))
		pglGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
	else
		maximumAnisotropy = 1;

	if (sscanf((const char*)gl_version, "%d.%d", &majorGL, &minorGL)
		&& (!(majorGL == 1 && minorGL <= 3)))
		supportMipMap = true;
	else
		supportMipMap = false;

	SetupGLFunc4();

	glanisotropicmode_cons_t[1].value = maximumAnisotropy;

	SDL_GL_SetSwapInterval(cv_vidwait.value ? 1 : 0);

	SetModelView(w, h);
	SetStates();
	pglClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	HWR_Startup();
	textureformatGL = cbpp > 16 ? GL_RGBA : GL_RGB5_A1;

	return true;
}

/**	\brief	The OglSdlFinishUpdate function

	\param	vidwait	wait for video sync

	\return	void
*/
void OglSdlFinishUpdate(boolean waitvbl)
{
	static boolean oldwaitvbl = false;
	int sdlw, sdlh;
	if (oldwaitvbl != waitvbl)
	{
		SDL_GL_SetSwapInterval(waitvbl ? 1 : 0);
	}

	oldwaitvbl = waitvbl;

	SDL_GetWindowSize(window, &sdlw, &sdlh);

	HWR_MakeScreenFinalTexture();
	HWR_DrawScreenFinalTexture(sdlw, sdlh);
	SDL_GL_SwapWindow(window);

	GClipRect(0, 0, realwidth, realheight, NZCLIP_PLANE);

	// Sryder:	We need to draw the final screen texture again into the other buffer in the original position so that
	//			effects that want to take the old screen can do so after this
	// Generic2 has the screen image without palette rendering brightness adjustments.
	// Using that here will prevent brightness adjustments being applied twice.
	DrawScreenTexture(HWD_SCREENTEXTURE_GENERIC2, NULL, 0);
}

EXPORT void HWRAPI(OglSdlSetPalette) (RGBA_t *palette)
{
	size_t palsize = (sizeof(RGBA_t) * 256);
	// on a palette change, you have to reload all of the textures
	if (memcmp(&myPaletteData, palette, palsize))
	{
		memcpy(&myPaletteData, palette, palsize);
		Flush();
	}
}

#endif //HWRENDER
#endif //SDL
