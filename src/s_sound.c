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
/// \file  s_sound.c
/// \brief System-independent sound and music routines

#ifdef MUSSERV
#include <sys/msg.h>
struct musmsg
{
	long msg_type;
	char msg_text[12];
};
extern INT32 msg_id;
#endif

#include "doomdef.h"
#include "doomstat.h"
#include "command.h"
#include "g_game.h"
#include "m_argv.h"
#include "r_main.h" // R_PointToAngle2() used to calc stereo sep.
#include "r_things.h" // for skins
#include "i_system.h"
#include "i_sound.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "d_main.h"
#include "r_sky.h" // skyflatnum
#include "p_local.h" // camera info
#include "m_misc.h" // for tunes command

#if defined(HAVE_LUA_MUSICPLUS)
#include "lua_hook.h" // MusicChange hook
#endif

#ifdef HW3SOUND
// 3D Sound Interface
#include "hardware/hw3sound.h"
#else
static INT32 S_AdjustSoundParams(const mobj_t *listener, const mobj_t *source, INT32 *vol, INT32 *sep, INT32 *pitch, sfxinfo_t *sfxinfo);
#endif

CV_PossibleValue_t soundvolume_cons_t[] = {{0, "MIN"}, {31, "MAX"}, {0, NULL}};
static void SetChannelsNum(void);
static void Command_Tunes_f(void);
static void Command_RestartAudio_f(void);

// Sound system toggles
static void GameMIDIMusic_OnChange(void);
static void GameSounds_OnChange(void);
static void GameDigiMusic_OnChange(void);
static void MusicPref_OnChange(void);

#ifdef HAVE_OPENMPT
static void ModFilter_OnChange(void);
#endif

// commands for music and sound servers
#ifdef MUSSERV
consvar_t musserver_cmd = CVAR_INIT ("musserver_cmd", "musserver", NULL, CV_SAVE, NULL, NULL);
consvar_t musserver_arg = CVAR_INIT ("musserver_arg", "-t 20 -f -u 0 -i music.dta", NULL, CV_SAVE, NULL, NULL);
#endif
#ifdef SNDSERV
consvar_t sndserver_cmd = CVAR_INIT ("sndserver_cmd", "llsndserv", NULL, CV_SAVE, NULL);
consvar_t sndserver_arg = CVAR_INIT ("sndserver_arg", "-quiet", NULL, CV_SAVE, NULL);
#endif

#if defined (_WINDOWS) && !defined (SURROUND) //&& defined (_X86_)
#define SURROUND
#endif

#ifdef _WINDOWS
consvar_t cv_samplerate = CVAR_INIT ("samplerate", "44100", NULL, 0, CV_Unsigned, NULL); //Alam: For easy hacking?
#else
consvar_t cv_samplerate = CVAR_INIT ("samplerate", "22050", NULL, 0, CV_Unsigned, NULL); //Alam: For easy hacking?
#endif

// stereo reverse
consvar_t stereoreverse = CVAR_INIT ("stereoreverse", "Off", NULL, CV_SAVE, CV_OnOff, NULL);

// if true, all sounds are loaded at game startup
static consvar_t precachesound = CVAR_INIT ("precachesound", "Off", NULL, CV_SAVE, CV_OnOff, NULL);

// actual general (maximum) sound & music volume, saved into the config
consvar_t cv_soundvolume = CVAR_INIT ("soundvolume", "18", "Volume of sound effects", CV_SAVE, soundvolume_cons_t, NULL);
consvar_t cv_digmusicvolume = CVAR_INIT ("digmusicvolume", "18", "Volume of digital music", CV_SAVE, soundvolume_cons_t, NULL);
consvar_t cv_midimusicvolume = CVAR_INIT ("midimusicvolume", "18", "Volume of MIDI music", CV_SAVE, soundvolume_cons_t, NULL);
// number of channels available
#if defined (_WIN32_WCE) || defined (DC) || defined (PSP) || defined(GP2X)
consvar_t cv_numChannels = CVAR_INIT ("snd_channels", "8", NULL, CV_SAVE|CV_CALL, CV_Unsigned, SetChannelsNum);
#else
consvar_t cv_numChannels = CVAR_INIT ("snd_channels", "32", NULL, CV_SAVE|CV_CALL, CV_Unsigned, SetChannelsNum);
#endif

static consvar_t surround = CVAR_INIT ("surround", "Off", NULL, CV_SAVE, CV_OnOff, NULL);
consvar_t cv_resetmusic = CVAR_INIT ("resetmusic", "No", NULL, CV_SAVE, CV_YesNo, NULL);

// Sound system toggles, saved into the config
consvar_t cv_gamedigimusic = CVAR_INIT ("digimusic", "On", "Enable or disable digital music", CV_SAVE|CV_CALL|CV_NOINIT, CV_OnOff, GameDigiMusic_OnChange);
consvar_t cv_gamemidimusic = CVAR_INIT ("midimusic", "On", "Enable or disable MIDI music", CV_SAVE|CV_CALL|CV_NOINIT, CV_OnOff, GameMIDIMusic_OnChange);
consvar_t cv_gamesounds = CVAR_INIT ("sounds", "On", "Enable or disable sound effects", CV_SAVE|CV_CALL|CV_NOINIT, CV_OnOff, GameSounds_OnChange);

// Music preference
static CV_PossibleValue_t cons_musicpref_t[] = {
	{0, "Digital"},
	{1, "MIDI"},
	{0, NULL}
};
consvar_t cv_musicpref = CVAR_INIT ("musicpref", "Digital", "Preferred type of music to use, if available", CV_SAVE|CV_CALL|CV_NOINIT, cons_musicpref_t, MusicPref_OnChange);

// Window focus sound sytem toggles
consvar_t cv_playmusicifunfocused = CVAR_INIT ("playmusicifunfocused", "No", "Play music if game window is not focused", CV_SAVE, CV_YesNo, NULL);
consvar_t cv_playsoundsifunfocused = CVAR_INIT ("playsoundsifunfocused", "No", "Play sound effects if game window is not focused", CV_SAVE, CV_YesNo, NULL);

#ifdef HAVE_OPENMPT
openmpt_module *openmpt_mhandle = NULL;
static CV_PossibleValue_t interpolationfilter_cons_t[] = {{0, "Default"}, {1, "None"}, {2, "Linear"}, {4, "Cubic"}, {8, "Windowed sinc"}, {0, NULL}};
consvar_t cv_modfilter = CVAR_INIT ("modfilter", "0", "Filter to use for module music", CV_SAVE|CV_CALL, interpolationfilter_cons_t, ModFilter_OnChange);
#endif

#define S_MAX_VOLUME 127

// when to clip out sounds
// Does not fit the large outdoor areas.
// added 2-2-98 in 8 bit volume control (before (1200*0x10000))
#define S_CLIPPING_DIST (1536*0x10000)

// Distance to origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// Originally: (200*0x10000).
// added 2-2-98 in 8 bit volume control (before (160*0x10000))
#define S_CLOSE_DIST (160*0x10000)

// added 2-2-98 in 8 bit volume control (before remove the +4)
#define S_ATTENUATOR ((S_CLIPPING_DIST-S_CLOSE_DIST)>>(FRACBITS+4))

// Adjustable by menu.
#define NORM_VOLUME snd_MaxVolume

#define NORM_PITCH 128
#define NORM_PRIORITY 64
#define NORM_SEP 128

#define S_PITCH_PERTURB 1
#define S_STEREO_SWING (96*0x10000)

#ifdef SURROUND
#define SURROUND_SEP -128
#endif

// percent attenuation from front to back
#define S_IFRACVOL 30

typedef struct
{
	// sound information (if null, channel avail.)
	sfxinfo_t *sfxinfo;

	// origin of sound
	const void *origin;

	// handle of the sound being played
	INT32 handle;

} channel_t;

// the set of channels available
static channel_t *channels = NULL;
static INT32 numofchannels = 0;

//
// Internals.
//
static void S_StopChannel(INT32 cnum);

//
// S_getChannel
//
// If none available, return -1. Otherwise channel #.
//
static INT32 S_getChannel(const void *origin, sfxinfo_t *sfxinfo)
{
	// channel number to use
	INT32 cnum;

	channel_t *c;

	// Find an open channel
	for (cnum = 0; cnum < numofchannels; cnum++)
	{
		if (!channels[cnum].sfxinfo)
			break;

		// Now checks if same sound is being played, rather
		// than just one sound per mobj
		else if (sfxinfo == channels[cnum].sfxinfo && (sfxinfo->pitch & SF_NOMULTIPLESOUND))
		{
			return -1;
			break;
		}
		else if (sfxinfo == channels[cnum].sfxinfo && sfxinfo->singularity == true)
		{
			S_StopChannel(cnum);
			break;
		}
		else if (origin && channels[cnum].origin == origin && channels[cnum].sfxinfo == sfxinfo)
		{
			if (sfxinfo->pitch & SF_NOINTERRUPT)
				return -1;
			else
				S_StopChannel(cnum);
			break;
		}
		else if (origin && channels[cnum].origin == origin
			&& channels[cnum].sfxinfo->name != sfxinfo->name
			&& channels[cnum].sfxinfo->pitch == SF_TOTALLYSINGLE && sfxinfo->pitch == SF_TOTALLYSINGLE)
		{
			S_StopChannel(cnum);
			break;
		}
	}

	// None available
	if (cnum == numofchannels)
	{
		// Look for lower priority
		for (cnum = 0; cnum < numofchannels; cnum++)
			if (channels[cnum].sfxinfo->priority <= sfxinfo->priority)
				break;

		if (cnum == numofchannels)
		{
			// No lower priority. Sorry, Charlie.
			return -1;
		}
		else
		{
			// Otherwise, kick out lower priority.
			S_StopChannel(cnum);
		}
	}

	c = &channels[cnum];

	// channel is decided to be cnum.
	c->sfxinfo = sfxinfo;
	c->origin = origin;

	return cnum;
}

void S_RegisterSoundStuff(void)
{
	if (dedicated)
	{
		sound_disabled = true;
		return;
	}

	CV_RegisterVar(&stereoreverse);
	CV_RegisterVar(&precachesound);

#ifdef SNDSERV
	CV_RegisterVar(&sndserver_cmd);
	CV_RegisterVar(&sndserver_arg);
#endif
#ifdef MUSSERV
	CV_RegisterVar(&musserver_cmd);
	CV_RegisterVar(&musserver_arg);
#endif
	CV_RegisterVar(&surround);
	CV_RegisterVar(&cv_samplerate);
	CV_RegisterVar(&cv_resetmusic);
	CV_RegisterVar(&cv_playsoundsifunfocused);
	CV_RegisterVar(&cv_playmusicifunfocused);
	CV_RegisterVar(&cv_gamesounds);
	CV_RegisterVar(&cv_gamedigimusic);
	CV_RegisterVar(&cv_gamemidimusic);
	CV_RegisterVar(&cv_musicpref);
#ifdef HAVE_OPENMPT
	CV_RegisterVar(&cv_modfilter);
#endif

	COM_AddCommand("tunes", NULL, Command_Tunes_f);
	COM_AddCommand("restartaudio", NULL, Command_RestartAudio_f);

#if defined (macintosh) && !defined (HAVE_SDL) // mp3 playlist stuff
	{
		INT32 i;
		for (i = 0; i < PLAYLIST_LENGTH; i++)
		{
			user_songs[i].name = malloc(7);
			if (!user_songs[i].name)
				I_Error("No more free memory for mp3 playlist");
			sprintf(user_songs[i].name, "song%d%d",i/10,i%10);
			user_songs[i].defaultvalue = malloc(sizeof (char));
			if (user_songs[i].defaultvalue)
				I_Error("No more free memory for blank mp3 playerlist");
			*user_songs[i].defaultvalue = 0;
			user_songs[i].flags = CV_SAVE;
			user_songs[i].PossibleValue = NULL;
			CV_RegisterVar(&user_songs[i]);
		}
		CV_RegisterVar(&play_mode);
	}
#endif
}

static void SetChannelsNum(void)
{
	INT32 i;

	// Allocating the internal channels for mixing
	// (the maximum number of sounds rendered
	// simultaneously) within zone memory.
	if (channels)
		S_StopSounds();

	Z_Free(channels);
	channels = NULL;


	if (cv_numChannels.value == 999999999) //Alam_GBC: OH MY ROD!(ROD rimmiced with GOD!)
		CV_StealthSet(&cv_numChannels,cv_numChannels.defaultvalue);

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_SetSourcesNum();
		return;
	}
#endif
	if (cv_numChannels.value)
		channels = (channel_t *)Z_Malloc(cv_numChannels.value * sizeof (channel_t), PU_STATIC, NULL);
	numofchannels = cv_numChannels.value;

	// Free all channels for use
	for (i = 0; i < numofchannels; i++)
		channels[i].sfxinfo = 0;
}


// Retrieve the lump number of sfx
//
lumpnum_t S_GetSfxLumpNum(sfxinfo_t *sfx)
{
	char namebuf[9];
	lumpnum_t sfxlump;

	sprintf(namebuf, "ds%s", sfx->name);

	sfxlump = W_CheckNumForName(namebuf);
	if (sfxlump != LUMPERROR)
		return sfxlump;

	strlcpy(namebuf, sfx->name, sizeof namebuf);

	sfxlump = W_CheckNumForName(namebuf);
	if (sfxlump != LUMPERROR)
		return sfxlump;

	return W_GetNumForName("dsthok");
}

//
// Sound Status
//

boolean S_SoundDisabled(void)
{
	return (
			sound_disabled ||
			( window_notinfocus && ! cv_playsoundsifunfocused.value )
	);
}

// Stop all sounds, load level info, THEN start sounds.
void S_StopSounds(void)
{
	INT32 cnum;

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_StopSounds();
		return;
	}
#endif

	// kill all playing sounds at start of level
	for (cnum = 0; cnum < numofchannels; cnum++)
		if (channels[cnum].sfxinfo)
			S_StopChannel(cnum);
}

void S_StopSoundByID(void *origin, sfxenum_t sfx_id)
{
	INT32 cnum;

	// Sounds without origin can have multiple sources, they shouldn't
	// be stopped by new sounds.
	if (!origin)
		return;
#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_StopSoundByID(origin, sfx_id);
		return;
	}
#endif
	for (cnum = 0; cnum < numofchannels; cnum++)
	{
		if (channels[cnum].sfxinfo == &S_sfx[sfx_id] && channels[cnum].origin == origin)
		{
			S_StopChannel(cnum);
			break;
		}
	}
}

void S_StopSoundByNum(sfxenum_t sfxnum)
{
	INT32 cnum;

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_StopSoundByNum(sfxnum);
		return;
	}
#endif
	for (cnum = 0; cnum < numofchannels; cnum++)
	{
		if (channels[cnum].sfxinfo == &S_sfx[sfxnum])
		{
			S_StopChannel(cnum);
			break;
		}
	}
}

void S_StartSoundAtVolume(const void *origin_p, sfxenum_t sfx_id, INT32 volume)
{
	INT32 sep, pitch, priority, cnum;
	sfxinfo_t *sfx;

	const mobj_t *origin = (const mobj_t *)origin_p;

	listener_t listener  = {0,0,0,0};
	listener_t listener2 = {0,0,0,0};

	mobj_t *listenmobj = players[displayplayer].mo;
	mobj_t *listenmobj2 = NULL;

	if (S_SoundDisabled() || !sound_started)
		return;

	// Don't want a sound? Okay then...
	if (sfx_id == sfx_None)
		return;

	if (players[displayplayer].awayviewtics)
		listenmobj = players[displayplayer].awayviewmobj;

	if (splitscreen)
	{
		listenmobj2 = players[secondarydisplayplayer].mo;
		if (players[secondarydisplayplayer].awayviewtics)
			listenmobj2 = players[secondarydisplayplayer].awayviewmobj;
	}

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_StartSound(origin, sfx_id);
		return;
	};
#endif

	if (camera.chase && !players[displayplayer].awayviewtics)
	{
		listener.x = camera.x;
		listener.y = camera.y;
		listener.z = camera.z;
		listener.angle = camera.angle;
	}
	else if (listenmobj)
	{
		listener.x = listenmobj->x;
		listener.y = listenmobj->y;
		listener.z = listenmobj->z;
		listener.angle = listenmobj->angle;
	}
	else if (origin)
		return;

	if (listenmobj2)
	{
		if (camera2.chase && !players[secondarydisplayplayer].awayviewtics)
		{
			listener2.x = camera2.x;
			listener2.y = camera2.y;
			listener2.z = camera2.z;
			listener2.angle = camera2.angle;
		}
		else
		{
			listener2.x = listenmobj2->x;
			listener2.y = listenmobj2->y;
			listener2.z = listenmobj2->z;
			listener2.angle = listenmobj2->angle;
		}
	}

	// check for bogus sound #
	I_Assert(sfx_id >= 1);
	I_Assert(sfx_id < NUMSFX);

	sfx = &S_sfx[sfx_id];

	if (sfx->skinsound != -1 && origin && origin->skin)
	{
		// redirect player sound to the sound in the skin table
		sfx_id = ((skin_t *)origin->skin)->soundsid[sfx->skinsound];
		sfx = &S_sfx[sfx_id];
	}

	// Initialize sound parameters
	pitch = NORM_PITCH;
	priority = NORM_PRIORITY;

	if (splitscreen && listenmobj2) // Copy the sound for the split player
	{
		// Check to see if it is audible, and if not, modify the params
		if (origin && origin != listenmobj2)
		{
			INT32 rc;
			rc = S_AdjustSoundParams(listenmobj2, origin, &volume, &sep, &pitch, sfx);

			if (!rc)
				goto dontplay; // Maybe the other player can hear it...

			if (origin->x == listener2.x && origin->y == listener2.y)
				sep = NORM_SEP;
		}
		else if (!origin)
			// Do not play origin-less sounds for the second player.
			// The first player will be able to hear it just fine,
			// we really don't want it playing twice.
			goto dontplay;
		else
			sep = NORM_SEP;

		// try to find a channel
		cnum = S_getChannel(origin, sfx);

		if (cnum < 0)
			return; // If there's no free channels, it's not gonna be free for player 1, either.

		// This is supposed to handle the loading/caching.
		// For some odd reason, the caching is done nearly
		// each time the sound is needed?

		// cache data if necessary
		// NOTE: set sfx->data NULL sfx->lump -1 to force a reload
		if (!sfx->data)
			sfx->data = I_GetSfx(sfx);

		// increase the usefulness
		if (sfx->usefulness++ < 0)
			sfx->usefulness = -1;

#ifdef SURROUND
		// Avoid channel reverse if surround
		if (stereoreverse.value && sep != SURROUND_SEP)
			sep = (~sep) & 255;
#else
		if (stereoreverse.value)
			sep = (~sep) & 255;
#endif

		// Assigns the handle to one of the channels in the
		// mix/output buffer.
		channels[cnum].handle = I_StartSound(sfx_id, volume, sep, pitch, priority, cnum);
	}

dontplay:

	// Check to see if it is audible, and if not, modify the params
	if (origin && origin != listenmobj)
	{
		INT32 rc;
		rc = S_AdjustSoundParams(listenmobj, origin, &volume, &sep, &pitch, sfx);

		if (!rc)
			return;

		if (origin->x == listener.x && origin->y == listener.y)
			sep = NORM_SEP;
	}
	else
		sep = NORM_SEP;

	// try to find a channel
	cnum = S_getChannel(origin, sfx);

	if (cnum < 0)
		return;

	// This is supposed to handle the loading/caching.
	// For some odd reason, the caching is done nearly
	// each time the sound is needed?

	// cache data if necessary
	// NOTE: set sfx->data NULL sfx->lump -1 to force a reload
	if (!sfx->data)
		sfx->data = I_GetSfx(sfx);

	// increase the usefulness
	if (sfx->usefulness++ < 0)
		sfx->usefulness = -1;

#ifdef SURROUND
	// Avoid channel reverse if surround
	if (stereoreverse.value && sep != SURROUND_SEP)
		sep = (~sep) & 255;
#else
	if (stereoreverse.value)
		sep = (~sep) & 255;
#endif

	// Assigns the handle to one of the channels in the
	// mix/output buffer.
	channels[cnum].handle = I_StartSound(sfx_id, volume, sep, pitch, priority, cnum);
}

void S_StartSound(const void *origin, sfxenum_t sfx_id)
{
	if (S_SoundDisabled())
		return;

	if (mariomode) // Sounds change in Mario mode!
	{
		switch (sfx_id)
		{
//			case sfx_altow1:
//			case sfx_altow2:
//			case sfx_altow3:
//			case sfx_altow4:
//				sfx_id = sfx_mario8;
//				break;
			case sfx_thok:
				sfx_id = sfx_mario7;
				break;
			case sfx_pop:
				sfx_id = sfx_mario5;
				break;
			case sfx_jump:
				sfx_id = sfx_mario6;
				break;
			case sfx_shield:
				sfx_id = sfx_mario3;
				break;
			case sfx_itemup:
				sfx_id = sfx_mario4;
				break;
//			case sfx_tink:
//				sfx_id = sfx_mario1;
//				break;
//			case sfx_cgot:
//				sfx_id = sfx_mario9;
//				break;
//			case sfx_lose:
//				sfx_id = sfx_mario2;
//				break;
			default:
				break;
		}
	}
	if (maptol & TOL_XMAS) // Some sounds change for xmas
	{
		switch (sfx_id)
		{
		case sfx_ideya:
		case sfx_nbmper:
		case sfx_ncitem:
		case sfx_ngdone:
			++sfx_id;
		default:
			break;
		}
	}

	// the volume is handled 8 bits
#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
		HW3S_StartSound(origin, sfx_id);
	else
#endif
		S_StartSoundAtVolume(origin, sfx_id, 255);
}

void S_StopSound(void *origin)
{
	INT32 cnum;

	// Sounds without origin can have multiple sources, they shouldn't
	// be stopped by new sounds.
	if (!origin)
		return;

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_StopSound(origin);
		return;
	}
#endif
	for (cnum = 0; cnum < numofchannels; cnum++)
	{
		if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
		{
			S_StopChannel(cnum);
			break;
		}
	}
}

//
// Updates music & sounds
//
static INT32 actualsfxvolume; // check for change through console
static INT32 actualdigmusicvolume;
static INT32 actualmidimusicvolume;

void S_UpdateSounds(void)
{
	INT32 audible, cnum, volume, sep, pitch;
	channel_t *c;

	listener_t listener;
	listener_t listener2;

	mobj_t *listenmobj = players[displayplayer].mo;
	mobj_t *listenmobj2 = NULL;

	memset(&listener, 0, sizeof(listener_t));
	memset(&listener2, 0, sizeof(listener_t));

	// Update sound/music volumes, if changed manually at console
	if (actualsfxvolume != cv_soundvolume.value)
		S_SetSfxVolume (cv_soundvolume.value);
	if (actualdigmusicvolume != cv_digmusicvolume.value)
		S_SetDigMusicVolume (cv_digmusicvolume.value);
	if (actualmidimusicvolume != cv_midimusicvolume.value)
		S_SetMIDIMusicVolume (cv_midimusicvolume.value);

	// We're done now, if we're not in a level.
	if (gamestate != GS_LEVEL)
	{
#ifndef NOMUMBLE
		// Stop Mumble cutting out. I'm sick of it.
		I_UpdateMumble(NULL, listener);
#endif

		// Stop cutting FMOD out. WE'RE sick of it.
		I_UpdateSound();
		return;
	}

	if (dedicated || sound_disabled)
		return;

	if (players[displayplayer].awayviewtics)
		listenmobj = players[displayplayer].awayviewmobj;

	if (splitscreen)
	{
		listenmobj2 = players[secondarydisplayplayer].mo;
		if (players[secondarydisplayplayer].awayviewtics)
			listenmobj2 = players[secondarydisplayplayer].awayviewmobj;
	}

	if (camera.chase && !players[displayplayer].awayviewtics)
	{
		listener.x = camera.x;
		listener.y = camera.y;
		listener.z = camera.z;
		listener.angle = camera.angle;
	}
	else if (listenmobj)
	{
		listener.x = listenmobj->x;
		listener.y = listenmobj->y;
		listener.z = listenmobj->z;
		listener.angle = listenmobj->angle;
	}

#ifndef NOMUMBLE
	I_UpdateMumble(players[consoleplayer].mo, listener);
#endif

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
	{
		HW3S_UpdateSources();
		I_UpdateSound();
		return;
	}
#endif

	if (listenmobj2)
	{
		if (camera2.chase && !players[secondarydisplayplayer].awayviewtics)
		{
			listener2.x = camera2.x;
			listener2.y = camera2.y;
			listener2.z = camera2.z;
			listener2.angle = camera2.angle;
		}
		else
		{
			listener2.x = listenmobj2->x;
			listener2.y = listenmobj2->y;
			listener2.z = listenmobj2->z;
			listener2.angle = listenmobj2->angle;
		}
	}

	for (cnum = 0; cnum < numofchannels; cnum++)
	{
		c = &channels[cnum];

		if (c->sfxinfo)
		{
			if (I_SoundIsPlaying(c->handle))
			{
				// initialize parameters
				volume = 255; // 8 bits internal volume precision
				pitch = NORM_PITCH;
				sep = NORM_SEP;

				// check non-local sounds for distance clipping
				//  or modify their params
				if (c->origin && ((c->origin != players[consoleplayer].mo) ||
					(splitscreen && c->origin != players[secondarydisplayplayer].mo)))
				{
					// Whomever is closer gets the sound, but only in splitscreen.
					if (listenmobj && listenmobj2 && splitscreen)
					{
						const mobj_t *soundmobj = c->origin;

						fixed_t dist1, dist2;
						dist1 = P_AproxDistance(listener.x-soundmobj->x, listener.y-soundmobj->y);
						dist2 = P_AproxDistance(listener2.x-soundmobj->x, listener2.y-soundmobj->y);

						if (dist1 <= dist2)
						{
							// Player 1 gets the sound
							audible = S_AdjustSoundParams(listenmobj, c->origin, &volume, &sep, &pitch,
								c->sfxinfo);
						}
						else
						{
							// Player 2 gets the sound
							audible = S_AdjustSoundParams(listenmobj2, c->origin, &volume, &sep, &pitch,
								c->sfxinfo);
						}

						if (audible)
							I_UpdateSoundParams(c->handle, volume, sep, pitch);
						else
							S_StopChannel(cnum);
					}
					else if (listenmobj && !splitscreen)
					{
						// In the case of a single player, he or she always should get updated sound.
						audible = S_AdjustSoundParams(listenmobj, c->origin, &volume, &sep, &pitch,
							c->sfxinfo);

						if (audible)
							I_UpdateSoundParams(c->handle, volume, sep, pitch);
						else
							S_StopChannel(cnum);
					}
				}
			}
			else
			{
				// if channel is allocated but sound has stopped, free it
				S_StopChannel(cnum);
			}
		}
	}

	I_UpdateSound();
}

void S_SetSfxVolume(INT32 volume)
{
	if (volume < 0 || volume > 31)
		CONS_Alert(CONS_WARNING, "sfxvolume should be between 0-31\n");

	CV_SetValue(&cv_soundvolume, volume&0x1F);
	actualsfxvolume = cv_soundvolume.value; // check for change of var

#ifdef HW3SOUND
	hws_mode == HWS_DEFAULT_MODE ? I_SetSfxVolume(volume&0x1F) : HW3S_SetSfxVolume(volume&0x1F);
#else
	// now hardware volume
	I_SetSfxVolume(volume&0x1F);
#endif
}

void S_ClearSfx(void)
{
#ifndef DJGPPDOS
	size_t i;
	for (i = 1; i < NUMSFX; i++)
		I_FreeSfx(S_sfx + i);
#endif
}

static void S_StopChannel(INT32 cnum)
{
	INT32 i;
	channel_t *c = &channels[cnum];

	if (c->sfxinfo)
	{
		// stop the sound playing
		if (I_SoundIsPlaying(c->handle))
			I_StopSound(c->handle);

		// check to see
		//  if other channels are playing the sound
		for (i = 0; i < numofchannels; i++)
			if (cnum != i && c->sfxinfo == channels[i].sfxinfo)
				break;

		// degrade usefulness of sound data
		c->sfxinfo->usefulness--;

		c->sfxinfo = 0;
	}
}

//
// S_CalculateSoundDistance
//
// Calculates the distance between two points for a sound.
// Clips the distance to prevent overflow.
//
fixed_t S_CalculateSoundDistance(fixed_t sx1, fixed_t sy1, fixed_t sz1, fixed_t sx2, fixed_t sy2, fixed_t sz2)
{
	fixed_t approx_dist, adx, ady;

	// calculate the distance to sound origin and clip it if necessary
	adx = abs((sx1>>FRACBITS) - (sx2>>FRACBITS));
	ady = abs((sy1>>FRACBITS) - (sy2>>FRACBITS));

	// From _GG1_ p.428. Approx. euclidian distance fast.
	// Take Z into account
	adx = adx + ady - ((adx < ady ? adx : ady)>>1);
	ady = abs((sz1>>FRACBITS) - (sz2>>FRACBITS));
	approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);

	if (approx_dist >= FRACUNIT/2)
		approx_dist = FRACUNIT/2-1;

	approx_dist <<= FRACBITS;

	return approx_dist;
}

//
// Changes volume, stereo-separation, and pitch variables
// from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//
INT32 S_AdjustSoundParams(const mobj_t *listener, const mobj_t *source, INT32 *vol, INT32 *sep, INT32 *pitch,
	sfxinfo_t *sfxinfo)
{
	fixed_t approx_dist;
	angle_t angle;

	listener_t listensource;

	(void)pitch;
	if (!listener)
		return false;

	if (listener == players[displayplayer].mo && camera.chase)
	{
		listensource.x = camera.x;
		listensource.y = camera.y;
		listensource.z = camera.z;
		listensource.angle = camera.angle;
	}
	else if (splitscreen && listener == players[secondarydisplayplayer].mo && camera2.chase)
	{
		listensource.x = camera2.x;
		listensource.y = camera2.y;
		listensource.z = camera2.z;
		listensource.angle = camera2.angle;
	}
	else
	{
		listensource.x = listener->x;
		listensource.y = listener->y;
		listensource.z = listener->z;
		listensource.angle = listener->angle;
	}

	if (sfxinfo->pitch & SF_OUTSIDESOUND) // Rain special case
	{
		fixed_t x, y, yl, yh, xl, xh, newdist;

		if (R_PointInSubsectorFast(listensource.x, listensource.y)->sector->ceilingpic == skyflatnum)
			approx_dist = 0;
		else
		{
			// Essentially check in a 1024 unit radius of the player for an outdoor area.
			yl = listensource.y - 1024*FRACUNIT;
			yh = listensource.y + 1024*FRACUNIT;
			xl = listensource.x - 1024*FRACUNIT;
			xh = listensource.x + 1024*FRACUNIT;
			approx_dist = 1024*FRACUNIT;
			for (y = yl; y <= yh; y += FRACUNIT*64)
				for (x = xl; x <= xh; x += FRACUNIT*64)
				{
					if (R_PointInSubsectorFast(x, y)->sector->ceilingpic == skyflatnum)
					{
						// Found the outdoors!
						newdist = S_CalculateSoundDistance(listensource.x, listensource.y, 0, x, y, 0);
						if (newdist < approx_dist)
						{
							approx_dist = newdist;
						}
					}
				}
		}
	}
	else
	{
		approx_dist = S_CalculateSoundDistance(listensource.x, listensource.y, listensource.z,
												source->x, source->y, source->z);
	}

	// Ring loss, deaths, etc, should all be heard louder.
	if (sfxinfo->pitch & SF_X8AWAYSOUND)
		approx_dist = FixedDiv(approx_dist,8*FRACUNIT);

	// Combine 8XAWAYSOUND with 4XAWAYSOUND and get.... 32XAWAYSOUND?
	if (sfxinfo->pitch & SF_X4AWAYSOUND)
		approx_dist = FixedDiv(approx_dist,4*FRACUNIT);

	if (sfxinfo->pitch & SF_X2AWAYSOUND)
		approx_dist = FixedDiv(approx_dist,2*FRACUNIT);

	if (approx_dist > S_CLIPPING_DIST)
		return 0;

	// angle of source to listener
	angle = R_PointToAngle2(listensource.x, listensource.y, source->x, source->y);

	if (angle > listensource.angle)
		angle = angle - listensource.angle;
	else
		angle = angle + InvAngle(listensource.angle);

#ifdef SURROUND
	// Produce a surround sound for angle from 105 till 255
	if (surround.value == 1 && (angle > ANG105 && angle < ANG255 ))
		*sep = SURROUND_SEP;
	else
#endif
	{
		angle >>= ANGLETOFINESHIFT;

		// stereo separation
		*sep = 128 - (FixedMul(S_STEREO_SWING, FINESINE(angle))>>FRACBITS);
	}

	// volume calculation
	if (approx_dist < S_CLOSE_DIST)
	{
		// SfxVolume is now hardware volume
		*vol = 255; // not snd_SfxVolume
	}
	else
	{
		// distance effect
		*vol = (15 * ((S_CLIPPING_DIST - approx_dist)>>FRACBITS)) / S_ATTENUATOR;
	}

	return (*vol > 0);
}

// Searches through the channels and checks if a sound is playing
// on the given origin.
INT32 S_OriginPlaying(void *origin)
{
	INT32 cnum;
	if (!origin)
		return false;

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
		return HW3S_OriginPlaying(origin);
#endif

	for (cnum = 0; cnum < numofchannels; cnum++)
		if (channels[cnum].origin == origin)
			return 1;
	return 0;
}

// Searches through the channels and checks if a given id
// is playing anywhere.
INT32 S_IdPlaying(sfxenum_t id)
{
	INT32 cnum;

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
		return HW3S_IdPlaying(id);
#endif

	for (cnum = 0; cnum < numofchannels; cnum++)
		if ((size_t)(channels[cnum].sfxinfo - S_sfx) == (size_t)id)
			return 1;
	return 0;
}

// Searches through the channels and checks for
// origin x playing sound id y.
INT32 S_SoundPlaying(void *origin, sfxenum_t id)
{
	INT32 cnum;
	if (!origin)
		return 0;

#ifdef HW3SOUND
	if (hws_mode != HWS_DEFAULT_MODE)
		return HW3S_SoundPlaying(origin, id);
#endif

	for (cnum = 0; cnum < numofchannels; cnum++)
	{
		if (channels[cnum].origin == origin
		 && (size_t)(channels[cnum].sfxinfo - S_sfx) == (size_t)id)
			return 1;
	}
	return 0;
}

//
// S_StartSoundName
// Starts a sound using the given name.
#define MAXNEWSOUNDS 10
static sfxenum_t newsounds[MAXNEWSOUNDS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void S_StartSoundName(void *mo, const char *soundname)
{
	INT32 i, soundnum = 0;
	// Search existing sounds...
	for (i = sfx_None + 1; i < NUMSFX; i++)
	{
		if (!S_sfx[i].name)
			continue;
		if (!stricmp(S_sfx[i].name, soundname))
		{
			soundnum = i;
			break;
		}
	}

	if (!soundnum)
	{
		for (i = 0; i < MAXNEWSOUNDS; i++)
		{
			if (newsounds[i] == 0)
				break;
			if (!S_IdPlaying(newsounds[i]))
			{
				S_RemoveSoundFx(newsounds[i]);
				break;
			}
		}

		if (i == MAXNEWSOUNDS)
		{
			CONS_Debug(DBG_GAMELOGIC, "Cannot load another extra sound!\n");
			return;
		}

		soundnum = S_AddSoundFx(soundname, false, 0, false);
		newsounds[i] = soundnum;
	}

	S_StartSound(mo, soundnum);
}

//
// Initializes sound stuff, including volume
// Sets channels, SFX volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_InitSfxChannels(INT32 sfxVolume)
{
	INT32 i;

	if (dedicated)
		return;

	S_SetSfxVolume(sfxVolume);

	SetChannelsNum();

	// Note that sounds have not been cached (yet).
	for (i = 1; i < NUMSFX; i++)
	{
		S_sfx[i].usefulness = -1; // for I_GetSfx()
		S_sfx[i].lumpnum = LUMPERROR;
	}

	// precache sounds if requested by cmdline, or precachesound var true
	if (!sound_disabled && (M_CheckParm("-precachesound") || precachesound.value))
	{
		// Initialize external data (all sounds) at start, keep static.
		CONS_Printf(M_GetText("Loading sounds... "));

		for (i = 1; i < NUMSFX; i++)
			if (S_sfx[i].name)
				S_sfx[i].data = I_GetSfx(&S_sfx[i]);

		CONS_Printf(M_GetText(" pre-cached all sound data\n"));
	}
}

/// ------------------------
/// Music
/// ------------------------

#ifdef MUSICSLOT_COMPATIBILITY
const char *compat_special_music_slots[16] =
{
	"titles", // 1036  title screen
	"read_m", // 1037  intro
	"lclear", // 1038  level clear
	"invinc", // 1039  invincibility
	"shoes",  // 1040  super sneakers
	"minvnc", // 1041  Mario invincibility
	"drown",  // 1042  drowning
	"gmover", // 1043  game over
	"xtlife", // 1044  extra life
	"contsc", // 1045  continue screen
	"supers", // 1046  Super Sonic
	"chrsel", // 1047  character select
	"credit", // 1048  credits
	"racent", // 1049  Race Results
	"stjr",   // 1050  Sonic Team Jr. Presents
	""
};
#endif

static char      music_name[7]; // up to 6-character name
static void      *music_data;
static UINT16    music_flags;
static boolean   music_looping;

static char      queue_name[7];
static UINT16    queue_flags;
static boolean   queue_looping;
static UINT32    queue_position;
static UINT32    queue_fadeinms;

/// ------------------------
/// Music Status
/// ------------------------

boolean S_DigMusicDisabled(void)
{
	return digital_disabled;
}

boolean S_MIDIMusicDisabled(void)
{
	return midi_disabled;
}

boolean S_MusicDisabled(void)
{
	return (midi_disabled && digital_disabled);
}

boolean S_MusicPlaying(void)
{
	return I_SongPlaying();
}

boolean S_MusicPaused(void)
{
	return I_SongPaused();
}

boolean S_MusicNotInFocus(void)
{
	return (
			( window_notinfocus && ! cv_playmusicifunfocused.value )
	);
}
musictype_t S_MusicType(void)
{
	return I_SongType();
}

const char *S_MusicName(void)
{
	return music_name;
}

boolean S_MusicExists(const char *mname, boolean checkMIDI, boolean checkDigi)
{
	return (
		(checkDigi ? W_CheckNumForName(va("O_%s", mname)) != LUMPERROR : false)
		|| (checkMIDI ? W_CheckNumForName(va("D_%s", mname)) != LUMPERROR : false)
	);
}

/// ------------------------
/// Music Effects
/// ------------------------

boolean S_SpeedMusic(float speed)
{
	return I_SetSongSpeed(speed);
}

/// ------------------------
/// Music Seeking
/// ------------------------

UINT32 S_GetMusicLength(void)
{
	return I_GetSongLength();
}

boolean S_SetMusicLoopPoint(UINT32 looppoint)
{
	return I_SetSongLoopPoint(looppoint);
}

UINT32 S_GetMusicLoopPoint(void)
{
	return I_GetSongLoopPoint();
}

boolean S_SetMusicPosition(UINT32 position)
{
	return I_SetSongPosition(position);
}

UINT32 S_GetMusicPosition(void)
{
	return I_GetSongPosition();
}

/// ------------------------
/// Music Playback
/// ------------------------

static lumpnum_t S_GetMusicLumpNum(const char *mname)
{
	boolean midipref = cv_musicpref.value;

	if (S_PrefAvailable(midipref, mname))
		return W_GetNumForName(va(midipref ? "d_%s":"o_%s", mname));
	else if (S_PrefAvailable(!midipref, mname))
		return W_GetNumForName(va(midipref ? "o_%s":"d_%s", mname));
	else
		return LUMPERROR;
}

static boolean S_LoadMusic(const char *mname)
{
	lumpnum_t mlumpnum;
	void *mdata;

	if (S_MusicDisabled())
		return false;

	mlumpnum = S_GetMusicLumpNum(mname);

	if (mlumpnum == LUMPERROR)
	{
		CONS_Alert(CONS_ERROR, "Music %.6s could not be loaded: lump not found!\n", mname);
		return false;
	}

	// load & register it
	mdata = W_CacheLumpNum(mlumpnum, PU_MUSIC);

#ifdef MUSSERV
	if (msg_id != -1)
	{
		struct musmsg msg_buffer;

		msg_buffer.msg_type = 6;
		memset(msg_buffer.msg_text, 0, sizeof (msg_buffer.msg_text));
		sprintf(msg_buffer.msg_text, "d_%s", mname);
		msgsnd(msg_id, (struct msgbuf*)&msg_buffer, sizeof (msg_buffer.msg_text), IPC_NOWAIT);
	}
#endif

	if (I_LoadSong(mdata, W_LumpLength(mlumpnum)))
	{
		strncpy(music_name, mname, 7);
		music_name[6] = 0;
		music_data = mdata;
		return true;
	}
	else
		return false;
}

static void S_UnloadMusic(void)
{
	I_UnloadSong();

#ifndef HAVE_SDL //SDL uses RWOPS
	Z_ChangeTag(music_data, PU_CACHE);
#endif
	music_data = NULL;

	music_name[0] = 0;
	music_flags = 0;
	music_looping = false;
}

static boolean S_PlayMusic(boolean looping, UINT32 fadeinms)
{
	if (S_MusicDisabled())
		return false;

	if ((!fadeinms && !I_PlaySong(looping)) ||
		(fadeinms && !I_FadeInPlaySong(fadeinms, looping)))
	{
		S_UnloadMusic();
		return false;
	}

	S_InitMusicVolume(); // switch between digi and sequence volume

	if (S_MusicNotInFocus())
		S_PauseAudio();

	return true;
}

static void S_QueueMusic(const char *mmusic, UINT16 mflags, boolean looping, UINT32 position, UINT32 fadeinms)
{
	strncpy(queue_name, mmusic, 7);
	queue_flags = mflags;
	queue_looping = looping;
	queue_position = position;
	queue_fadeinms = fadeinms;
}

static void S_ClearQueue(void)
{
	queue_name[0] = 0;
	queue_flags = 0;
	queue_looping = false;
	queue_position = 0;
	queue_fadeinms = 0;
}

static void S_ChangeMusicToQueue(void)
{
	S_ChangeMusicEx(queue_name, queue_flags, queue_looping, queue_position, 0, queue_fadeinms);
	S_ClearQueue();
}

void S_ChangeMusicEx(const char *mmusic, UINT16 mflags, boolean looping, UINT32 position, UINT32 prefadems, UINT32 fadeinms)
{
	char newmusic[7];
	boolean currentmidi = (I_SongType() == MU_MID);
	boolean midipref = cv_musicpref.value;

#if defined (DC) || defined (_WIN32_WCE) || defined (PSP) || defined(GP2X)
	S_ClearSfx();
#endif

	if (S_MusicDisabled())
		return;

	strncpy(newmusic, mmusic, 7);
#if defined(HAVE_LUA_MUSICPLUS)
	if(LUAh_MusicChange(music_name, newmusic, &mflags, &looping, &position, &prefadems, &fadeinms))
		return;
#endif
	newmusic[6] = 0;

 	// No Music (empty string)
	if (newmusic[0] == 0)
 	{
		if (prefadems)
			I_FadeSong(0, prefadems, &S_StopMusic);
		else
			S_StopMusic();
		return;
	}

	if (prefadems && S_MusicPlaying()) // queue music change for after fade // allow even if the music is the same
	{
		CONS_Debug(DBG_DETAILED, "Now fading out song %s\n", music_name);
		S_QueueMusic(newmusic, mflags, looping, position, fadeinms);
		I_FadeSong(0, prefadems, S_ChangeMusicToQueue);
		return;
	}
	else if (strnicmp(music_name, newmusic, 6) || (mflags & MUSIC_FORCERESET) ||
		(midipref != currentmidi && S_PrefAvailable(midipref, newmusic)))
 	{
		CONS_Debug(DBG_DETAILED, "Now playing song %s\n", newmusic);

		S_StopMusic();

		if (!S_LoadMusic(newmusic))
		{
			CONS_Alert(CONS_ERROR, "Music %.6s could not be loaded!\n", newmusic);
			return;
		}

		music_flags = mflags;
		music_looping = looping;

		if (!S_PlayMusic(looping, fadeinms))
 		{
			CONS_Alert(CONS_ERROR, "Music %.6s could not be played!\n", newmusic);
			return;
		}

		if (position)
			I_SetSongPosition(position);

		I_SetSongTrack(mflags & MUSIC_TRACKMASK);
	}
	else if (fadeinms) // let fades happen with same music
	{
		I_SetSongPosition(position);
		I_FadeSong(100, fadeinms, NULL);
 	}
	else // reset volume to 100 with same music
	{
		I_StopFadingSong();
		I_FadeSong(100, 500, NULL);
	}
}

void S_StopMusic(void)
{
	if (!I_SongPlaying())
		return;

	if (I_SongPaused())
		I_ResumeSong();

	S_SpeedMusic(1.0f);
	I_StopSong();
	S_UnloadMusic(); // for now, stopping also means you unload the song
}

//
// Stop and resume music, during game PAUSE.
//
void S_PauseAudio(void)
{
	if (I_SongPlaying() && !I_SongPaused())
		I_PauseSong();
}

void S_ResumeAudio(void)
{
	if (S_MusicNotInFocus())
		return;

	if (I_SongPlaying() && I_SongPaused())
		I_ResumeSong();
}

void S_SetMusicVolume(INT32 digvolume, INT32 seqvolume)
{
	if (digvolume < 0)
		digvolume = cv_digmusicvolume.value;
	if (seqvolume < 0)
		seqvolume = cv_midimusicvolume.value;

	if (digvolume < 0 || digvolume > 31)
		CONS_Alert(CONS_WARNING, "digmusicvolume should be between 0-31\n");
	CV_SetValue(&cv_digmusicvolume, digvolume&31);
	actualdigmusicvolume = cv_digmusicvolume.value;   //check for change of var

	if (seqvolume < 0 || seqvolume > 31)
		CONS_Alert(CONS_WARNING, "midimusicvolume should be between 0-31\n");
	CV_SetValue(&cv_midimusicvolume, seqvolume&31);
	actualmidimusicvolume = cv_midimusicvolume.value;   //check for change of var

	switch(I_SongType())
	{
		case MU_MID:
		//case MU_MOD:
		//case MU_GME:
			I_SetMusicVolume(seqvolume&31);
			break;
		default:
			I_SetMusicVolume(digvolume&31);
			break;
	}
}

/// ------------------------
/// Music Fading
/// ------------------------

void S_SetInternalMusicVolume(INT32 volume)
{
	I_SetInternalMusicVolume(min(max(volume, 0), 100));
}

void S_StopFadingMusic(void)
{
	I_StopFadingSong();
}

boolean S_FadeMusicFromVolume(UINT8 target_volume, INT16 source_volume, UINT32 ms)
{
	if (source_volume < 0)
		return I_FadeSong(target_volume, ms, NULL);
	else
		return I_FadeSongFromVolume(target_volume, source_volume, ms, NULL);
}

boolean S_FadeOutStopMusic(UINT32 ms)
{
	return I_FadeSong(0, ms, &S_StopMusic);
}

/// ------------------------
/// Init & Others
/// ------------------------

//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void)
{
	if (mapmusflags & MUSIC_RELOADRESET)
	{
		strncpy(mapmusname, mapheaderinfo[gamemap-1]->musname, 7);
		mapmusname[6] = 0;
		mapmusflags = (mapheaderinfo[gamemap-1]->mustrack & MUSIC_TRACKMASK);
		mapmusposition = mapheaderinfo[gamemap-1]->muspos;
	}

	if (cv_resetmusic.value)
		S_StopMusic();
	S_ChangeMusicEx(mapmusname, mapmusflags, true, mapmusposition, 0, 0);
}

static void Command_Tunes_f(void)
{
	const char *tunearg;
	UINT16 tunenum, track = 0;
	UINT32 position = 0;
	const size_t argc = COM_Argc();

	if (argc < 2) //tunes slot ...
	{
		CONS_Printf("tunes <name/num> [track] [speed] [position] / <-show> / <-default> / <-none>:\n");
		CONS_Printf(M_GetText("Play an arbitrary music lump. If a map number is used, 'MAP##M' is played.\n"));
		CONS_Printf(M_GetText("If the format supports multiple songs, you can specify which one to play.\n\n"));
		CONS_Printf(M_GetText("* With \"-show\", shows the currently playing tune and track.\n"));
		CONS_Printf(M_GetText("* With \"-default\", returns to the default music for the map.\n"));
		CONS_Printf(M_GetText("* With \"-none\", any music playing will be stopped.\n"));
		return;
	}

	tunearg = COM_Argv(1);
	tunenum = (UINT16)atoi(tunearg);
	track = 0;

	if (!strcasecmp(tunearg, "-show"))
	{
		CONS_Printf(M_GetText("The current tune is: %s [track %d]\n"),
			mapmusname, (mapmusflags & MUSIC_TRACKMASK));
		return;
	}
	if (!strcasecmp(tunearg, "-none"))
	{
		S_StopMusic();
		return;
	}
	else if (!strcasecmp(tunearg, "-default"))
	{
		tunearg = mapheaderinfo[gamemap-1]->musname;
		track = mapheaderinfo[gamemap-1]->mustrack;
	}
	else if (!tunearg[2] && toupper(tunearg[0]) >= 'A' && toupper(tunearg[0]) <= 'Z')
		tunenum = (UINT16)M_MapNumber(tunearg[0], tunearg[1]);

	if (tunenum && tunenum >= 1036)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Valid music slots are 1 to 1035.\n"));
		return;
	}
	if (!tunenum && strlen(tunearg) > 6) // This is automatic -- just show the error just in case
		CONS_Alert(CONS_NOTICE, M_GetText("Music name too long - truncated to six characters.\n"));

	if (argc > 2)
		track = (UINT16)atoi(COM_Argv(2))-1;

	if (tunenum)
		snprintf(mapmusname, 7, "%sM", G_BuildMapName(tunenum));
	else
		strncpy(mapmusname, tunearg, 7);

	if (argc > 4)
		position = (UINT32)atoi(COM_Argv(4));

	mapmusname[6] = 0;
	mapmusflags = (track & MUSIC_TRACKMASK);
	mapmusposition = position;

	S_ChangeMusicEx(mapmusname, mapmusflags, true, mapmusposition, 0, 0);

	if (argc > 3)
	{
		float speed = (float)atof(COM_Argv(3));
		if (speed > 0.0f)
			S_SpeedMusic(speed);
	}
}

static void Command_RestartAudio_f(void)
{
	S_StopMusic();
	S_StopSounds();
	I_ShutdownMusic();
	I_ShutdownSound();
	I_StartupSound();
	I_InitMusic();

// These must be called or no sound and music until manually set.

	I_SetSfxVolume(cv_soundvolume.value);
	S_SetMusicVolume(cv_digmusicvolume.value, cv_midimusicvolume.value);
	if (Playing()) // Gotta make sure the player is in a level
		P_RestoreMusic(&players[consoleplayer]);
}

void GameSounds_OnChange(void)
{
	if (M_CheckParm("-nosound") || M_CheckParm("-noaudio"))
		return;

	if (sound_disabled)
	{
		sound_disabled = false;
		S_InitSfxChannels(cv_soundvolume.value);
		S_StartSound(NULL, sfx_strpst);
	}
	else
	{
		sound_disabled = true;
		S_StopSounds();
	}
}

void GameDigiMusic_OnChange(void)
{
	if (M_CheckParm("-nomusic") || M_CheckParm("-noaudio"))
		return;
	else if (M_CheckParm("-nodigmusic"))
		return;

	if (digital_disabled)
	{
		digital_disabled = false;
		I_InitMusic();

		if (Playing())
			P_RestoreMusic(&players[consoleplayer]);
		else if ((!cv_musicpref.value || midi_disabled) && S_DigExists("lclear"))
			S_ChangeMusicInternal("lclear", false);
	}
	else
	{
		digital_disabled = true;
		if (S_MusicType() != MU_MID)
		{
			S_StopMusic();
			if (!midi_disabled)
			{
				if (Playing())
					P_RestoreMusic(&players[consoleplayer]);
				else
					S_ChangeMusicInternal("lclear", false);
			}
		}
	}
}

void GameMIDIMusic_OnChange(void)
{
	if (M_CheckParm("-nomusic") || M_CheckParm("-noaudio"))
		return;
	else if (M_CheckParm("-nomidimusic"))
		return;

	if (midi_disabled)
	{
		midi_disabled = false;
		I_InitMusic();

		if (Playing())
			P_RestoreMusic(&players[consoleplayer]);
		else if ((cv_musicpref.value || digital_disabled) && S_MIDIExists("lclear"))
			S_ChangeMusicInternal("lclear", false);
	}
	else
	{
		midi_disabled = true;
		if (S_MusicType() == MU_MID)
		{
			S_StopMusic();
			if (!digital_disabled)
			{
				if (Playing())
					P_RestoreMusic(&players[consoleplayer]);
				else
					S_ChangeMusicInternal("lclear", false);
			}
		}
	}
}

void MusicPref_OnChange(void)
{
	// Ensure sound is started.
	// This gets called on startup, before Mix_OpenAudio has been called, marking native MIDI as broken.
	if(!sound_started)
		return;

	if (M_CheckParm("-nomusic") || M_CheckParm("-noaudio") ||
		M_CheckParm("-nomidimusic") || M_CheckParm("-nodigmusic"))
		return;

	if (Playing())
		P_RestoreMusic(&players[consoleplayer]);
	else if (S_PrefAvailable(cv_musicpref.value, "lclear"))
		S_ChangeMusicInternal("lclear", false);
}

#ifdef HAVE_OPENMPT
void ModFilter_OnChange(void)
{
	if (openmpt_mhandle)
		openmpt_module_set_render_param(openmpt_mhandle, OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH, cv_modfilter.value);
}
#endif
