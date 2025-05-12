// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2022 by Jaime Ita Passos.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  SDL_android_main.c
/// \brief Android entry point

#include "SDL.h"
#include "SDL_main.h"
#include "SDL_config.h"

#include "../sdlmain.h"

#include "../../doomdef.h"
#include "../../d_main.h"
#include "../../m_argv.h"
#include "../../m_misc.h"/* path shit */
#include "../../i_system.h"

#ifdef SPLASH_SCREEN
#include "../../i_video.h"
#include "../ogl_sdl.h"
#endif

#include "../../android-jni/jni_android.h"

#include "time.h" // For log timestamps

#ifdef LOGMESSAGES
FILE *logstream = NULL;
char logfilename[1024];
#endif

#ifdef SPLASH_SCREEN
static SDL_bool displayingSplash = SDL_FALSE;

static void BlitSplashScreen(void)
{
	Impl_PumpEvents();
	Impl_PresentSplashScreen();
}

static void ShowSplashScreen(void)
{
	displayingSplash = Impl_LoadSplashScreen();

	if (displayingSplash == SDL_TRUE)
	{
		// Present it for two seconds.
		UINT32 delay = SDL_GetTicks() + 2000;

		while (SDL_GetTicks() < delay)
			BlitSplashScreen();
	}
}
#endif

#define REQUEST_STORAGE_PERMISSION

#define REQUEST_MESSAGE_TITLE "Permission required"
#define REQUEST_MESSAGE_TEXT "Sonic Robo Blast 2 Legacy needs storage permission.\nYour settings and game progress will not be saved if you decline."

static void PermissionRequestMessage(void)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, REQUEST_MESSAGE_TITLE, REQUEST_MESSAGE_TEXT, NULL);
}

static boolean StorageInit(void)
{
	JNI_SharedStorage = I_SharedStorageLocation();
	return (JNI_SharedStorage != NULL);
}

static void StorageGrantedPermission(void)
{
	I_mkdir(JNI_SharedStorage, 0755);
	JNI_StoragePermission = true;
}

static boolean StorageCheckPermission(void)
{
	// Permission was already granted. Create the directory anyway.
	if (JNI_CheckStoragePermission())
	{
		StorageGrantedPermission();
		return true;
	}

	PermissionRequestMessage();

	// Permission granted. Create the directory.
	if (I_RequestSystemPermission(JNI_GetWriteExternalStoragePermission()))
	{
		StorageGrantedPermission();
		return true;
	}

	return false;
}

/** Return the number of parts of this path.
*/
int M_PathParts(const char *path) {
    int n;
    const char *p;
    const char *t;
    if (path == NULL)
        return 0;
    for (n = 0, p = path;; ++n) {
        t = p;
        if ((p = strchr(p, PATHSEP[0])))
            p += strspn(p, PATHSEP);
        else {
            if (*t)/* there is something after the final delimiter */
                n++;
            break;
        }
    }
    return n;
}

/** Check whether a path is an absolute path.
*/
boolean M_IsPathAbsolute(const char *path)
{
#ifdef _WIN32
    return ( strncmp(&path[1], ":\\", 2) == 0 );
#else
    return ( path[0] == '/' );
#endif
}

/** I_mkdir for each part of the path.
*/
void M_MkdirEachUntil(const char *cpath, int start, int end, int mode)
{
    char path[MAX_WADPATH];
    char *p;
    char *t;

    if (end > 0 && end <= start)
        return;

    strlcpy(path, cpath, sizeof path);
#ifdef _WIN32
    if (strncmp(&path[1], ":\\", 2) == 0)
		p = &path[3];
	else
#endif
    p = path;

    if (end > 0)
        end -= start;

    for (; start > 0; --start)
    {
        p += strspn(p, PATHSEP);
        if (!( p = strchr(p, PATHSEP[0]) ))
            return;
    }
    p += strspn(p, PATHSEP);
    for (;;)
    {
        if (end > 0 && !--end)
            break;

        t = p;
        if (( p = strchr(p, PATHSEP[0]) ))
        {
            *p = '\0';
            I_mkdir(path, mode);
            *p = PATHSEP[0];
            p += strspn(p, PATHSEP);
        }
        else
        {
            if (*t)
                I_mkdir(path, mode);
            break;
        }
    }
}

#ifdef LOGMESSAGES
void I_InitLogging(void)
{
	const char *logdir = NULL;
	time_t my_time;
	struct tm * timeinfo;
	const char *format;
	const char *reldir;
	int left;
	boolean fileabs;
#ifdef LOGSYMLINK
	const char *link;
#endif

	logdir = D_Home();

	my_time = time(NULL);
	timeinfo = localtime(&my_time);

	if (M_CheckParm("-logfile") && M_IsNextParm())
	{
		format = M_GetNextParm();
		fileabs = M_IsPathAbsolute(format);
	}
	else
	{
		format = "log-%Y-%m-%d_%H-%M-%S.txt";
		fileabs = false;
	}

	if (fileabs)
	{
		strftime(logfilename, sizeof logfilename, format, timeinfo);
	}
	else
	{
		if (M_CheckParm("-logdir") && M_IsNextParm())
			reldir = M_GetNextParm();
		else
			reldir = "logs";

		if (M_IsPathAbsolute(reldir))
		{
			left = snprintf(logfilename, sizeof logfilename,
					"%s"PATHSEP, reldir);
		}
		else
#if defined(__ANDROID__)
		if (logdir)
		{
			left = snprintf(logfilename, sizeof logfilename,
					"%s"PATHSEP "%s"PATHSEP, logdir, reldir);
		}
		else
#elif defined(DEFAULTDIR)
		if (logdir)
		{
			left = snprintf(logfilename, sizeof logfilename,
					"%s"PATHSEP DEFAULTDIR PATHSEP"%s"PATHSEP, logdir, reldir);
		}
		else
#endif
		{
			left = snprintf(logfilename, sizeof logfilename,
					"."PATHSEP"%s"PATHSEP, reldir);
		}

		strftime(&logfilename[left], sizeof logfilename - left,
				format, timeinfo);
	}

	M_MkdirEachUntil(logfilename,
			M_PathParts(logdir) - 1,
			M_PathParts(logfilename) - 1, 0755);

#ifdef LOGSYMLINK
	logstream = fopen(logfilename, "w");
#ifdef DEFAULTDIR
	if (logdir)
		link = va("%s/"DEFAULTDIR"/latest-log.txt", logdir);
	else
#endif/*DEFAULTDIR*/
		link = "latest-log.txt";
	unlink(link);
	if (symlink(logfilename, link) == -1)
	{
		I_OutputMsg("Error symlinking latest-log.txt: %s\n", strerror(errno));
	}
#elif defined(__ANDROID__)
	logstream = fopen(va("%s/latest-log.txt", I_SharedStorageLocation()), "wt+");
#else/*LOGSYMLINK*/
	logstream = fopen("latest-log.txt", "wt+");
#endif
}
#else
void I_InitLogging(void) {}
#endif
int main(int argc, char* argv[])
{
#ifdef LOGMESSAGES
	boolean logging = !M_CheckParm("-nolog");
#endif

	myargc = argc;
	myargv = argv;

	// Obtain the activity class before doing anything else.
	JNI_Startup();

	// Start up the main system.
	I_OutputMsg("I_StartupSystem()...\n");
	I_StartupSystem();

#ifdef SPLASH_SCREEN
	// Load the splash screen, and display it.
	ShowSplashScreen();
#endif

	// Init shared storage.
	if (StorageInit())
		StorageCheckPermission(); // Check storage permissions.

#ifdef LOGMESSAGES
	// Start logging.
	if (logging && I_StoragePermission())
		I_InitLogging();
#endif

	CONS_Printf("Sonic Robo Blast 2 Legacy for Android\n");

#ifdef LOGMESSAGES
	if (logstream)
		CONS_Printf("Logfile: %s\n", logfilename);
#endif

#ifdef SPLASH_SCREEN
	if (displayingSplash == SDL_TRUE)
		BlitSplashScreen();
#endif

	// Begin the normal game setup and loop.
	CONS_Printf("Setting up Sonic Robo Blast 2 Legacy...\n");
	D_SRB2Main();

	CONS_Printf("Entering main game loop...\n");
	D_SRB2Loop();

	return 0;
}
