// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  doomdef.h
/// \brief Internally used data structures for virtually everything,
///        key definitions, lots of other stuff.

#ifndef __DOOMDEF__
#define __DOOMDEF__

// Sound system select
// This should actually be in the makefile,
// but I can't stand that gibberish. D:
#define SOUND_DUMMY   0
#define SOUND_SDL     1
#define SOUND_MIXER   2
#define SOUND_FMOD    3

#ifndef SOUND
#ifdef HAVE_SDL

// Use Mixer interface?
#ifdef HAVE_MIXER
    //#if !defined(DC) && !defined(_WIN32_WCE) && !defined(_XBOX) && !defined(GP2X)
    #define SOUND SOUND_MIXER
    #define NOHS // No HW3SOUND
    #ifdef HW3SOUND
    #undef HW3SOUND
    #endif
    //#endif
#endif

// Use generic SDL interface.
#ifndef SOUND
#define SOUND SOUND_SDL
#endif

#else // No SDL.

// Use FMOD?
#ifdef HAVE_FMOD
    #define SOUND SOUND_FMOD
    #define NOHS // No HW3SOUND
    #ifdef HW3SOUND
    #undef HW3SOUND
    #endif
#else
    // No more interfaces. :(
    #define SOUND SOUND_DUMMY
#endif

#endif
#endif

#ifdef _WINDOWS
#define NONET
#if !defined (HWRENDER) && !defined (NOHW)
#define HWRENDER
#endif
// judgecutor: 3D sound support
#if !defined(HW3SOUND) && !defined (NOHS)
#define HW3SOUND
#endif
#endif

#if defined (_WIN32) || defined (_WIN32_WCE)
#define ASMCALL __cdecl
#else
#define ASMCALL
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4127 4152 4213 4514)
#ifdef _WIN64
#pragma warning(disable : 4306)
#endif
#endif
// warning level 4
// warning C4127: conditional expression is constant
// warning C4152: nonstandard extension, function/data pointer conversion in expression
// warning C4213: nonstandard extension used : cast on l-value

#if defined (_WIN32_WCE) && defined (DEBUG) && defined (ARM)
#if defined (ARMV4) || defined (ARMV4I)
//#pragma warning(disable : 1166)
// warning LNK1166: cannot adjust code at offset=
#endif
#endif


#include "doomtype.h"
#include "version.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _USE_MATH_DEFINES // fixes M_PI errors in r_plane.c for Visual Studio
#include <math.h>

#ifdef GETTEXT
#include <libintl.h>
#include <locale.h>
#endif

#if !defined (_WIN32_WCE)
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <ctype.h>

#if ((defined (_WIN32) && !defined (_WIN32_WCE)) || defined (__DJGPP__)) && !defined (_XBOX)
#include <io.h>
#endif

FILE *fopenfile(const char*, const char*);

//#define NOMD5

// Uncheck this to compile debugging code
//#define RANGECHECK
//#ifndef PARANOIA
//#define PARANOIA // do some tests that never fail but maybe
// turn this on by make etc.. DEBUGMODE = 1 or use the Debug profile in the VC++ projects
//#endif
#if defined (_WIN32) || (defined (__unix__) && !defined (MSDOS)) || defined(__APPLE__) || defined (UNIXCOMMON) || defined (macintosh)
#define LOGMESSAGES // write message in log.txt
#endif

#ifdef LOGMESSAGES
extern FILE *logstream;
extern char logfilename[1024];
#endif

//#define DEVELOP // Disable this for release builds to remove excessive cheat commands and enable MD5 checking and stuff, all in one go. :3
#ifdef DEVELOP
#define VERSIONSTRING "Development EXE"
// most interface strings are ignored in development mode.
// we use comprevision and compbranch instead.
#else
#ifdef BETAVERSION
#define VERSIONSTRING "v"SRB2VERSION" "BETAVERSION
#else
#define VERSIONSTRING "v"SRB2VERSION
#endif
// Hey! If you change this, add 1 to the MODVERSION below!
// Otherwise we can't force updates!
#endif

#define VERSIONSTRINGW WSTRING (VERSIONSTRING)

// Does this version require an added patch file?
// Comment or uncomment this as necessary.
#define USE_PATCH_DTA

// Use .kart extension addons
//#define USE_KART

// Modification options
// If you want to take advantage of the Master Server's ability to force clients to update
// to the latest version, fill these out.  Otherwise, just comment out UPDATE_ALERT and leave
// the other options the same.

// Comment out this line to completely disable update alerts (recommended for testing, but not for release)
#define UPDATE_ALERT

// If you maintain a fork of srb2-legacy, change this.
#define RELEASES "github.com/P-AS/srb2-legacy/releases\n"

// The string used in the alert that pops up in the event of an update being available.
// Please change to apply to your modification (we don't want everyone asking where your mod is on SRB2.org!).
#define UPDATE_ALERT_STRING \
"A new update is available for SRB2 Legacy.\n"\
"You can grab the latest release from:\n"\
RELEASES \
"\n"\
"You are using version: %s\n"\
"The newest version is: %s\n"\
"\n"\
"This update is required for online\n"\
"play using the Master Server.\n"\
"You will not be able to connect to\n"\
"the Master Server until you update to\n"\
"the newest version of the game.\n"\
"\n"\
"(Press a key)\n"

// The string used in the I_Error alert upon trying to host through command line parameters.
// Generally less filled with newlines, since Windows gives you lots more room to work with.
#define UPDATE_ALERT_STRING_CONSOLE \
"A new update is available for SRB2 Legacy.\n"\
"You can grab the latest release from:\n"\
RELEASES \
"\n"\
"You are using version: %s\n"\
"The newest version is: %s\n"\
"\n"\
"This update is required for online play using the Master Server.\n"\
"You will not be able to connect to the Master Server\n"\
"until you update to the newest version of the game.\n"

// For future use, the codebase is the version of SRB2 that the modification is based on,
// and should not be changed unless you have merged changes between versions of SRB2
// (such as 2.0.4 to 2.0.5, etc) into your working copy.
// Will always resemble the versionstring, 205 = 2.0.5, 210 = 2.1, etc.
#define CODEBASE 210

// To version config.cfg, MAJOREXECVERSION is set equal to MODVERSION automatically.
// Increment MINOREXECVERSION whenever a config change is needed that does not correspond
// to an increment in MODVERSION. This might never happen in practice.
// If MODVERSION increases, set MINOREXECVERSION to 0.
#define MAJOREXECVERSION MODVERSION
#define MINOREXECVERSION 0
// (It would have been nice to use VERSION and SUBVERSION but those are zero'd out for DEVELOP builds)

// Macros
#define GETMAJOREXECVERSION(v) (v & 0xFFFF)
#define GETMINOREXECVERSION(v) (v >> 16)
#define GETEXECVERSION(major,minor) (major + (minor << 16))
#define EXECVERSION GETEXECVERSION(MAJOREXECVERSION, MINOREXECVERSION)

// =========================================================================

// The maximum number of players, multiplayer/networking.
// NOTE: it needs more than this to increase the number of players...

#define MAXPLAYERS 32
#define MAXSKINS MAXPLAYERS
#define PLAYERSMASK (MAXPLAYERS-1)
#define MAXPLAYERNAME 21

#define COLORRAMPSIZE 16
#define MAXCOLORNAME 32
#define NUMCOLORFREESLOTS 1024

typedef struct skincolor_s
{
	char name[MAXCOLORNAME+1];  // Skincolor name
	UINT8 ramp[COLORRAMPSIZE];  // Colormap ramp
	UINT16 invcolor;            // Signpost color
	UINT8 invshade;             // Signpost color shade
	UINT16 chatcolor;           // Chat color
	boolean accessible;         // Accessible by the color command + setup menu
} skincolor_t;

typedef enum
{
	SKINCOLOR_NONE = 0,
	
	SKINCOLOR_WHITE,
	SKINCOLOR_SILVER,
	SKINCOLOR_GREY,
	SKINCOLOR_BLACK,
	SKINCOLOR_CYAN,
	SKINCOLOR_TEAL,
	SKINCOLOR_STEELBLUE,
	SKINCOLOR_BLUE,
	SKINCOLOR_PEACH,
	SKINCOLOR_TAN,
	SKINCOLOR_PINK,
	SKINCOLOR_LAVENDER,
	SKINCOLOR_PURPLE,
	SKINCOLOR_ORANGE,
	SKINCOLOR_ROSEWOOD,
	SKINCOLOR_BEIGE,
	SKINCOLOR_BROWN,
	SKINCOLOR_RED,
	SKINCOLOR_DARKRED,
	SKINCOLOR_NEONGREEN,
	SKINCOLOR_GREEN,
	SKINCOLOR_ZIM,
	SKINCOLOR_OLIVE,
	SKINCOLOR_YELLOW,
	SKINCOLOR_GOLD,
	
	FIRSTSUPERCOLOR,

	// Super special awesome Super flashing colors!
	SKINCOLOR_SUPER1 = FIRSTSUPERCOLOR,
	SKINCOLOR_SUPER2,
	SKINCOLOR_SUPER3,
	SKINCOLOR_SUPER4,
	SKINCOLOR_SUPER5,

	// Super Tails
	SKINCOLOR_TSUPER1,
	SKINCOLOR_TSUPER2,
	SKINCOLOR_TSUPER3,
	SKINCOLOR_TSUPER4,
	SKINCOLOR_TSUPER5,

	// Super Knuckles
	SKINCOLOR_KSUPER1,
	SKINCOLOR_KSUPER2,
	SKINCOLOR_KSUPER3,
	SKINCOLOR_KSUPER4,
	SKINCOLOR_KSUPER5,
	
	SKINCOLOR_FIRSTFREESLOT,
	SKINCOLOR_LASTFREESLOT = SKINCOLOR_FIRSTFREESLOT + NUMCOLORFREESLOTS - 1,
	
	MAXSKINCOLORS,

	NUMSUPERCOLORS = ((SKINCOLOR_FIRSTFREESLOT - FIRSTSUPERCOLOR)/5)
} skincolornum_t;

extern UINT16 numskincolors;
extern skincolor_t skincolors[MAXSKINCOLORS];

// State updates, number of tics / second.
// NOTE: used to setup the timer rate, see I_StartupTimer().
#define TICRATE 35
#define NEWTICRATERATIO 1 // try 4 for 140 fps :)
#define NEWTICRATE (TICRATE*NEWTICRATERATIO)

#define MUSICRATE 1000 // sound timing is calculated by milliseconds

#define RING_DIST 512*FRACUNIT // how close you need to be to a ring to attract it

#define PUSHACCEL (2*FRACUNIT) // Acceleration for MF2_SLIDEPUSH items.

// Special linedef executor tag numbers!
enum {
	LE_PINCHPHASE      = -2, // A boss entered pinch phase (and, in most cases, is preparing their pinch phase attack!)
	LE_ALLBOSSESDEAD   = -3, // All bosses in the map are dead (Egg capsule raise)
	LE_BOSSDEAD        = -4, // A boss in the map died (Chaos mode boss tally)
	LE_BOSS4DROP       = -5,  // CEZ boss dropped its cage
	LE_BRAKVILEATACK   = -6  // Brak's doing his LOS attack, oh noes
};

// Name of local directory for config files and savegames
#if !defined(_arch_dreamcast) && !defined(_WIN32_WCE) && !defined(GP2X) && !defined(_WII) && !defined(_PS3)
#if (((defined (__unix__) && !defined (MSDOS)) || defined (UNIXCOMMON)) && !defined (__CYGWIN__)) && !defined (__APPLE__)
#define DEFAULTDIR ".srb2_21"
#else
#define DEFAULTDIR "srb2_21"
#endif
#endif

#include "g_state.h"

// commonly used routines - moved here for include convenience

/**	\brief	The I_Error function

	\param	error	the error message

	\return	void


*/
void I_Error(const char *error, ...) FUNCIERROR;

/**	\brief	write a message to stderr (use before I_Quit) for when you need to quit with a msg, but need
 the return code 0 of I_Quit();

	\param	error	message string

	\return	void
*/
void I_OutputMsg(const char *error, ...) FUNCPRINTF;

// console.h
typedef enum
{
	CONS_NOTICE,
	CONS_WARNING,
	CONS_ERROR
} alerttype_t;

void CONS_Printf(const char *fmt, ...) FUNCPRINTF;
void CONS_Alert(alerttype_t level, const char *fmt, ...) FUNCDEBUG;
void CONS_Debug(INT32 debugflags, const char *fmt, ...) FUNCDEBUG;

// For help debugging functions.
#define POTENTIALLYUNUSED CONS_Alert(CONS_WARNING, "(%s:%d) Unused code appears to be used.\n", __FILE__, __LINE__)

#include "m_swap.h"

// Things that used to be in dstrings.h
#define SAVEGAMENAME "srb2sav"
extern char savegamename[256];

// m_misc.h
#ifdef GETTEXT
#define M_GetText(String) gettext(String)
void M_StartupLocale(void);
#else
// If no translations are to be used, make a stub
// M_GetText function that just returns the string.
#define M_GetText(x) (x)
#endif
void *M_Memcpy(void *dest, const void *src, size_t n);
char *va(const char *format, ...) FUNCPRINTF;
char *M_GetToken(const char *inputString);
char *sizeu1(size_t num);
char *sizeu2(size_t num);
char *sizeu3(size_t num);
char *sizeu4(size_t num);
char *sizeu5(size_t num);

// d_main.c
extern int    VERSION;
extern int SUBVERSION;

extern boolean devparm; // development mode (-debug)
// d_netcmd.c
extern INT32 cv_debug;

#define DBG_BASIC       0x0001
#define DBG_DETAILED    0x0002
#define DBG_RANDOMIZER  0x0004
#define DBG_RENDER      0x0008
#define DBG_NIGHTSBASIC 0x0010
#define DBG_NIGHTS      0x0020
#define DBG_POLYOBJ     0x0040
#define DBG_GAMELOGIC   0x0080
#define DBG_NETPLAY     0x0100
#define DBG_MEMORY      0x0200
#define DBG_SETUP       0x0400
#define DBG_LUA         0x0800
#define DBG_VIEWMORPH   0x2000


// =======================
// Misc stuff for later...
// =======================

#define ANG2RAD(angle) ((float)((angle)*M_PI)/ANGLE_180)

// Modifier key variables, accessible anywhere
extern UINT8 shiftdown, ctrldown, altdown;
extern boolean capslock;

// if we ever make our alloc stuff...
#define ZZ_Alloc(x) Z_Malloc(x, PU_STATIC, NULL)

// i_system.c, replace getchar() once the keyboard has been appropriated
INT32 I_GetKey(void);

#ifndef min // Double-Check with WATTCP-32's cdefs.h
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef max // Double-Check with WATTCP-32's cdefs.h
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

// Floating point comparison epsilons from float.h
#ifndef FLT_EPSILON
#define FLT_EPSILON 1.1920928955078125e-7f
#endif

#ifndef DBL_EPSILON
#define DBL_EPSILON 2.2204460492503131e-16
#endif

// An assert-type mechanism.
#ifdef PARANOIA
#define I_Assert(e) ((e) ? (void)0 : I_Error("assert failed: %s, file %s, line %d", #e, __FILE__, __LINE__))
#else
#define I_Assert(e) ((void)0)
#endif

// The character that separates pathnames. Forward slash on
// most systems, but reverse solidus (\) on Windows.
#if defined (_WIN32)
	#define PATHSEP "\\"
#else
	#define PATHSEP "/"
#endif

// Compile date and time and revision.
extern const char *compdate, *comptime, *comprevision, *compbranch, *compnote;

// Disabled code and code under testing
// None of these that are disabled in the normal build are guaranteed to work perfectly
// Compile them at your own risk!

/// Kalaron/Eternity Engine slope code (SRB2CB ported)
#define ESLOPE


/// Backwards compatibility with SRB2CB's slope linedef types.
///	\note	A simple shim that prints a warning.
#define ESLOPE_TYPESHIM


///	Delete file while the game is running.
///	\note	EXTREMELY buggy, tends to crash game.
//#define DELFILE

///	Allows the use of devmode in multiplayer. AKA "fishcake"
//#define NETGAME_DEVMODE

///	Allows gravity changes in netgames, no questions asked.
//#define NETGAME_GRAVITY

///	Dumps the contents of a network save game upon consistency failure for debugging.
//#define DUMPCONSISTENCY

///	Polyobject fake flat code
#define POLYOBJECTS_PLANES

///	See name of player in your crosshair
#define SEENAMES

///	Who put weights on my recycler?  ... Inuyasha did.
///	\note	XMOD port.
//#define WEIGHTEDRECYCLER

///	Allow loading of savegames between different versions of the game.
///	\note	XMOD port.
///	    	Most modifications should probably enable this.
//#define SAVEGAME_OTHERVERSIONS

#if !defined (_NDS) && !defined (_PSP)
///	Shuffle's incomplete OpenGL sorting code.
#define SHUFFLE // This has nothing to do with sorting, why was it disabled?
#endif

#if !defined (_NDS) && !defined (_PSP)
///	Allow the use of the SOC RESETINFO command.
///	\note	Builds that are tight on memory should disable this.
///	    	This stops the game from storing backups of the states, sprites, and mobjinfo tables.
///	    	Though this info is compressed under normal circumstances, it's still a lot of extra
///	    	memory that never gets touched.
#define ALLOW_RESETDATA
#endif

#ifndef NONET
///	Display a connection screen on join attempts.
#define CLIENT_LOADINGSCREEN
#endif

/// Experimental tweaks to analog mode. (Needs a lot of work before it's ready for primetime.)
//#define REDSANALOG

/// Backwards compatibility with musicslots.
/// \note	You should leave this enabled unless you're working with a future SRB2 version.
#define MUSICSLOT_COMPATIBILITY

/// Handle touching sector specials in P_PlayerAfterThink instead of P_PlayerThink.
/// \note   Required for proper collision with moving sloped surfaces that have sector specials on them.
//#define SECTORSPECIALSAFTERTHINK


/// FINALLY some real clipping that doesn't make walls dissappear AND speeds the game up
/// (that was the original comment from SRB2CB, sadly it is a lie and actually slows game down)
/// on the bright side it fixes some weird issues with translucent walls
/// \note	SRB2CB port.
///      	SRB2CB itself ported this from PrBoom+
#define NEWCLIP

/// Cache patches in Lua in a way that renderer switching will work flawlessly.
//#define LUA_PATCH_SAFETY

/// OpenGL shaders
#define GL_SHADERS

// Here once lied the corpse of ALAM_LIGHTING

#endif // __DOOMDEF__
