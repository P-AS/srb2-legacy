// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  d_netcmd.c
/// \brief host/client network commands
///        commands are executed through the command buffer
///	       like console commands, other miscellaneous commands (at the end)

#include "doomdef.h"

#include "console.h"
#include "command.h"
#include "i_time.h"
#include "i_system.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "g_input.h"
#include "m_menu.h"
#include "r_local.h"
#include "r_things.h"
#include "p_local.h"
#include "p_setup.h"
#include "s_sound.h"
#include "i_sound.h"
#include "m_misc.h"
#include "am_map.h"
#include "byteptr.h"
#include "d_netfil.h"
#include "p_spec.h"
#include "m_cheat.h"
#include "d_clisrv.h"
#include "d_net.h"
#include "v_video.h"
#include "d_main.h"
#include "m_random.h"
#include "f_finale.h"
#include "filesrch.h"
#include "mserv.h"
#include "z_zone.h"
#include "lua_script.h"
#include "lua_hook.h"
#include "m_cond.h"
#include "m_anigif.h"
#include "md5.h"
#include "m_perfstats.h"

#ifdef NETGAME_DEVMODE
#define CV_RESTRICT CV_NETVAR
#else
#define CV_RESTRICT 0
#endif

// ------
// protos
// ------

static void Got_NameAndColor(UINT8 **cp, INT32 playernum);
static void Got_WeaponPref(UINT8 **cp, INT32 playernum);
static void Got_Mapcmd(UINT8 **cp, INT32 playernum);
static void Got_ExitLevelcmd(UINT8 **cp, INT32 playernum);
static void Got_RequestAddfilecmd(UINT8 **cp, INT32 playernum);
#ifdef DELFILE
static void Got_Delfilecmd(UINT8 **cp, INT32 playernum);
#endif
static void Got_Addfilecmd(UINT8 **cp, INT32 playernum);
static void Got_Pause(UINT8 **cp, INT32 playernum);
static void Got_Suicide(UINT8 **cp, INT32 playernum);
static void Got_RandomSeed(UINT8 **cp, INT32 playernum);
static void Got_RunSOCcmd(UINT8 **cp, INT32 playernum);
static void Got_Teamchange(UINT8 **cp, INT32 playernum);
static void Got_Clearscores(UINT8 **cp, INT32 playernum);

static void PointLimit_OnChange(void);
static void TimeLimit_OnChange(void);
static void NumLaps_OnChange(void);
static void Mute_OnChange(void);

static void Hidetime_OnChange(void);

static void AutoBalance_OnChange(void);
static void TeamScramble_OnChange(void);

static void NetTimeout_OnChange(void);
static void JoinTimeout_OnChange(void);

static void Ringslinger_OnChange(void);
static void Gravity_OnChange(void);
static void ForceSkin_OnChange(void);

static void Name_OnChange(void);
static void Name2_OnChange(void);
static void Skin_OnChange(void);
static void Skin2_OnChange(void);
static void Color_OnChange(void);
static void Color2_OnChange(void);
static void DummyConsvar_OnChange(void);
static void SoundTest_OnChange(void);

#ifdef NETGAME_DEVMODE
static void Fishcake_OnChange(void);
#endif

static void Command_Playdemo_f(void);
static void Command_Timedemo_f(void);
static void Command_Stopdemo_f(void);
static void Command_StartMovie_f(void);
static void Command_StopMovie_f(void);
static void Command_Map_f(void);
static void Command_ResetCamera_f(void);

static void Command_Addfile(void);
static void Command_ListWADS_f(void);
#ifdef DELFILE
static void Command_Delfile(void);
#endif
static void Command_RunSOC(void);
static void Command_Pause(void);
static void Command_Suicide(void);

static void Command_Version_f(void);
#ifdef UPDATE_ALERT
static void Command_ModDetails_f(void);
#endif
static void Command_ShowGametype_f(void);
FUNCNORETURN static ATTRNORETURN void Command_Quit_f(void);
static void Command_Playintro_f(void);

static void Command_Displayplayer_f(void);

static void Command_ExitLevel_f(void);
static void Command_Showmap_f(void);
static void Command_Mapmd5_f(void);

static void Command_Teamchange_f(void);
static void Command_Teamchange2_f(void);
static void Command_ServerTeamChange_f(void);

static void Command_Clearscores_f(void);

// Remote Administration
static void Command_Changepassword_f(void);
static void Command_Login_f(void);
static void Got_Verification(UINT8 **cp, INT32 playernum);
static void Got_Removal(UINT8 **cp, INT32 playernum);
static void Command_Verify_f(void);
static void Command_RemoveAdmin_f(void);
static void Command_MotD_f(void);
static void Got_MotD_f(UINT8 **cp, INT32 playernum);

static void Command_ShowScores_f(void);
static void Command_ShowTime_f(void);

static void Command_Isgamemodified_f(void);
static void Command_Cheats_f(void);
#ifdef _DEBUG
static void Command_Togglemodified_f(void);
static void Command_Archivetest_f(void);
#endif

// =========================================================================
//                           CLIENT VARIABLES
// =========================================================================

void SendWeaponPref(void);
void SendWeaponPref2(void);

static CV_PossibleValue_t usemouse_cons_t[] = {{0, "Off"}, {1, "On"}, {2, "Force"}, {0, NULL}};
#if (defined (__unix__) && !defined (MSDOS)) || defined(__APPLE__) || defined (UNIXCOMMON)
static CV_PossibleValue_t mouse2port_cons_t[] = {{0, "/dev/gpmdata"}, {1, "/dev/ttyS0"},
	{2, "/dev/ttyS1"}, {3, "/dev/ttyS2"}, {4, "/dev/ttyS3"}, {0, NULL}};
#else
static CV_PossibleValue_t mouse2port_cons_t[] = {{1, "COM1"}, {2, "COM2"}, {3, "COM3"}, {4, "COM4"},
	{0, NULL}};
#endif

#ifdef LJOYSTICK
static CV_PossibleValue_t joyport_cons_t[] = {{1, "/dev/js0"}, {2, "/dev/js1"}, {3, "/dev/js2"},
	{4, "/dev/js3"}, {0, NULL}};
#else
// accept whatever value - it is in fact the joystick device number
#define usejoystick_cons_t NULL
#endif

static CV_PossibleValue_t autobalance_cons_t[] = {{0, "MIN"}, {4, "MAX"}, {0, NULL}};
static CV_PossibleValue_t teamscramble_cons_t[] = {{0, "Off"}, {1, "Random"}, {2, "Points"}, {0, NULL}};

static CV_PossibleValue_t startingliveslimit_cons_t[] = {{1, "MIN"}, {99, "MAX"}, {0, NULL}};
static CV_PossibleValue_t sleeping_cons_t[] = {{0, "MIN"}, {1000/TICRATE, "MAX"}, {0, NULL}};
static CV_PossibleValue_t competitionboxes_cons_t[] = {{0, "Normal"}, {1, "Random"}, {2, "Teleports"},
	{3, "None"}, {0, NULL}};

static CV_PossibleValue_t matchboxes_cons_t[] = {{0, "Normal"}, {1, "Random"}, {2, "Non-Random"},
	{3, "None"}, {0, NULL}};

static CV_PossibleValue_t chances_cons_t[] = {{0, "MIN"}, {9, "MAX"}, {0, NULL}};
static CV_PossibleValue_t match_scoring_cons_t[] = {{0, "Normal"}, {1, "Classic"}, {0, NULL}};
static CV_PossibleValue_t pause_cons_t[] = {{0, "Server"}, {1, "All"}, {0, NULL}};

static CV_PossibleValue_t timetic_cons_t[] = {{0, "Normal"}, {1, "Centiseconds"}, {2, "Mania"}, {3, "Tics"}, {0, NULL}};
//Using "Normal" instead of 2.2's "Classic" in order to be compatible with people's existing configurations.

consvar_t cv_showinput = CVAR_INIT ("showinput", "Off", NULL, CV_SAVE, CV_OnOff, NULL);
consvar_t cv_showinputjoy = CVAR_INIT ("showinputjoy", "Off", NULL, 0, CV_OnOff, NULL);
static CV_PossibleValue_t  scorepos_cons_t[] = {{0, "Normal"}, {1, "Modern"}, {0, NULL}};

#ifdef NETGAME_DEVMODE
static consvar_t cv_fishcake = CVAR_INIT ("fishcake", "Off", NULL, CV_CALL|CV_NOSHOWHELP|CV_RESTRICT, CV_OnOff, Fishcake_OnChange);
#endif
static consvar_t cv_dummyconsvar = CVAR_INIT ("dummyconsvar", "Off", NULL, CV_CALL|CV_NOSHOWHELP, CV_OnOff,
	DummyConsvar_OnChange);

consvar_t cv_restrictskinchange = CVAR_INIT ("restrictskinchange", "Yes", NULL, CV_NETVAR|CV_CHEAT, CV_YesNo, NULL);
consvar_t cv_allowteamchange = CVAR_INIT ("allowteamchange", "Yes", NULL, CV_NETVAR, CV_YesNo, NULL);

consvar_t cv_startinglives = CVAR_INIT ("startinglives", "3", NULL, CV_NETVAR|CV_CHEAT, startingliveslimit_cons_t, NULL);

static CV_PossibleValue_t respawntime_cons_t[] = {{0, "MIN"}, {30, "MAX"}, {0, NULL}};
consvar_t cv_respawntime = CVAR_INIT ("respawndelay", "3", NULL, CV_NETVAR|CV_CHEAT, respawntime_cons_t, NULL);

consvar_t cv_competitionboxes = CVAR_INIT ("competitionboxes", "Random", NULL, CV_NETVAR|CV_CHEAT, competitionboxes_cons_t, NULL);

#ifdef SEENAMES
static CV_PossibleValue_t seenames_cons_t[] = {{0, "Off"}, {1, "Colorless"}, {2, "Team"}, {3, "Ally/Foe"}, {0, NULL}};
consvar_t cv_seenames = CVAR_INIT ("seenames", "Ally/Foe", "If and how to show other players name when close to them", CV_SAVE, seenames_cons_t, 0);
consvar_t cv_allowseenames = CVAR_INIT ("allowseenames", "Yes", NULL, CV_NETVAR, CV_YesNo, NULL);
#endif

// these are just meant to be saved to the config
consvar_t cv_playername = CVAR_INIT ("name", "Sonic", NULL, CV_SAVE|CV_CALL|CV_NOINIT, NULL, Name_OnChange);
consvar_t cv_playername2 = CVAR_INIT ("name2", "Tails", NULL, CV_SAVE|CV_CALL|CV_NOINIT, NULL, Name2_OnChange);
// player colors
consvar_t cv_playercolor = CVAR_INIT ("color", "Blue", NULL, CV_CALL|CV_NOINIT, Color_cons_t, Color_OnChange);
consvar_t cv_playercolor2 = CVAR_INIT ("color2", "Orange", NULL, CV_CALL|CV_NOINIT, Color_cons_t, Color2_OnChange);
// player's skin, saved for commodity, when using a favorite skins wad..
consvar_t cv_skin = CVAR_INIT ("skin", DEFAULTSKIN, NULL, CV_CALL|CV_NOINIT, NULL, Skin_OnChange);
consvar_t cv_skin2 = CVAR_INIT ("skin2", DEFAULTSKIN2, NULL, CV_CALL|CV_NOINIT, NULL, Skin2_OnChange);

// saved versions of the above six
consvar_t cv_defaultplayercolor = CVAR_INIT ("defaultcolor", "Blue", NULL, CV_SAVE, Color_cons_t, NULL);
consvar_t cv_defaultplayercolor2 = CVAR_INIT ("defaultcolor2", "Orange", NULL, CV_SAVE, Color_cons_t, NULL);
consvar_t cv_defaultskin = CVAR_INIT ("defaultskin", DEFAULTSKIN, NULL, CV_SAVE, NULL, NULL);
consvar_t cv_defaultskin2 = CVAR_INIT ("defaultskin2", DEFAULTSKIN2, NULL, CV_SAVE, NULL, NULL);

consvar_t cv_skipmapcheck = CVAR_INIT ("skipmapcheck", "Off", NULL, CV_SAVE, CV_OnOff, NULL);

INT32 cv_debug;

consvar_t cv_usemouse = CVAR_INIT ("use_mouse", "On", NULL, CV_SAVE|CV_CALL,usemouse_cons_t, I_StartupMouse);
consvar_t cv_usemouse2 = CVAR_INIT ("use_mouse2", "Off", NULL, CV_SAVE|CV_CALL,usemouse_cons_t, I_StartupMouse2);

#if defined (DC) || defined (_XBOX) || defined (WMINPUT) || defined (_WII) || defined(HAVE_SDL) || defined(_WINDOWS) //joystick 1 and 2
consvar_t cv_usejoystick = CVAR_INIT ("use_joystick", "1", NULL, CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick);
consvar_t cv_usejoystick2 = CVAR_INIT ("use_joystick2", "2", NULL, CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick2);
#elif defined (PSP) || defined (GP2X) || defined (_NDS) //only one joystick
consvar_t cv_usejoystick = CVAR_INIT ("use_joystick", "1", NULL, CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick);
consvar_t cv_usejoystick2 = CVAR_INIT ("use_joystick2", "0", NULL, CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick2);
#else //all esle, no joystick
consvar_t cv_usejoystick = CVAR_INIT ("use_joystick", "0", NULL, CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick);
consvar_t cv_usejoystick2 = CVAR_INIT ("use_joystick2", "0", NULL, CV_SAVE|CV_CALL, usejoystick_cons_t,
	I_InitJoystick2);
#endif
#if (defined (LJOYSTICK) || defined (HAVE_SDL))
#ifdef LJOYSTICK
consvar_t cv_joyport = CVAR_INIT ("joyport", "/dev/js0", NULL, CV_SAVE, joyport_cons_t, NULL);
consvar_t cv_joyport2 = CVAR_INIT ("joyport2", "/dev/js0", NULL, CV_SAVE, joyport_cons_t, NULL); //Alam: for later
#endif
consvar_t cv_joyscale = CVAR_INIT ("joyscale", "1", NULL, CV_SAVE|CV_CALL, NULL, I_JoyScale);
consvar_t cv_joyscale2 = CVAR_INIT ("joyscale2", "1", NULL, CV_SAVE|CV_CALL, NULL, I_JoyScale2);
#else
consvar_t cv_joyscale = CVAR_INIT ("joyscale", "1", NULL, CV_SAVE|CV_HIDEN, NULL, NULL); //Alam: Dummy for save
consvar_t cv_joyscale2 = CVAR_INIT ("joyscale2", "1", NULL, CV_SAVE|CV_HIDEN, NULL, NULL); //Alam: Dummy for save
#endif
#if (defined (__unix__) && !defined (MSDOS)) || defined(__APPLE__) || defined (UNIXCOMMON)
consvar_t cv_mouse2port = CVAR_INIT ("mouse2port", "/dev/gpmdata", NULL, CV_SAVE, mouse2port_cons_t, NULL);
consvar_t cv_mouse2opt = CVAR_INIT ("mouse2opt", "0", NULL, CV_SAVE, NULL, NULL);
#else
consvar_t cv_mouse2port = CVAR_INIT ("mouse2port", "COM2", NULL, CV_SAVE, mouse2port_cons_t, NULL);
#endif

consvar_t cv_matchboxes = CVAR_INIT ("matchboxes", "Normal", NULL, CV_SAVE|CV_NETVAR|CV_CHEAT, matchboxes_cons_t, NULL);
consvar_t cv_specialrings = CVAR_INIT ("specialrings", "On", NULL, CV_NETVAR, CV_OnOff, NULL);
consvar_t cv_powerstones = CVAR_INIT ("powerstones", "On", NULL, CV_NETVAR, CV_OnOff, NULL);

consvar_t cv_recycler =      CVAR_INIT ("tv_recycler",      "5", NULL, CV_NETVAR|CV_CHEAT, chances_cons_t, NULL);
consvar_t cv_teleporters =   CVAR_INIT("tv_teleporter",    "5", NULL, CV_NETVAR|CV_CHEAT, chances_cons_t, NULL);
consvar_t cv_superring =     CVAR_INIT("tv_superring",     "5", NULL, CV_NETVAR|CV_CHEAT, chances_cons_t, NULL);
consvar_t cv_supersneakers = CVAR_INIT ("tv_supersneaker",  "5", NULL, CV_NETVAR|CV_CHEAT, chances_cons_t, NULL);
consvar_t cv_invincibility = CVAR_INIT ("tv_invincibility", "5", NULL, CV_NETVAR|CV_CHEAT, chances_cons_t, NULL);
consvar_t cv_jumpshield =    CVAR_INIT("tv_jumpshield",    "5", NULL, CV_NETVAR|CV_CHEAT, chances_cons_t, NULL);
consvar_t cv_watershield =   CVAR_INIT("tv_watershield",   "5", NULL, CV_NETVAR|CV_CHEAT, chances_cons_t, NULL);
consvar_t cv_ringshield =    CVAR_INIT("tv_ringshield",    "5", NULL, CV_NETVAR|CV_CHEAT, chances_cons_t, NULL);
consvar_t cv_forceshield =   CVAR_INIT("tv_forceshield",   "5", NULL, CV_NETVAR|CV_CHEAT, chances_cons_t, NULL);
consvar_t cv_bombshield =    CVAR_INIT("tv_bombshield",    "5", NULL, CV_NETVAR|CV_CHEAT, chances_cons_t, NULL);
consvar_t cv_1up =           CVAR_INIT("tv_1up",           "5", NULL, CV_NETVAR|CV_CHEAT, chances_cons_t, NULL);
consvar_t cv_eggmanbox =     CVAR_INIT("tv_eggman",        "5", NULL, CV_NETVAR|CV_CHEAT, chances_cons_t, NULL);

consvar_t cv_ringslinger = CVAR_INIT ("ringslinger", "No", NULL, CV_NETVAR|CV_NOSHOWHELP|CV_CALL|CV_CHEAT, CV_YesNo,
	Ringslinger_OnChange);
consvar_t cv_gravity = CVAR_INIT ("gravity", "0.5", NULL, CV_RESTRICT|CV_FLOAT|CV_CALL, NULL, Gravity_OnChange);

consvar_t cv_soundtest = CVAR_INIT ("soundtest", "0", NULL, CV_CALL, NULL, SoundTest_OnChange);

static CV_PossibleValue_t minitimelimit_cons_t[] = {{15, "MIN"}, {9999, "MAX"}, {0, NULL}};
consvar_t cv_countdowntime = CVAR_INIT ("countdowntime", "60", NULL, CV_NETVAR|CV_CHEAT, minitimelimit_cons_t, NULL);

consvar_t cv_touchtag = CVAR_INIT ("touchtag", "Off", NULL, CV_NETVAR, CV_OnOff, NULL);
consvar_t cv_hidetime = CVAR_INIT ("hidetime", "30", NULL, CV_NETVAR|CV_CALL, minitimelimit_cons_t, Hidetime_OnChange);

consvar_t cv_autobalance = CVAR_INIT ("autobalance", "0", NULL, CV_NETVAR|CV_CALL, autobalance_cons_t, AutoBalance_OnChange);
consvar_t cv_teamscramble = CVAR_INIT ("teamscramble", "Off", NULL, CV_NETVAR|CV_CALL|CV_NOINIT, teamscramble_cons_t, TeamScramble_OnChange);
consvar_t cv_scrambleonchange = CVAR_INIT ("scrambleonchange", "Off", NULL, CV_NETVAR, teamscramble_cons_t, NULL);

consvar_t cv_friendlyfire = CVAR_INIT ("friendlyfire", "Off", NULL, CV_NETVAR, CV_OnOff, NULL);
consvar_t cv_itemfinder = CVAR_INIT ("itemfinder", "Off", NULL, CV_CALL, CV_OnOff, ItemFinder_OnChange);

// Scoring type options
consvar_t cv_match_scoring = CVAR_INIT ("matchscoring", "Normal", NULL, CV_NETVAR|CV_CHEAT, match_scoring_cons_t, NULL);
consvar_t cv_overtime = CVAR_INIT ("overtime", "Yes", NULL, CV_NETVAR, CV_YesNo, NULL);

consvar_t cv_rollingdemos = CVAR_INIT ("rollingdemos", "On", "Show demos on the title screen when inactive for some time", CV_SAVE, CV_OnOff, NULL);

consvar_t cv_timetic = CVAR_INIT ("timerres", "Normal", "Style of HUD time display", CV_SAVE, timetic_cons_t, NULL); // use tics in display
consvar_t cv_scorepos = CVAR_INIT ("scorepos", "Normal", "Style of HUD score display", CV_SAVE, scorepos_cons_t, NULL); // for score position
static CV_PossibleValue_t pointlimit_cons_t[] = {{0, "MIN"}, {999999990, "MAX"}, {0, NULL}};
consvar_t cv_pointlimit = CVAR_INIT ("pointlimit", "0", NULL, CV_NETVAR|CV_CALL|CV_NOINIT, pointlimit_cons_t,
	PointLimit_OnChange);
static CV_PossibleValue_t timelimit_cons_t[] = {{0, "MIN"}, {30, "MAX"}, {0, NULL}};
consvar_t cv_timelimit = CVAR_INIT ("timelimit", "0", NULL, CV_NETVAR|CV_CALL|CV_NOINIT, timelimit_cons_t,
	TimeLimit_OnChange);
static CV_PossibleValue_t numlaps_cons_t[] = {{0, "MIN"}, {50, "MAX"}, {0, NULL}};
consvar_t cv_numlaps = CVAR_INIT ("numlaps", "4", NULL, CV_NETVAR|CV_CALL|CV_NOINIT, numlaps_cons_t,
	NumLaps_OnChange);
consvar_t cv_usemapnumlaps = CVAR_INIT ("usemaplaps", "Yes", NULL, CV_NETVAR, CV_YesNo, NULL);

// log elemental hazards -- not a netvar, is local to current player
consvar_t cv_hazardlog = CVAR_INIT ("hazardlog", "Yes", "Whether or not to log when a player gets hurt in certain gamemodes", 0, CV_YesNo, NULL);

consvar_t cv_forceskin = CVAR_INIT ("forceskin", "-1", NULL, CV_NETVAR|CV_CALL|CV_CHEAT, NULL, ForceSkin_OnChange);
consvar_t cv_downloading = CVAR_INIT ("downloading", "On", NULL, 0, CV_OnOff, NULL);
consvar_t cv_allowexitlevel = CVAR_INIT ("allowexitlevel", "No", NULL, CV_NETVAR, CV_YesNo, NULL);

consvar_t cv_killingdead = CVAR_INIT ("killingdead", "Off", NULL, CV_NETVAR, CV_OnOff, NULL);

consvar_t cv_netstat = CVAR_INIT ("netstat", "Off", NULL, 0, CV_OnOff, NULL); // show bandwidth statistics
static CV_PossibleValue_t nettimeout_cons_t[] = {{TICRATE/7, "MIN"}, {60*TICRATE, "MAX"}, {0, NULL}};
consvar_t cv_nettimeout = CVAR_INIT ("nettimeout", "350", NULL, CV_CALL|CV_SAVE, nettimeout_cons_t, NetTimeout_OnChange);
static CV_PossibleValue_t jointimeout_cons_t[] = {{5*TICRATE, "MIN"}, {60*TICRATE, "MAX"}, {0, NULL}};
consvar_t cv_jointimeout = CVAR_INIT ("jointimeout", "350", NULL, CV_CALL|CV_SAVE, jointimeout_cons_t, JoinTimeout_OnChange);
consvar_t cv_maxping = CVAR_INIT ("maxping", "0", NULL, CV_SAVE, CV_Unsigned, NULL);

static CV_PossibleValue_t pingtimeout_cons_t[] = {{8, "MIN"}, {120, "MAX"}, {0, NULL}};
consvar_t cv_pingtimeout = CVAR_INIT ("pingtimeout", "10", NULL, CV_SAVE, pingtimeout_cons_t, NULL);

// show your ping on the HUD next to framerate. Defaults to warning only (shows up if your ping is > maxping)
static CV_PossibleValue_t showping_cons_t[] = {{0, "Off"}, {1, "Always"}, {2, "Warning"}, {0, NULL}};
consvar_t cv_showping = CVAR_INIT ("showping", "Warning", "Show your ping in the corner of the screen", CV_SAVE, showping_cons_t, NULL);
static CV_PossibleValue_t pingmeasurement_cons_t[] = {{0, "Frames"}, {1, "Milliseconds"}, {0, NULL}};
consvar_t cv_pingmeasurement = CVAR_INIT ("pingmeasurement", "Milliseconds", NULL, CV_SAVE, pingmeasurement_cons_t, NULL);

// Intermission time Tails 04-19-2002
static CV_PossibleValue_t inttime_cons_t[] = {{0, "MIN"}, {3600, "MAX"}, {0, NULL}};
consvar_t cv_inttime = CVAR_INIT ("inttime", "10", NULL, CV_NETVAR|CV_SAVE, inttime_cons_t, NULL);

static CV_PossibleValue_t advancemap_cons_t[] = {{0, "Off"}, {1, "Next"}, {2, "Random"}, {0, NULL}};
consvar_t cv_advancemap = CVAR_INIT ("advancemap", "Next", NULL, CV_NETVAR, advancemap_cons_t, NULL);
static CV_PossibleValue_t playersforexit_cons_t[] = {{0, "One"}, {1, "All"}, {0, NULL}};
consvar_t cv_playersforexit = CVAR_INIT ("playersforexit", "One", NULL, CV_NETVAR, playersforexit_cons_t, NULL);

consvar_t cv_runscripts = CVAR_INIT ("runscripts", "Yes", NULL, 0, CV_YesNo, NULL);

consvar_t cv_pause = CVAR_INIT ("pausepermission", "Server", NULL, CV_NETVAR, pause_cons_t, NULL);
consvar_t cv_mute = CVAR_INIT ("mute", "Off", NULL, CV_NETVAR|CV_CALL, CV_OnOff, Mute_OnChange);

consvar_t cv_sleep = CVAR_INIT ("cpusleep", "1", NULL, CV_SAVE, sleeping_cons_t, NULL);
consvar_t cv_freedemocamera = CVAR_INIT ("freedemocamera", "Off", NULL, CV_SAVE, CV_OnOff, NULL);

static CV_PossibleValue_t perfstats_cons_t[] = {
	{0, "Off"}, {1, "Rendering"}, {2, "Logic"}, {3, "ThinkFrame"}, {0, NULL}};
consvar_t cv_perfstats = CVAR_INIT ("perfstats", "Off", NULL, CV_CALL, perfstats_cons_t, PS_PerfStats_OnChange);
static CV_PossibleValue_t ps_samplesize_cons_t[] = {
	{1, "MIN"}, {1000, "MAX"}, {0, NULL}};
consvar_t cv_ps_samplesize = CVAR_INIT ("ps_samplesize", "1", NULL, CV_CALL, ps_samplesize_cons_t, PS_SampleSize_OnChange);
static CV_PossibleValue_t ps_descriptor_cons_t[] = {
	{1, "Average"}, {2, "SD"}, {3, "Minimum"}, {4, "Maximum"}, {0, NULL}};
consvar_t cv_ps_descriptor = CVAR_INIT ("ps_descriptor", "Average", NULL, 0, ps_descriptor_cons_t, NULL);

consvar_t cv_lastserver = CVAR_INIT ("lastserver", "", NULL, CV_SAVE, NULL, NULL);


INT16 gametype = GT_COOP;
boolean splitscreen = false;
boolean circuitmap = false;
INT32 adminplayers[MAXPLAYERS];

/// \warning Keep this up-to-date if you add/remove/rename net text commands
const char *netxcmdnames[MAXNETXCMD - 1] =
{
	"NAMEANDCOLOR",
	"WEAPONPREF",
	"KICK",
	"NETVAR",
	"SAY",
	"MAP",
	"EXITLEVEL",
	"ADDFILE",
	"PAUSE",
	"ADDPLAYER",
	"TEAMCHANGE",
	"CLEARSCORES",
	"LOGIN",
	"VERIFIED",
	"RANDOMSEED",
	"RUNSOC",
	"REQADDFILE",
	"DELFILE",
	"SETMOTD",
	"SUICIDE",
	"LUACMD",
	"LUAVAR",
	"LUAFILE"
};

// =========================================================================
//                           SERVER STARTUP
// =========================================================================

/** Registers server commands and variables.
  * Anything required by a dedicated server should probably go here.
  *
  * \sa D_RegisterClientCommands
  */
void D_RegisterServerCommands(void)
{
	INT32 i;

	for (i = 0; i < NUMGAMETYPES; i++)
	{
		gametype_cons_t[i].value = i;
		gametype_cons_t[i].strvalue = Gametype_Names[i];
	}
	gametype_cons_t[NUMGAMETYPES].value = 0;
	gametype_cons_t[NUMGAMETYPES].strvalue = NULL;

	RegisterNetXCmd(XD_NAMEANDCOLOR, Got_NameAndColor);
	RegisterNetXCmd(XD_WEAPONPREF, Got_WeaponPref);
	RegisterNetXCmd(XD_MAP, Got_Mapcmd);
	RegisterNetXCmd(XD_EXITLEVEL, Got_ExitLevelcmd);
	RegisterNetXCmd(XD_ADDFILE, Got_Addfilecmd);
	RegisterNetXCmd(XD_REQADDFILE, Got_RequestAddfilecmd);
#ifdef DELFILE
	RegisterNetXCmd(XD_DELFILE, Got_Delfilecmd);
#endif
	RegisterNetXCmd(XD_PAUSE, Got_Pause);
	RegisterNetXCmd(XD_SUICIDE, Got_Suicide);
	RegisterNetXCmd(XD_RUNSOC, Got_RunSOCcmd);
	RegisterNetXCmd(XD_LUACMD, Got_Luacmd);
	RegisterNetXCmd(XD_LUAFILE, Got_LuaFile);

	// Remote Administration
	COM_AddCommand("password", NULL, Command_Changepassword_f);
	COM_AddCommand("login", NULL, Command_Login_f); // useful in dedicated to kick off remote admin
	COM_AddCommand("promote", NULL, Command_Verify_f);
	RegisterNetXCmd(XD_VERIFIED, Got_Verification);
	COM_AddCommand("demote", NULL, Command_RemoveAdmin_f);
	RegisterNetXCmd(XD_DEMOTED, Got_Removal);

	COM_AddCommand("motd", NULL, Command_MotD_f);
	RegisterNetXCmd(XD_SETMOTD, Got_MotD_f); // For remote admin

	RegisterNetXCmd(XD_TEAMCHANGE, Got_Teamchange);
	COM_AddCommand("serverchangeteam", NULL, Command_ServerTeamChange_f);

	RegisterNetXCmd(XD_CLEARSCORES, Got_Clearscores);
	COM_AddCommand("clearscores", NULL, Command_Clearscores_f);
	COM_AddCommand("map", NULL, Command_Map_f);

	COM_AddCommand("exitgame", NULL, Command_ExitGame_f);
	COM_AddCommand("retry", NULL, Command_Retry_f);
	COM_AddCommand("exitlevel", NULL, Command_ExitLevel_f);
	COM_AddCommand("showmap", NULL, Command_Showmap_f);
	COM_AddCommand("mapmd5", NULL, Command_Mapmd5_f);

	COM_AddCommand("addfile", NULL, Command_Addfile);
	COM_AddCommand("listwad", NULL, Command_ListWADS_f);

#ifdef DELFILE
	COM_AddCommand("delfile", NULL, Command_Delfile);
#endif
	COM_AddCommand("runsoc", NULL, Command_RunSOC);
	COM_AddCommand("pause", NULL, Command_Pause);
	COM_AddCommand("suicide", NULL, Command_Suicide);

	COM_AddCommand("gametype", NULL, Command_ShowGametype_f);
	COM_AddCommand("version", NULL, Command_Version_f);
#ifdef UPDATE_ALERT
	COM_AddCommand("mod_details", NULL, Command_ModDetails_f);
#endif
	COM_AddCommand("quit", NULL, Command_Quit_f);

	COM_AddCommand("saveconfig", NULL, Command_SaveConfig_f);
	COM_AddCommand("loadconfig", NULL, Command_LoadConfig_f);
	COM_AddCommand("changeconfig", NULL, Command_ChangeConfig_f);
	COM_AddCommand("isgamemodified", NULL, Command_Isgamemodified_f); // test
	COM_AddCommand("showscores", NULL, Command_ShowScores_f);
	COM_AddCommand("showtime", NULL, Command_ShowTime_f);
	COM_AddCommand("cheats", NULL, Command_Cheats_f); // test
#ifdef _DEBUG
	COM_AddCommand("togglemodified", NULL, Command_Togglemodified_f);
	COM_AddCommand("archivetest", NULL, Command_Archivetest_f);
#endif

	// for master server connection
	AddMServCommands();

	// p_mobj.c
	CV_RegisterVar(&cv_itemrespawntime);
	CV_RegisterVar(&cv_itemrespawn);
	CV_RegisterVar(&cv_flagtime);
	CV_RegisterVar(&cv_suddendeath);

	// misc
	CV_RegisterVar(&cv_friendlyfire);
	CV_RegisterVar(&cv_pointlimit);
	CV_RegisterVar(&cv_numlaps);
	CV_RegisterVar(&cv_usemapnumlaps);

	CV_RegisterVar(&cv_hazardlog);

	CV_RegisterVar(&cv_autobalance);
	CV_RegisterVar(&cv_teamscramble);
	CV_RegisterVar(&cv_scrambleonchange);

	CV_RegisterVar(&cv_touchtag);
	CV_RegisterVar(&cv_hidetime);

	CV_RegisterVar(&cv_inttime);
	CV_RegisterVar(&cv_advancemap);
	CV_RegisterVar(&cv_playersforexit);
	CV_RegisterVar(&cv_timelimit);
	CV_RegisterVar(&cv_playbackspeed);
	CV_RegisterVar(&cv_forceskin);
	CV_RegisterVar(&cv_downloading);

	CV_RegisterVar(&cv_specialrings);
	CV_RegisterVar(&cv_powerstones);
	CV_RegisterVar(&cv_competitionboxes);
	CV_RegisterVar(&cv_matchboxes);

	CV_RegisterVar(&cv_recycler);
	CV_RegisterVar(&cv_teleporters);
	CV_RegisterVar(&cv_superring);
	CV_RegisterVar(&cv_supersneakers);
	CV_RegisterVar(&cv_invincibility);
	CV_RegisterVar(&cv_jumpshield);
	CV_RegisterVar(&cv_watershield);
	CV_RegisterVar(&cv_ringshield);
	CV_RegisterVar(&cv_forceshield);
	CV_RegisterVar(&cv_bombshield);
	CV_RegisterVar(&cv_1up);
	CV_RegisterVar(&cv_eggmanbox);

	CV_RegisterVar(&cv_ringslinger);

	CV_RegisterVar(&cv_startinglives);
	CV_RegisterVar(&cv_countdowntime);
	CV_RegisterVar(&cv_runscripts);
	CV_RegisterVar(&cv_match_scoring);
	CV_RegisterVar(&cv_overtime);
	CV_RegisterVar(&cv_pause);
	CV_RegisterVar(&cv_mute);

	RegisterNetXCmd(XD_RANDOMSEED, Got_RandomSeed);

	CV_RegisterVar(&cv_allowexitlevel);
	CV_RegisterVar(&cv_restrictskinchange);
	CV_RegisterVar(&cv_allowteamchange);
	CV_RegisterVar(&cv_respawntime);
	CV_RegisterVar(&cv_killingdead);

	// d_clisrv
	CV_RegisterVar(&cv_maxplayers);
	CV_RegisterVar(&cv_maxsend);
	CV_RegisterVar(&cv_noticedownload);
	CV_RegisterVar(&cv_downloadspeed);
	CV_RegisterVar(&cv_dedicatedidletime);
	CV_RegisterVar(&cv_allowgamestateresend);

	COM_AddCommand("ping", NULL, Command_Ping_f);
	CV_RegisterVar(&cv_nettimeout);
	CV_RegisterVar(&cv_jointimeout);

	CV_RegisterVar(&cv_skipmapcheck);
	CV_RegisterVar(&cv_sleep);
	CV_RegisterVar(&cv_maxping);
	CV_RegisterVar(&cv_pingtimeout);
	CV_RegisterVar(&cv_showping);
	CV_RegisterVar(&cv_pingmeasurement);

#ifdef SEENAMES
	 CV_RegisterVar(&cv_allowseenames);
#endif

	CV_RegisterVar(&cv_dummyconsvar);
}

// =========================================================================
//                           CLIENT STARTUP
// =========================================================================

/** Registers client commands and variables.
  * Nothing needed for a dedicated server should be registered here.
  *
  * \sa D_RegisterServerCommands
  */
void D_RegisterClientCommands(void)
{
	INT32 i;

	for (i = 0; i < MAXSKINCOLORS; i++)
	{
		Color_cons_t[i].value = i;
		Color_cons_t[i].strvalue = skincolors[i].name;
	}
	Color_cons_t[MAXSKINCOLORS].value = 0;
	Color_cons_t[MAXSKINCOLORS].strvalue = NULL;

	if (dedicated)
		return;

	COM_AddCommand("numthinkers", NULL, Command_Numthinkers_f);
	COM_AddCommand("countmobjs", NULL, Command_CountMobjs_f);

	COM_AddCommand("changeteam", NULL, Command_Teamchange_f);
	COM_AddCommand("changeteam2", NULL, Command_Teamchange2_f);

	COM_AddCommand("playdemo", NULL, Command_Playdemo_f);
	COM_AddCommand("timedemo", NULL, Command_Timedemo_f);
	COM_AddCommand("stopdemo", NULL, Command_Stopdemo_f);
	COM_AddCommand("playintro", NULL, Command_Playintro_f);

	COM_AddCommand("resetcamera", NULL, Command_ResetCamera_f);

	COM_AddCommand("setcontrol", NULL, Command_Setcontrol_f);
	COM_AddCommand("setcontrol2", NULL, Command_Setcontrol2_f);

	COM_AddCommand("screenshot", NULL, M_ScreenShot);
	COM_AddCommand("startmovie", NULL, Command_StartMovie_f);
	COM_AddCommand("stopmovie", NULL, Command_StopMovie_f);

	CV_RegisterVar(&cv_screenshot_option);
	CV_RegisterVar(&cv_screenshot_folder);
	CV_RegisterVar(&cv_moviemode);
	CV_RegisterVar(&cv_movie_option);
	CV_RegisterVar(&cv_movie_folder);
	// PNG variables
	CV_RegisterVar(&cv_zlib_level);
	CV_RegisterVar(&cv_zlib_memory);
	CV_RegisterVar(&cv_zlib_strategy);
	CV_RegisterVar(&cv_zlib_window_bits);
	// APNG variables
	CV_RegisterVar(&cv_zlib_levela);
	CV_RegisterVar(&cv_zlib_memorya);
	CV_RegisterVar(&cv_zlib_strategya);
	CV_RegisterVar(&cv_zlib_window_bitsa);
	CV_RegisterVar(&cv_apng_delay);
	// GIF variables
	CV_RegisterVar(&cv_gif_optimize);
	CV_RegisterVar(&cv_gif_downscale);

#ifdef WALLSPLATS
	CV_RegisterVar(&cv_splats);
#endif

	// register these so it is saved to config
	CV_RegisterVar(&cv_playername);
	CV_RegisterVar(&cv_playercolor);
	CV_RegisterVar(&cv_skin); // r_things.c (skin NAME)
	// secondary player (splitscreen)
	CV_RegisterVar(&cv_playername2);
	CV_RegisterVar(&cv_playercolor2);
	CV_RegisterVar(&cv_skin2);
	// saved versions of the above six
	CV_RegisterVar(&cv_defaultplayercolor);
	CV_RegisterVar(&cv_defaultskin);
	CV_RegisterVar(&cv_defaultplayercolor2);
	CV_RegisterVar(&cv_defaultskin2);

#ifdef SEENAMES
	CV_RegisterVar(&cv_seenames);
#endif
	CV_RegisterVar(&cv_rollingdemos);
	CV_RegisterVar(&cv_netstat);
	CV_RegisterVar(&cv_netticbuffer);

#ifdef NETGAME_DEVMODE
	CV_RegisterVar(&cv_fishcake);
#endif

	// HUD
	CV_RegisterVar(&cv_timetic);
	CV_RegisterVar(&cv_scorepos);
	CV_RegisterVar(&cv_itemfinder);
	CV_RegisterVar(&cv_showinput);
	CV_RegisterVar(&cv_showinputjoy);

	// time attack ghost options are also saved to config
	CV_RegisterVar(&cv_ghost_bestscore);
	CV_RegisterVar(&cv_ghost_besttime);
	CV_RegisterVar(&cv_ghost_bestrings);
	CV_RegisterVar(&cv_ghost_last);
	CV_RegisterVar(&cv_ghost_guest);

	COM_AddCommand("displayplayer", NULL, Command_Displayplayer_f);

	// FIXME: not to be here.. but needs be done for config loading
	CV_RegisterVar(&cv_globalgamma);
	CV_RegisterVar(&cv_globalsaturation);

	CV_RegisterVar(&cv_rhue);
	CV_RegisterVar(&cv_yhue);
	CV_RegisterVar(&cv_ghue);
	CV_RegisterVar(&cv_chue);
	CV_RegisterVar(&cv_bhue);
	CV_RegisterVar(&cv_mhue);

	CV_RegisterVar(&cv_rgamma);
	CV_RegisterVar(&cv_ygamma);
	CV_RegisterVar(&cv_ggamma);
	CV_RegisterVar(&cv_cgamma);
	CV_RegisterVar(&cv_bgamma);
	CV_RegisterVar(&cv_mgamma);

	CV_RegisterVar(&cv_rsaturation);
	CV_RegisterVar(&cv_ysaturation);
	CV_RegisterVar(&cv_gsaturation);
	CV_RegisterVar(&cv_csaturation);
	CV_RegisterVar(&cv_bsaturation);
	CV_RegisterVar(&cv_msaturation);

	// m_menu.c
	CV_RegisterVar(&cv_compactscoreboard);
	CV_RegisterVar(&cv_chatheight);
	CV_RegisterVar(&cv_chatwidth);
	CV_RegisterVar(&cv_chattime);
	CV_RegisterVar(&cv_chatspamprotection);
	CV_RegisterVar(&cv_chatbacktint);
	CV_RegisterVar(&cv_consolechat);
	CV_RegisterVar(&cv_chatnotifications);
	CV_RegisterVar(&cv_crosshair);
	CV_RegisterVar(&cv_crosshair2);
	CV_RegisterVar(&cv_alwaysfreelook);
	CV_RegisterVar(&cv_alwaysfreelook2);
	CV_RegisterVar(&cv_chasefreelook);
	CV_RegisterVar(&cv_chasefreelook2);
	CV_RegisterVar(&cv_showfocuslost);

	// g_input.c
	CV_RegisterVar(&cv_sideaxis);
	CV_RegisterVar(&cv_sideaxis2);
	CV_RegisterVar(&cv_turnaxis);
	CV_RegisterVar(&cv_turnaxis2);
	CV_RegisterVar(&cv_moveaxis);
	CV_RegisterVar(&cv_moveaxis2);
	CV_RegisterVar(&cv_lookaxis);
	CV_RegisterVar(&cv_lookaxis2);
	CV_RegisterVar(&cv_jumpaxis);
	CV_RegisterVar(&cv_jumpaxis2);
	CV_RegisterVar(&cv_spinaxis);
	CV_RegisterVar(&cv_spinaxis2);
	CV_RegisterVar(&cv_fireaxis);
	CV_RegisterVar(&cv_fireaxis2);
	CV_RegisterVar(&cv_firenaxis);
	CV_RegisterVar(&cv_firenaxis2);

	// filesrch.c
	CV_RegisterVar(&cv_addons_option);
	CV_RegisterVar(&cv_addons_folder);
	CV_RegisterVar(&cv_addons_md5);
	CV_RegisterVar(&cv_addons_showall);
	CV_RegisterVar(&cv_addons_search_type);
	CV_RegisterVar(&cv_addons_search_case);

	// WARNING: the order is important when initialising mouse2
	// we need the mouse2port
	CV_RegisterVar(&cv_mouse2port);
#if (defined (__unix__) && !defined (MSDOS)) || defined(__APPLE__) || defined (UNIXCOMMON)
	CV_RegisterVar(&cv_mouse2opt);
#endif
	CV_RegisterVar(&cv_controlperkey);

	CV_RegisterVar(&cv_usemouse);
	CV_RegisterVar(&cv_usemouse2);
	CV_RegisterVar(&cv_invertmouse);
	CV_RegisterVar(&cv_invertmouse2);
	CV_RegisterVar(&cv_mousesens);
	CV_RegisterVar(&cv_mousesens2);
	CV_RegisterVar(&cv_mouseysens);
	CV_RegisterVar(&cv_mouseysens2);
	CV_RegisterVar(&cv_mousemove);
	CV_RegisterVar(&cv_mousemove2);

	CV_RegisterVar(&cv_usejoystick);
	CV_RegisterVar(&cv_usejoystick2);
#ifdef LJOYSTICK
	CV_RegisterVar(&cv_joyport);
	CV_RegisterVar(&cv_joyport2);
#endif
	CV_RegisterVar(&cv_joyscale);
	CV_RegisterVar(&cv_joyscale2);

	// Analog Control
	CV_RegisterVar(&cv_analog);
	CV_RegisterVar(&cv_analog2);
	CV_RegisterVar(&cv_useranalog);
	CV_RegisterVar(&cv_useranalog2);

	// s_sound.c
	CV_RegisterVar(&cv_soundvolume);
	CV_RegisterVar(&cv_digmusicvolume);
	CV_RegisterVar(&cv_midimusicvolume);
	CV_RegisterVar(&cv_numChannels);

	// screen.c
	CV_RegisterVar(&cv_fullscreen);
	CV_RegisterVar(&cv_renderview);
	CV_RegisterVar(&cv_renderer);
	CV_RegisterVar(&cv_scr_depth);
	CV_RegisterVar(&cv_scr_width);
	CV_RegisterVar(&cv_scr_height);
	CV_RegisterVar(&cv_scr_width_w);
	CV_RegisterVar(&cv_scr_height_w);

	CV_RegisterVar(&cv_soundtest);

	CV_RegisterVar(&cv_perfstats);
	CV_RegisterVar(&cv_ps_samplesize);
	CV_RegisterVar(&cv_ps_descriptor);

	// ingame object placing
	COM_AddCommand("objectplace", NULL, Command_ObjectPlace_f);
	COM_AddCommand("writethings", NULL, Command_Writethings_f);
	CV_RegisterVar(&cv_speed);
	CV_RegisterVar(&cv_opflags);
	CV_RegisterVar(&cv_mapthingnum);
//	CV_RegisterVar(&cv_grid);
//	CV_RegisterVar(&cv_snapto);

	CV_RegisterVar(&cv_freedemocamera);
	CV_RegisterVar(&cv_lastserver);

	// add cheat commands
	COM_AddCommand("noclip", NULL, Command_CheatNoClip_f);
	COM_AddCommand("god", NULL, Command_CheatGod_f);
	COM_AddCommand("notarget", NULL, Command_CheatNoTarget_f);
	COM_AddCommand("getallemeralds", NULL, Command_Getallemeralds_f);
	COM_AddCommand("resetemeralds", NULL, Command_Resetemeralds_f);
	COM_AddCommand("setrings", NULL, Command_Setrings_f);
	COM_AddCommand("setlives", NULL, Command_Setlives_f);
	COM_AddCommand("setcontinues", NULL, Command_Setcontinues_f);
	COM_AddCommand("devmode", NULL, Command_Devmode_f);
	COM_AddCommand("savecheckpoint", NULL, Command_Savecheckpoint_f);
	COM_AddCommand("scale", NULL, Command_Scale_f);
	COM_AddCommand("gravflip", NULL, Command_Gravflip_f);
	COM_AddCommand("hurtme", NULL, Command_Hurtme_f);
	COM_AddCommand("jumptoaxis", NULL, Command_JumpToAxis_f);
	COM_AddCommand("charability", NULL, Command_Charability_f);
	COM_AddCommand("charspeed", NULL, Command_Charspeed_f);
	COM_AddCommand("teleport", NULL, Command_Teleport_f);
	COM_AddCommand("rteleport", NULL, Command_RTeleport_f);
	COM_AddCommand("skynum", NULL, Command_Skynum_f);
	COM_AddCommand("weather", NULL, Command_Weather_f);
	COM_AddCommand("toggletwod", NULL, Command_Toggletwod_f);
#ifdef _DEBUG
	COM_AddCommand("causecfail", NULL, Command_CauseCfail_f);
#endif
#if defined(LUA_ALLOW_BYTECODE)
	COM_AddCommand("dumplua", NULL, Command_Dumplua_f);
#endif
}

/** Checks if a name (as received from another player) is okay.
  * A name is okay if it is no fewer than 1 and no more than ::MAXPLAYERNAME
  * chars long (not including NUL), it does not begin or end with a space,
  * it does not contain non-printing characters (according to isprint(), which
  * allows space), it does not start with a digit, and no other player is
  * currently using it.
  * \param name      Name to check.
  * \param playernum Player who wants the name, so we can check if they already
  *                  have it, and let them keep it if so.
  * \sa CleanupPlayerName, SetPlayerName, Got_NameAndColor
  * \author Graue <graue@oceanbase.org>
  */
static boolean IsNameGood(char *name, INT32 playernum)
{
	INT32 ix;

	if (strlen(name) == 0 || strlen(name) > MAXPLAYERNAME)
		return false; // Empty or too long.
	if (name[0] == ' ' || name[strlen(name)-1] == ' ')
		return false; // Starts or ends with a space.
	if (isdigit(name[0]))
		return false; // Starts with a digit.
	if (name[0] == '@' || name[0] == '~')
		return false; // Starts with an admin symbol.

	// Check if it contains a non-printing character.
	// Note: ANSI C isprint() considers space a printing character.
	// Also don't allow semicolons, since they are used as
	// console command separators.

	// Also, anything over 0x80 is disallowed too, since compilers love to
	// differ on whether they're printable characters or not.
	for (ix = 0; name[ix] != '\0'; ix++)
		if (!isprint(name[ix]) || name[ix] == ';' || (UINT8)(name[ix]) >= 0x80)
			return false;

	// Check if a player is currently using the name, case-insensitively.
	for (ix = 0; ix < MAXPLAYERS; ix++)
	{
		if (ix != playernum && playeringame[ix]
			&& strcasecmp(name, player_names[ix]) == 0)
		{
			// We shouldn't kick people out just because
			// they joined the game with the same name
			// as someone else -- modify the name instead.
			size_t len = strlen(name);

			// Recursion!
			// Slowly strip characters off the end of the
			// name until we no longer have a duplicate.
			if (len > 1)
			{
				name[len-1] = '\0';
				if (!IsNameGood (name, playernum))
					return false;
			}
			else if (len == 1) // Agh!
			{
				// Last ditch effort...
				sprintf(name, "%d", M_RandomKey(10));
				if (!IsNameGood (name, playernum))
					return false;
			}
			else
				return false;
		}
	}

	return true;
}

/** Cleans up a local player's name before sending a name change.
  * Spaces at the beginning or end of the name are removed. Then if the new
  * name is identical to another player's name, ignoring case, the name change
  * is canceled, and the name in cv_playername.value or cv_playername2.value
  * is restored to what it was before.
  *
  * We assume that if playernum is ::consoleplayer or ::secondarydisplayplayer
  * the console variable ::cv_playername or ::cv_playername2 respectively is
  * already set to newname. However, the player name table is assumed to
  * contain the old name.
  *
  * \param playernum Player number who has requested a name change.
  *                  Should be ::consoleplayer or ::secondarydisplayplayer.
  * \param newname   New name for that player; should already be in
  *                  ::cv_playername or ::cv_playername2 if player is the
  *                  console or secondary display player, respectively.
  * \sa cv_playername, cv_playername2, SendNameAndColor, SendNameAndColor2,
  *     SetPlayerName
  * \author Graue <graue@oceanbase.org>
  */
static void CleanupPlayerName(INT32 playernum, const char *newname)
{
	char *buf;
	char *p;
	char *tmpname = NULL;
	INT32 i;
	boolean namefailed = true;

	buf = Z_StrDup(newname);

	do
	{
		p = buf;

		while (*p == ' ')
			p++; // remove leading spaces

		if (strlen(p) == 0)
			break; // empty names not allowed

		if (isdigit(*p))
			break; // names starting with digits not allowed

		if (*p == '@' || *p == '~')
			break; // names that start with @ or ~ (admin symbols) not allowed

		tmpname = p;

		// Remove trailing spaces.
		p = &tmpname[strlen(tmpname)-1]; // last character
		while (*p == ' ' && p >= tmpname)
		{
			*p = '\0';
			p--;
		}

		if (strlen(tmpname) == 0)
			break; // another case of an empty name

		// Truncate name if it's too long (max MAXPLAYERNAME chars
		// excluding NUL).
		if (strlen(tmpname) > MAXPLAYERNAME)
			tmpname[MAXPLAYERNAME] = '\0';

		// Remove trailing spaces again.
		p = &tmpname[strlen(tmpname)-1]; // last character
		while (*p == ' ' && p >= tmpname)
		{
			*p = '\0';
			p--;
		}

		// no stealing another player's name
		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (i != playernum && playeringame[i]
				&& strcasecmp(tmpname, player_names[i]) == 0)
			{
				break;
			}
		}

		if (i < MAXPLAYERS)
			break;

		// name is okay then
		namefailed = false;
	} while (0);

	if (namefailed)
		tmpname = player_names[playernum];

	// set consvars whether namefailed or not, because even if it succeeded,
	// spaces may have been removed
	if (playernum == consoleplayer)
		CV_StealthSet(&cv_playername, tmpname);
	else if (playernum == secondarydisplayplayer
		|| (!netgame && playernum == 1))
	{
		CV_StealthSet(&cv_playername2, tmpname);
	}
	else I_Assert(((void)"CleanupPlayerName used on non-local player", 0));

	Z_Free(buf);
}

/** Sets a player's name, if it is good.
  * If the name is not good (indicating a modified or buggy client), it is not
  * set, and if we are the server in a netgame, the player responsible is
  * kicked with a consistency failure.
  *
  * This function prints a message indicating the name change, unless the game
  * is currently showing the intro, e.g. when processing autoexec.cfg.
  *
  * \param playernum Player number who has requested a name change.
  * \param newname   New name for that player. Should be good, but won't
  *                  necessarily be if the client is maliciously modified or
  *                  buggy.
  * \sa CleanupPlayerName, IsNameGood
  * \author Graue <graue@oceanbase.org>
  */
static void SetPlayerName(INT32 playernum, char *newname)
{
	if (IsNameGood(newname, playernum))
	{
		if (strcasecmp(newname, player_names[playernum]) != 0)
		{
			if (netgame)
				HU_AddChatText(va("\x82*%s renamed to %s", player_names[playernum], newname), false);

			strcpy(player_names[playernum], newname);
		}
	}
	else
	{
		CONS_Printf(M_GetText("Player %d sent a bad name change\n"), playernum+1);
		if (server && netgame)
			SendKick(playernum, KICK_MSG_CON_FAIL);
	}
}

UINT8 CanChangeSkin(INT32 playernum)
{
	// Of course we can change if we're not playing
	if (!Playing() || !addedtogame)
		return true;

	// Force skin in effect.
	if (client && (cv_forceskin.value != -1) && !(IsPlayerAdmin(playernum) && serverplayer == -1))
		return false;

	// Can change skin in intermission and whatnot.
	if (gamestate != GS_LEVEL)
		return true;

	// Server has skin change restrictions.
	if (cv_restrictskinchange.value)
	{
		if (gametype == GT_COOP)
			return true;

		// Can change skin during initial countdown.
		if ((gametype == GT_RACE || gametype == GT_COMPETITION) && leveltime < 4*TICRATE)
			return true;

		if (G_TagGametype())
		{
			// Can change skin during hidetime.
			if (leveltime < hidetime * TICRATE)
				return true;

			// IT players can always change skins to persue players hiding in character only locations.
			if (players[playernum].pflags & PF_TAGIT)
				return true;
		}

		if (players[playernum].spectator || players[playernum].playerstate == PST_DEAD || players[playernum].playerstate == PST_REBORN)
			return true;

		return false;
	}

	return true;
}

static void ForceAllSkins(INT32 forcedskin)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; ++i)
	{
		if (!playeringame[i])
			continue;

		SetPlayerSkinByNum(i, forcedskin);

		// If it's me (or my brother), set appropriate skin value in cv_skin/cv_skin2
		if (!dedicated) // But don't do this for dedicated servers, of course.
		{
			if (i == consoleplayer)
				CV_StealthSet(&cv_skin, skins[forcedskin].name);
			else if (i == secondarydisplayplayer)
				CV_StealthSet(&cv_skin2, skins[forcedskin].name);
		}
	}
}

static INT32 snacpending = 0, snac2pending = 0, chmappending = 0;

// name, color, or skin has changed
//
static void SendNameAndColor(void)
{
	XBOXSTATIC char buf[MAXPLAYERNAME+2];
	char *p;

	p = buf;

	// normal player colors
	if (G_GametypeHasTeams())
	{
		if (players[consoleplayer].ctfteam == 1 && cv_playercolor.value != skincolor_redteam)
			CV_StealthSetValue(&cv_playercolor, skincolor_redteam);
		else if (players[consoleplayer].ctfteam == 2 && cv_playercolor.value != skincolor_blueteam)
			CV_StealthSetValue(&cv_playercolor, skincolor_blueteam);
	}

	// never allow inaccessible colors
	if (!cv_playercolor.value || !skincolors[cv_playercolor.value].accessible)
	{
		if (players[consoleplayer].skincolor && skincolors[cv_playercolor.value].accessible)
			CV_StealthSetValue(&cv_playercolor, players[consoleplayer].skincolor);
		else if (skins[players[consoleplayer].skin].prefcolor && skincolors[skins[players[consoleplayer].skin].prefcolor].accessible)
			CV_StealthSetValue(&cv_playercolor, skins[players[consoleplayer].skin].prefcolor);
		else
			CV_StealthSet(&cv_playercolor, cv_playercolor.defaultvalue);
	}

	if (!strcmp(cv_playername.string, player_names[consoleplayer])
	&& cv_playercolor.value == players[consoleplayer].skincolor
	&& !strcmp(cv_skin.string, skins[players[consoleplayer].skin].name))
		return;

	// We'll handle it later if we're not playing.
	if (!Playing())
		return;

	// If you're not in a netgame, merely update the skin, color, and name.
	if (!netgame)
	{
		INT32 foundskin;

		CleanupPlayerName(consoleplayer, cv_playername.zstring);
		strcpy(player_names[consoleplayer], cv_playername.zstring);

		players[consoleplayer].skincolor = cv_playercolor.value;

		if (players[consoleplayer].mo)
			players[consoleplayer].mo->color = players[consoleplayer].skincolor;

		if (metalrecording)
		{ // Metal Sonic is Sonic, obviously.
			SetPlayerSkinByNum(consoleplayer, 0);
			CV_StealthSet(&cv_skin, skins[0].name);
		}
		else if ((foundskin = R_SkinAvailable(cv_skin.string)) != -1)
		{
			boolean notsame;

			cv_skin.value = foundskin;

			notsame = (cv_skin.value != players[consoleplayer].skin);

			SetPlayerSkin(consoleplayer, cv_skin.string);
			CV_StealthSet(&cv_skin, skins[cv_skin.value].name);

			if (notsame)
			{
				CV_StealthSetValue(&cv_playercolor, skins[cv_skin.value].prefcolor);

				players[consoleplayer].skincolor = (cv_playercolor.value) % numskincolors;

				if (players[consoleplayer].mo)
					players[consoleplayer].mo->color = (UINT16)players[consoleplayer].skincolor;
			}
		}
		else
		{
			cv_skin.value = players[consoleplayer].skin;
			CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);
			// will always be same as current
			SetPlayerSkin(consoleplayer, cv_skin.string);
		}

		return;
	}

	snacpending++;

	// Don't change name if muted
	if (cv_mute.value && !(server || IsPlayerAdmin(consoleplayer)))
		CV_StealthSet(&cv_playername, player_names[consoleplayer]);
	else // Cleanup name if changing it
		CleanupPlayerName(consoleplayer, cv_playername.zstring);

	// Don't change skin if the server doesn't want you to.
	if (!CanChangeSkin(consoleplayer))
		CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);

	// check if player has the skin loaded (cv_skin may have
	// the name of a skin that was available in the previous game)
	cv_skin.value = R_SkinAvailable(cv_skin.string);
	if (cv_skin.value < 0)
	{
		CV_StealthSet(&cv_skin, DEFAULTSKIN);
		cv_skin.value = 0;
	}

	// Finally write out the complete packet and send it off.
	WRITESTRINGN(p, cv_playername.zstring, MAXPLAYERNAME);
	WRITEUINT16(p, (UINT16)cv_playercolor.value);
	WRITEUINT8(p, (UINT8)cv_skin.value);
	SendNetXCmd(XD_NAMEANDCOLOR, buf, p - buf);
}

// splitscreen
static void SendNameAndColor2(void)
{
	INT32 secondplaya;

	if (!splitscreen && !botingame)
		return; // can happen if skin2/color2/name2 changed

	if (secondarydisplayplayer != consoleplayer)
		secondplaya = secondarydisplayplayer;
	else // HACK
		secondplaya = 1;

	// normal player colors
	if (G_GametypeHasTeams())
	{
		if (players[secondplaya].ctfteam == 1 && cv_playercolor2.value != skincolor_redteam)
			CV_StealthSetValue(&cv_playercolor2, skincolor_redteam);
		else if (players[secondplaya].ctfteam == 2 && cv_playercolor2.value != skincolor_blueteam)
			CV_StealthSetValue(&cv_playercolor2, skincolor_blueteam);
	}

	// never allow inaccessible colors
	if (!cv_playercolor2.value || !skincolors[cv_playercolor2.value].accessible)
	{
		if (players[secondplaya].skincolor && skincolors[cv_playercolor2.value].accessible)
			CV_StealthSetValue(&cv_playercolor2, players[secondplaya].skincolor);
		else if (skins[players[secondplaya].skin].prefcolor && skincolors[skins[players[secondplaya].skin].prefcolor].accessible)
			CV_StealthSetValue(&cv_playercolor2, skins[players[secondplaya].skin].prefcolor);
		else
			CV_StealthSet(&cv_playercolor2, cv_playercolor2.defaultvalue);
	}

	// We'll handle it later if we're not playing.
	if (!Playing())
		return;

	// If you're not in a netgame, merely update the skin, color, and name.
	if (botingame)
	{
		players[secondplaya].skincolor = botcolor;
		if (players[secondplaya].mo)
			players[secondplaya].mo->color = players[secondplaya].skincolor;
		SetPlayerSkinByNum(secondplaya, botskin-1);
		return;
	}
	else if (!netgame)
	{
		INT32 foundskin;

		CleanupPlayerName(secondplaya, cv_playername2.zstring);
		strcpy(player_names[secondplaya], cv_playername2.zstring);

		// don't use secondarydisplayplayer: the second player must be 1
		players[secondplaya].skincolor = cv_playercolor2.value;
		if (players[secondplaya].mo)
			players[secondplaya].mo->color = players[secondplaya].skincolor;

		if (cv_forceskin.value >= 0 && (netgame || multiplayer)) // Server wants everyone to use the same player
		{
			const INT32 forcedskin = cv_forceskin.value;

			SetPlayerSkinByNum(secondplaya, forcedskin);
			CV_StealthSet(&cv_skin2, skins[forcedskin].name);
		}
		else if ((foundskin = R_SkinAvailable(cv_skin2.string)) != -1)
		{
			boolean notsame;

			cv_skin2.value = foundskin;

			notsame = (cv_skin2.value != players[secondplaya].skin);

			SetPlayerSkin(secondplaya, cv_skin2.string);

			if (notsame)
			{
				CV_StealthSetValue(&cv_playercolor2, skins[players[secondplaya].skin].prefcolor);

				players[secondplaya].skincolor = (cv_playercolor2.value) % numskincolors;

				if (players[secondplaya].mo)
					players[secondplaya].mo->color = players[secondplaya].skincolor;
			}
		}
		else
		{
			cv_skin2.value = players[secondplaya].skin;
			CV_StealthSet(&cv_skin2, skins[players[secondplaya].skin].name);
			// will always be same as current
			SetPlayerSkin(secondplaya, cv_skin2.string);
		}
		return;
	}

	// Don't actually send anything because splitscreen isn't actually allowed in netgames anyway!
}

static void Got_NameAndColor(UINT8 **cp, INT32 playernum)
{
	player_t *p = &players[playernum];
	char name[MAXPLAYERNAME+1];
	UINT16 color;
	UINT8 skin;

#ifdef PARANOIA
	if (playernum < 0 || playernum > MAXPLAYERS)
		I_Error("There is no player %d!", playernum);
#endif

	if (playernum == consoleplayer)
		snacpending--;
	else if (playernum == secondarydisplayplayer)
		snac2pending--;

#ifdef PARANOIA
	if (snacpending < 0 || snac2pending < 0)
		I_Error("snacpending negative!");
#endif

	READSTRINGN(*cp, name, MAXPLAYERNAME);
	color = READUINT16(*cp);
	skin = READUINT8(*cp);

	// set name
	if (strcasecmp(player_names[playernum], name) != 0)
		SetPlayerName(playernum, name);

	// set color
	p->skincolor = color % numskincolors;
	if (p->mo)
		p->mo->color = p->skincolor;

	// normal player colors
	if (server && (p != &players[consoleplayer] && p != &players[secondarydisplayplayer]))
	{
		boolean kick = false;

		// don't allow inaccessible colors
		if (skincolors[p->skincolor].accessible == false)
			kick = true;

		// team colors
		if (G_GametypeHasTeams())
		{
			if (p->ctfteam == 1 && p->skincolor != skincolor_redteam)
				kick = true;
			else if (p->ctfteam == 2 && p->skincolor != skincolor_blueteam)
				kick = true;
		}

		if (kick)
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal color change received from %s (team: %d), color: %d)\n"), player_names[playernum], p->ctfteam, p->skincolor);
			SendKick(playernum, KICK_MSG_CON_FAIL);;
			return;
		}
	}

	// set skin
	if (cv_forceskin.value >= 0 && (netgame || multiplayer)) // Server wants everyone to use the same player
	{
		const INT32 forcedskin = cv_forceskin.value;
		SetPlayerSkinByNum(playernum, forcedskin);

		if (playernum == consoleplayer)
			CV_StealthSet(&cv_skin, skins[forcedskin].name);
		else if (playernum == secondarydisplayplayer)
			CV_StealthSet(&cv_skin2, skins[forcedskin].name);
	}
	else
		SetPlayerSkinByNum(playernum, skin);
}

void SendWeaponPref(void)
{
	XBOXSTATIC UINT8 buf[1];

	buf[0] = 0;
	if (cv_flipcam.value)
		buf[0] |= 1;
	if (cv_analog.value)
		buf[0] |= 2;
	SendNetXCmd(XD_WEAPONPREF, buf, 1);
}

void SendWeaponPref2(void)
{
	XBOXSTATIC UINT8 buf[1];

	buf[0] = 0;
	if (cv_flipcam2.value)
		buf[0] |= 1;
	if (cv_analog2.value)
		buf[0] |= 2;
	SendNetXCmd2(XD_WEAPONPREF, buf, 1);
}

static void Got_WeaponPref(UINT8 **cp,INT32 playernum)
{
	UINT8 prefs = READUINT8(*cp);

	players[playernum].pflags &= ~(PF_FLIPCAM|PF_ANALOGMODE);
	if (prefs & 1)
		players[playernum].pflags |= PF_FLIPCAM;
	if (prefs & 2)
		players[playernum].pflags |= PF_ANALOGMODE;
}

void D_SendPlayerConfig(void)
{
	SendNameAndColor();
	if (splitscreen || botingame)
		SendNameAndColor2();
	SendWeaponPref();
	if (splitscreen)
		SendWeaponPref2();
}

// Only works for displayplayer, sorry!
static void Command_ResetCamera_f(void)
{
	P_ResetCamera(&players[displayplayer], &camera);
}

// ========================================================================

// play a demo, add .lmp for external demos
// eg: playdemo demo1 plays the internal game demo
//
// UINT8 *demofile; // demo file buffer
static void Command_Playdemo_f(void)
{
	char name[256];

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("playdemo <demoname>: playback a demo\n"));
		return;
	}

	if (netgame)
	{
		CONS_Printf(M_GetText("You can't play a demo while in a netgame.\n"));
		return;
	}

	// disconnect from server here?
	if (demoplayback)
		G_StopDemo();
	if (metalplayback)
		G_StopMetalDemo();

	// open the demo file
	strcpy(name, COM_Argv(1));
	// dont add .lmp so internal game demos can be played

	CONS_Printf(M_GetText("Playing back demo '%s'.\n"), name);

	// Internal if no extension, external if one exists
	// If external, convert the file name to a path in SRB2's home directory
	if (FIL_CheckExtension(name))
		G_DoPlayDemo(va("%s"PATHSEP"%s", srb2home, name));
	else
		G_DoPlayDemo(name);
}

static void Command_Timedemo_f(void)
{
	char name[256];

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("timedemo <demoname>: time a demo\n"));
		return;
	}

	if (netgame)
	{
		CONS_Printf(M_GetText("You can't play a demo while in a netgame.\n"));
		return;
	}

	// disconnect from server here?
	if (demoplayback)
		G_StopDemo();
	if (metalplayback)
		G_StopMetalDemo();

	// open the demo file
	strcpy (name, COM_Argv(1));
	// dont add .lmp so internal game demos can be played

	CONS_Printf(M_GetText("Timing demo '%s'.\n"), name);

	G_TimeDemo(name);
}

// stop current demo
static void Command_Stopdemo_f(void)
{
	G_CheckDemoStatus();
	CONS_Printf(M_GetText("Stopped demo.\n"));
}

static void Command_StartMovie_f(void)
{
	M_StartMovie();
}

static void Command_StopMovie_f(void)
{
	M_StopMovie();
}

INT32 mapchangepending = 0;

/** Runs a map change.
  * The supplied data are assumed to be good. If provided by a user, they will
  * have already been checked in Command_Map_f().
  *
  * Do \b NOT call this function directly from a menu! M_Responder() is called
  * from within the event processing loop, and this function calls
  * SV_SpawnServer(), which calls CL_ConnectToServer(), which gives you "Press
  * ESC to abort", which calls I_GetKey(), which adds an event. In other words,
  * 63 old events will get reexecuted, with ridiculous results. Just don't do
  * it (without setting delay to 1, which is the current solution).
  *
  * \param mapnum          Map number to change to.
  * \param gametype        Gametype to switch to.
  * \param pultmode        Is this 'Ultimate Mode'?
  * \param resetplayers    1 to reset player scores and lives and such, 0 not to.
  * \param delay           Determines how the function will be executed: 0 to do
  *                        it all right now (must not be done from a menu), 1 to
  *                        do step one and prepare step two, 2 to do step two.
  * \param skipprecutscene To skip the precutscence or not?
  * \sa D_GameTypeChanged, Command_Map_f
  * \author Graue <graue@oceanbase.org>
  */
void D_MapChange(INT32 mapnum, INT32 newgametype, boolean pultmode, boolean resetplayers, INT32 delay, boolean skipprecutscene, boolean FLS)
{
	static char buf[2+MAX_WADPATH+1+4];
	static char *buf_p = buf;

	// The supplied data are assumed to be good.
	I_Assert(delay >= 0 && delay <= 2);

	CONS_Debug(DBG_GAMELOGIC, "Map change: mapnum=%d gametype=%d ultmode=%d resetplayers=%d delay=%d skipprecutscene=%d\n",
	           mapnum, newgametype, pultmode, resetplayers, delay, skipprecutscene);

	if (netgame || multiplayer)
		FLS = false;

	if (delay != 2)
	{
		UINT8 flags = 0;
		const char *mapname = G_BuildMapName(mapnum);

		I_Assert(W_CheckNumForName(mapname) != LUMPERROR);

		buf_p = buf;
		if (pultmode)
			flags |= 1;
		if (!resetplayers)
			flags |= 1<<1;
		if (skipprecutscene)
			flags |= 1<<2;
		if (FLS)
			flags |= 1<<3;
		WRITEUINT8(buf_p, flags);

		// new gametype value
		WRITEUINT8(buf_p, newgametype);

		WRITESTRINGN(buf_p, mapname, MAX_WADPATH);
	}

	if (delay == 1)
		mapchangepending = 1;
	else
	{
		mapchangepending = 0;
		// spawn the server if needed
		// reset players if there is a new one
		if (!IsPlayerAdmin(consoleplayer))
		{
			if (SV_SpawnServer())
				buf[0] &= ~(1<<1);
			if (!Playing()) // you failed to start a server somehow, so cancel the map change
				return;
		}

		// Kick bot from special stages
		if (botskin)
		{
			if (G_IsSpecialStage(mapnum))
			{
				if (botingame)
				{
					//CL_RemoveSplitscreenPlayer();
					botingame = false;
					playeringame[1] = false;
				}
			}
			else if (!botingame)
			{
				//CL_AddSplitscreenPlayer();
				botingame = true;
				secondarydisplayplayer = 1;
				playeringame[1] = true;
				players[1].bot = 1;
				SendNameAndColor2();
			}
		}

		chmappending++;
		if (netgame)
			WRITEUINT32(buf_p, M_RandomizedSeed()); // random seed
		SendNetXCmd(XD_MAP, buf, buf_p - buf);
	}
}

// Warp to map code.
// Called either from map <mapname> console command, or idclev cheat.
//
static void Command_Map_f(void)
{
	const char *mapname;
	size_t i;
	INT32 newmapnum;
	boolean newresetplayers;
	INT32 newgametype = gametype;

	// max length of command: map map03 -gametype coop -noresetplayers -force
	//                         1    2       3       4         5           6
	// = 8 arg max
	if (COM_Argc() < 2 || COM_Argc() > 8)
	{
		CONS_Printf(M_GetText("map <mapname> [-gametype <type> [-force]: warp to map\n"));
		return;
	}

	if (client && !IsPlayerAdmin(consoleplayer))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	// internal wad lump always: map command doesn't support external files as in doom legacy
	if (W_CheckNumForName(COM_Argv(1)) == LUMPERROR)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Internal game level '%s' not found\n"), COM_Argv(1));
		return;
	}

	if (!(netgame || multiplayer) && (!modifiedgame || savemoddata))
	{
		if (COM_CheckParm("-force") || COM_CheckParm("-f"))
			G_SetGameModified(false);
		else
		{
			CONS_Printf(M_GetText("Sorry, level change disabled in single player.\n"));
			return;
		}
	}

	newresetplayers = !COM_CheckParm("-noresetplayers");

	if (!newresetplayers && !cv_debug)
	{
		CONS_Printf(M_GetText("DEVMODE must be enabled.\n"));
		return;
	}

	mapname = COM_Argv(1);
	if (strlen(mapname) != 5
	|| (newmapnum = M_MapNumber(mapname[3], mapname[4])) == 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Invalid level name %s\n"), mapname);
		return;
	}

	// Ultimate Mode only in SP via menu
	if (netgame || multiplayer)
		ultimatemode = false;

	// new gametype value
	// use current one by default
	i = COM_CheckParm("-gametype");
	if (i)
	{
		if (!multiplayer)
		{
			CONS_Printf(M_GetText("You can't switch gametypes in single player!\n"));
			return;
		}

		newgametype = G_GetGametypeByName(COM_Argv(i+1));

		if (newgametype == -1) // reached end of the list with no match
		{
			INT32 j = atoi(COM_Argv(i+1)); // assume they gave us a gametype number, which is okay too
			if (j >= 0 && j < NUMGAMETYPES)
				newgametype = (INT16)j;
		}
	}

	// don't use a gametype the map doesn't support
	if (cv_debug || COM_CheckParm("-force") || COM_CheckParm("-f") || cv_skipmapcheck.value)
		; // The player wants us to trek on anyway.  Do so.
	// G_TOLFlag handles both multiplayer gametype and ignores it for !multiplayer
	// Alternatively, bail if the map header is completely missing anyway.
	else if (!mapheaderinfo[newmapnum-1]
	 || !(mapheaderinfo[newmapnum-1]->typeoflevel & G_TOLFlag(newgametype)))
	{
		char gametypestring[32] = "Single Player";

		if (multiplayer)
		{
			if (newgametype >= 0 && newgametype < NUMGAMETYPES
			&& Gametype_Names[newgametype])
				strcpy(gametypestring, Gametype_Names[newgametype]);
		}

		CONS_Alert(CONS_WARNING, M_GetText("%s doesn't support %s mode!\n(Use -force to override)\n"), mapname, gametypestring);
		return;
	}

	// Prevent warping to locked levels
	// ... unless you're in a dedicated server.  Yes, technically this means you can view any level by
	// running a dedicated server and joining it yourself, but that's better than making dedicated server's
	// lives hell.
	if (!dedicated && M_MapLocked(newmapnum))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You need to unlock this level before you can warp to it!\n"));
		return;
	}

	fromlevelselect = false;
	D_MapChange(newmapnum, newgametype, false, newresetplayers, 0, false, false);
}

/** Receives a map command and changes the map.
  *
  * \param cp        Data buffer.
  * \param playernum Player number responsible for the message. Should be
  *                  ::serverplayer or ::adminplayer.
  * \sa D_MapChange
  */
static void Got_Mapcmd(UINT8 **cp, INT32 playernum)
{
	char mapname[MAX_WADPATH+1];
	UINT8 flags;
	INT32 resetplayer = 1, lastgametype;
	UINT8 skipprecutscene, FLS;
	INT16 mapnumber;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal map change received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	if (chmappending)
		chmappending--;

	flags = READUINT8(*cp);

	ultimatemode = ((flags & 1) != 0);
	if (netgame || multiplayer)
		ultimatemode = false;

	resetplayer = ((flags & (1<<1)) == 0);

	lastgametype = gametype;
	gametype = READUINT8(*cp);

	if (gametype != lastgametype)
		D_GameTypeChanged(lastgametype); // emulate consvar_t behavior for gametype

	skipprecutscene = ((flags & (1<<2)) != 0);

	FLS = ((flags & (1<<3)) != 0);

	READSTRINGN(*cp, mapname, MAX_WADPATH);

	if (netgame)
		P_SetRandSeed(READUINT32(*cp));

	if (!skipprecutscene)
	{
		DEBFILE(va("Warping to %s [resetplayer=%d lastgametype=%d gametype=%d cpnd=%d]\n",
			mapname, resetplayer, lastgametype, gametype, chmappending));
		CONS_Printf(M_GetText("Speeding off to level...\n"));
	}

	if (demoplayback && !timingdemo)
		precache = false;

	if (resetplayer)
	{
		if (!FLS || (netgame || multiplayer))
			emeralds = 0;
	}

	mapnumber = M_MapNumber(mapname[3], mapname[4]);
	LUAh_MapChange(mapnumber);

	G_InitNew(ultimatemode, mapname, resetplayer, skipprecutscene);
	if (demoplayback && !timingdemo)
		precache = true;
	if (timingdemo)
		G_DoneLevelLoad();

	if (modeattacking)
	{
		SetPlayerSkinByNum(0, cv_chooseskin.value-1);
		players[0].skincolor = skins[players[0].skin].prefcolor;
		CV_StealthSetValue(&cv_playercolor, players[0].skincolor);

		// a copy of color
		if (players[0].mo)
			players[0].mo->color = players[0].skincolor;
	}
	if (metalrecording)
		G_BeginMetal();
	if (demorecording) // Okay, level loaded, character spawned and skinned,
		G_BeginRecording(); // I AM NOW READY TO RECORD.
	demo_start = true;
}

static void Command_Pause(void)
{
	XBOXSTATIC UINT8 buf[2];
	UINT8 *cp = buf;

	if (COM_Argc() > 1)
		WRITEUINT8(cp, (char)(atoi(COM_Argv(1)) != 0));
	else
		WRITEUINT8(cp, (char)(!paused));

	if (dedicated)
		WRITEUINT8(cp, 1);
	else
		WRITEUINT8(cp, 0);

	if (cv_pause.value || server || (IsPlayerAdmin(consoleplayer)))
	{
		if (modeattacking || !(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION || gamestate == GS_WAITINGPLAYERS))
		{
			CONS_Printf(M_GetText("You can't pause here.\n"));
			return;
		}
		SendNetXCmd(XD_PAUSE, &buf, 2);
	}
	else
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
}

static void Got_Pause(UINT8 **cp, INT32 playernum)
{
	UINT8 dedicatedpause = false;
	const char *playername;

	if (netgame && !cv_pause.value && playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal pause command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	if (modeattacking)
		return;

	paused = READUINT8(*cp);
	dedicatedpause = READUINT8(*cp);

	if (!demoplayback)
	{
		if (netgame)
		{
			if (dedicatedpause)
				playername = "SERVER";
			else
				playername = player_names[playernum];

			if (paused)
				CONS_Printf(M_GetText("Game paused by %s\n"), playername);
			else
				CONS_Printf(M_GetText("Game unpaused by %s\n"), playername);
		}

		if (paused)
		{
			if (!menuactive || netgame)
				S_PauseAudio();
		}
		else
			S_ResumeAudio();
	}

	I_UpdateMouseGrab();
}

// Command for stuck characters in netgames, griefing, etc.
static void Command_Suicide(void)
{
	XBOXSTATIC UINT8 buf[4];
	UINT8 *cp = buf;

	WRITEINT32(cp, consoleplayer);

	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
	{
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
		return;
	}

	if (!G_PlatformGametype())
	{
		CONS_Printf(M_GetText("You may only use this in co-op, race, and competition!\n"));
		return;
	}

	// Retry is quicker.  Probably should force people to use it.
	if (!(netgame || multiplayer))
	{
		CONS_Printf(M_GetText("You can't use this in Single Player! Use \"retry\" instead.\n"));
		return;
	}

	SendNetXCmd(XD_SUICIDE, &buf, 4);
}

static void Got_Suicide(UINT8 **cp, INT32 playernum)
{
	INT32 suicideplayer = READINT32(*cp);

	// You can't suicide someone else.  Nice try, there.
	if (suicideplayer != playernum || (!G_PlatformGametype()))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal suicide command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	if (players[suicideplayer].mo)
		P_DamageMobj(players[suicideplayer].mo, NULL, NULL, 10000);
}

/** Deals with an ::XD_RANDOMSEED message in a netgame.
  * These messages set the position of the random number LUT and are crucial to
  * correct synchronization.
  *
  * Such a message should only ever come from the ::serverplayer. If it comes
  * from any other player, it is ignored.
  *
  * \param cp        Data buffer.
  * \param playernum Player responsible for the message. Must be ::serverplayer.
  * \author Graue <graue@oceanbase.org>
  */
static void Got_RandomSeed(UINT8 **cp, INT32 playernum)
{
	UINT32 seed;

	seed = READUINT32(*cp);

	if (playernum != serverplayer) // it's not from the server, wtf?
		return;

	P_SetRandSeed(seed);
}

/** Clears all players' scores in a netgame.
  * Only the server or a remote admin can use this command, for obvious reasons.
  *
  * \sa XD_CLEARSCORES, Got_Clearscores
  * \author SSNTails <http://www.ssntails.org>
  */
static void Command_Clearscores_f(void)
{
	if (!(server || (IsPlayerAdmin(consoleplayer))))
		return;

	SendNetXCmd(XD_CLEARSCORES, NULL, 1);
}

/** Handles an ::XD_CLEARSCORES message, which resets all players' scores in a
  * netgame to zero.
  *
  * \param cp        Data buffer.
  * \param playernum Player responsible for the message. Must be ::serverplayer
  *                  or ::adminplayer.
  * \sa XD_CLEARSCORES, Command_Clearscores_f
  * \author SSNTails <http://www.ssntails.org>
  */
static void Got_Clearscores(UINT8 **cp, INT32 playernum)
{
	INT32 i;

	(void)cp;
	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal clear scores command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
		players[i].score = 0;

	CONS_Printf(M_GetText("Scores have been reset by the server.\n"));
}

// Team changing functions
static void Command_Teamchange_f(void)
{
	changeteam_union NetPacket;
	boolean error = false;
	UINT16 usvalue;
	NetPacket.value.l = NetPacket.value.b = 0;

	//      0         1
	// changeteam  <color>

	if (COM_Argc() <= 1)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("changeteam <team>: switch to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("changeteam <team>: switch to a new team (%s)\n"), "spectator or playing");
		else
			CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (!strcasecmp(COM_Argv(1), "red") || !strcasecmp(COM_Argv(1), "1"))
			NetPacket.packet.newteam = 1;
		else if (!strcasecmp(COM_Argv(1), "blue") || !strcasecmp(COM_Argv(1), "2"))
			NetPacket.packet.newteam = 2;
		else if (!strcasecmp(COM_Argv(1), "spectator") || !strcasecmp(COM_Argv(1), "0"))
			NetPacket.packet.newteam = 0;
		else
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if (!strcasecmp(COM_Argv(1), "spectator") || !strcasecmp(COM_Argv(1), "0"))
			NetPacket.packet.newteam = 0;
		else if (!strcasecmp(COM_Argv(1), "playing") || !strcasecmp(COM_Argv(1), "1"))
			NetPacket.packet.newteam = 3;
		else
			error = true;
	}
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (error)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("changeteam <team>: switch to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("changeteam <team>: switch to a new team (%s)\n"), "spectator or playing");
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (NetPacket.packet.newteam == (unsigned)players[consoleplayer].ctfteam ||
			(players[consoleplayer].spectator && !NetPacket.packet.newteam))
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if ((players[consoleplayer].spectator && !NetPacket.packet.newteam) ||
			(!players[consoleplayer].spectator && NetPacket.packet.newteam == 3))
			error = true;
	}
#ifdef PARANOIA
	else
		I_Error("Invalid gametype after initial checks!");
#endif

	if (error)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You're already on that team!\n"));
		return;
	}

	if (!cv_allowteamchange.value && NetPacket.packet.newteam) // allow swapping to spectator even in locked teams.
	{
		CONS_Alert(CONS_NOTICE, M_GetText("The server is not allowing team changes at the moment.\n"));
		return;
	}

	//additional check for hide and seek. Don't allow change of status after hidetime ends.
	if (gametype == GT_HIDEANDSEEK && leveltime >= (hidetime * TICRATE))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Hiding time expired; no Hide and Seek status changes allowed!\n"));
		return;
	}

	usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
	SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
}

static void Command_Teamchange2_f(void)
{
	changeteam_union NetPacket;
	boolean error = false;
	UINT16 usvalue;
	NetPacket.value.l = NetPacket.value.b = 0;

	//      0         1
	// changeteam2 <color>

	if (COM_Argc() <= 1)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("changeteam <team>: switch to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("changeteam <team>: switch to a new team (%s)\n"), "spectator or playing");
		else
			CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (!strcasecmp(COM_Argv(1), "red") || !strcasecmp(COM_Argv(1), "1"))
			NetPacket.packet.newteam = 1;
		else if (!strcasecmp(COM_Argv(1), "blue") || !strcasecmp(COM_Argv(1), "2"))
			NetPacket.packet.newteam = 2;
		else if (!strcasecmp(COM_Argv(1), "spectator") || !strcasecmp(COM_Argv(1), "0"))
			NetPacket.packet.newteam = 0;
		else
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if (!strcasecmp(COM_Argv(1), "spectator") || !strcasecmp(COM_Argv(1), "0"))
			NetPacket.packet.newteam = 0;
		else if (!strcasecmp(COM_Argv(1), "playing") || !strcasecmp(COM_Argv(1), "1"))
			NetPacket.packet.newteam = 3;
		else
			error = true;
	}

	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (error)
	{
		if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("changeteam2 <team>: switch to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("changeteam2 <team>: switch to a new team (%s)\n"), "spectator or playing");
		return;
	}

	if (G_GametypeHasTeams())
	{
		if (NetPacket.packet.newteam == (unsigned)players[secondarydisplayplayer].ctfteam ||
			(players[secondarydisplayplayer].spectator && !NetPacket.packet.newteam))
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if ((players[secondarydisplayplayer].spectator && !NetPacket.packet.newteam) ||
			(!players[secondarydisplayplayer].spectator && NetPacket.packet.newteam == 3))
			error = true;
	}
#ifdef PARANOIA
	else
		I_Error("Invalid gametype after initial checks!");
#endif

	if (error)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You're already on that team!\n"));
		return;
	}

	if (!cv_allowteamchange.value && NetPacket.packet.newteam) // allow swapping to spectator even in locked teams.
	{
		CONS_Alert(CONS_NOTICE, M_GetText("The server is not allowing team changes at the moment.\n"));
		return;
	}

	//additional check for hide and seek. Don't allow change of status after hidetime ends.
	if (gametype == GT_HIDEANDSEEK && leveltime >= (hidetime * TICRATE))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Hiding time expired; no Hide and Seek status changes allowed!\n"));
		return;
	}

	usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
	SendNetXCmd2(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
}

static void Command_ServerTeamChange_f(void)
{
	changeteam_union NetPacket;
	boolean error = false;
	UINT16 usvalue;
	NetPacket.value.l = NetPacket.value.b = 0;

	if (!(server || (IsPlayerAdmin(consoleplayer))))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	//        0              1         2
	// serverchangeteam <playernum>  <team>

	if (COM_Argc() < 3)
	{
		if (G_TagGametype())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "it, notit, playing, or spectator");
		else if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "spectator or playing");
		else
			CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (G_TagGametype())
	{
		if (!strcasecmp(COM_Argv(2), "it") || !strcasecmp(COM_Argv(2), "1"))
			NetPacket.packet.newteam = 1;
		else if (!strcasecmp(COM_Argv(2), "notit") || !strcasecmp(COM_Argv(2), "2"))
			NetPacket.packet.newteam = 2;
		else if (!strcasecmp(COM_Argv(2), "playing") || !strcasecmp(COM_Argv(2), "3"))
			NetPacket.packet.newteam = 3;
		else if (!strcasecmp(COM_Argv(2), "spectator") || !strcasecmp(COM_Argv(2), "0"))
			NetPacket.packet.newteam = 0;
		else
			error = true;
	}
	else if (G_GametypeHasTeams())
	{
		if (!strcasecmp(COM_Argv(2), "red") || !strcasecmp(COM_Argv(2), "1"))
			NetPacket.packet.newteam = 1;
		else if (!strcasecmp(COM_Argv(2), "blue") || !strcasecmp(COM_Argv(2), "2"))
			NetPacket.packet.newteam = 2;
		else if (!strcasecmp(COM_Argv(2), "spectator") || !strcasecmp(COM_Argv(2), "0"))
			NetPacket.packet.newteam = 0;
		else
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if (!strcasecmp(COM_Argv(2), "spectator") || !strcasecmp(COM_Argv(2), "0"))
			NetPacket.packet.newteam = 0;
		else if (!strcasecmp(COM_Argv(2), "playing") || !strcasecmp(COM_Argv(2), "1"))
			NetPacket.packet.newteam = 3;
		else
			error = true;
	}
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		return;
	}

	if (error)
	{
		if (G_TagGametype())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "it, notit, playing, or spectator");
		else if (G_GametypeHasTeams())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "red, blue or spectator");
		else if (G_GametypeHasSpectators())
			CONS_Printf(M_GetText("serverchangeteam <playernum> <team>: switch player to a new team (%s)\n"), "spectator or playing");
		return;
	}

	NetPacket.packet.playernum = atoi(COM_Argv(1));

	if (!playeringame[NetPacket.packet.playernum])
	{
		CONS_Alert(CONS_NOTICE, M_GetText("There is no player %d!\n"), NetPacket.packet.playernum);
		return;
	}

	if (G_TagGametype())
	{
		if (( (players[NetPacket.packet.playernum].pflags & PF_TAGIT) && NetPacket.packet.newteam == 1) ||
			(!(players[NetPacket.packet.playernum].pflags & PF_TAGIT) && NetPacket.packet.newteam == 2) ||
			( players[NetPacket.packet.playernum].spectator && !NetPacket.packet.newteam) ||
			(!players[NetPacket.packet.playernum].spectator && NetPacket.packet.newteam == 3))
			error = true;
	}
	else if (G_GametypeHasTeams())
	{
		if (NetPacket.packet.newteam == (unsigned)players[NetPacket.packet.playernum].ctfteam ||
			(players[NetPacket.packet.playernum].spectator && !NetPacket.packet.newteam))
			error = true;
	}
	else if (G_GametypeHasSpectators())
	{
		if ((players[NetPacket.packet.playernum].spectator && !NetPacket.packet.newteam) ||
			(!players[NetPacket.packet.playernum].spectator && NetPacket.packet.newteam == 3))
			error = true;
	}
#ifdef PARANOIA
	else
		I_Error("Invalid gametype after initial checks!");
#endif

	if (error)
	{
		CONS_Alert(CONS_NOTICE, M_GetText("That player is already on that team!\n"));
		return;
	}

	//additional check for hide and seek. Don't allow change of status after hidetime ends.
	if (gametype == GT_HIDEANDSEEK && leveltime >= (hidetime * TICRATE))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("Hiding time expired; no Hide and Seek status changes allowed!\n"));
		return;
	}

	NetPacket.packet.verification = true; // This signals that it's a server change

	usvalue = SHORT(NetPacket.value.l|NetPacket.value.b);
	SendNetXCmd(XD_TEAMCHANGE, &usvalue, sizeof(usvalue));
}

//todo: This and the other teamchange functions are getting too long and messy. Needs cleaning.
static void Got_Teamchange(UINT8 **cp, INT32 playernum)
{
	changeteam_union NetPacket;
	boolean error = false;
	NetPacket.value.l = NetPacket.value.b = READINT16(*cp);

	if (!G_GametypeHasTeams() && !G_GametypeHasSpectators()) //Make sure you're in the right gametype.
	{
		// this should never happen unless the client is hacked/buggy
		CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
	}

	if (NetPacket.packet.verification) // Special marker that the server sent the request
	{
		if (playernum != serverplayer && (!IsPlayerAdmin(playernum)))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
			if (server)
				SendKick(playernum, KICK_MSG_CON_FAIL);
			return;
		}
		playernum = NetPacket.packet.playernum;
	}

	// Prevent multiple changes in one go.
	if (G_TagGametype())
	{
		if (((players[playernum].pflags & PF_TAGIT) && NetPacket.packet.newteam == 1) ||
			(!(players[playernum].pflags & PF_TAGIT) && NetPacket.packet.newteam == 2) ||
			(players[playernum].spectator && NetPacket.packet.newteam == 0) ||
			(!players[playernum].spectator && NetPacket.packet.newteam == 3))
			return;
	}
	else if (G_GametypeHasTeams())
	{
		if ((NetPacket.packet.newteam && (NetPacket.packet.newteam == (unsigned)players[playernum].ctfteam)) ||
			(players[playernum].spectator && !NetPacket.packet.newteam))
			return;
	}
	else if (G_GametypeHasSpectators())
	{
		if ((players[playernum].spectator && !NetPacket.packet.newteam) ||
			(!players[playernum].spectator && NetPacket.packet.newteam == 3))
			return;
	}
	else
	{
		if (playernum != serverplayer && (!IsPlayerAdmin(playernum)))
		{
			CONS_Alert(CONS_WARNING, M_GetText("Illegal team change received from player %s\n"), player_names[playernum]);
			if (server)
				SendKick(playernum, KICK_MSG_CON_FAIL);
		}
		return;
	}

	//Make sure that the right team number is sent. Keep in mind that normal clients cannot change to certain teams in certain gametypes.
	switch (gametype)
	{
	case GT_HIDEANDSEEK:
		//no status changes after hidetime
		if (leveltime >= (hidetime * TICRATE))
		{
			error = true;
			break;
		}
		/* FALLTHRU */
	case GT_TAG:
		switch (NetPacket.packet.newteam)
		{
		case 0:
			break;
		case 1: case 2:
			if (!NetPacket.packet.verification)
				error = true; //Only admin can change player's IT status' in tag.
			break;
		case 3: //Join game via console.
			if (!NetPacket.packet.verification && !cv_allowteamchange.value)
				error = true;
			break;
		}

		break;
	default:
#ifdef PARANOIA
		if (!G_GametypeHasTeams() && !G_GametypeHasSpectators())
			I_Error("Invalid gametype after initial checks!");
#endif

		if (!cv_allowteamchange.value)
		{
			if (!NetPacket.packet.verification && NetPacket.packet.newteam)
				error = true; //Only admin can change status, unless changing to spectator.
		}
		break; //Otherwise, you don't need special permissions.
	}

	if (server && ((NetPacket.packet.newteam < 0 || NetPacket.packet.newteam > 3) || error))
		SendKick(playernum, KICK_MSG_CON_FAIL);

	//Safety first!
	if (players[playernum].mo)
	{
		if (!players[playernum].spectator)
			P_DamageMobj(players[playernum].mo, NULL, NULL, 10000);
		else
		{
			P_RemoveMobj(players[playernum].mo);
			players[playernum].mo = NULL;
			players[playernum].playerstate = PST_REBORN;
		}
	}
	else
		players[playernum].playerstate = PST_REBORN;

	//Now that we've done our error checking and killed the player
	//if necessary, put the player on the correct team/status.
	if (G_TagGametype())
	{
		if (!NetPacket.packet.newteam)
		{
			players[playernum].spectator = true;
			players[playernum].pflags &= ~PF_TAGIT;
			players[playernum].pflags &= ~PF_TAGGED;
		}
		else if (NetPacket.packet.newteam != 3) // .newteam == 1 or 2.
		{
			players[playernum].spectator = false;
			players[playernum].pflags &= ~PF_TAGGED;//Just in case.

			if (NetPacket.packet.newteam == 1) //Make the player IT.
				players[playernum].pflags |= PF_TAGIT;
			else
				players[playernum].pflags &= ~PF_TAGIT;
		}
		else // Just join the game.
		{
			players[playernum].spectator = false;

			//If joining after hidetime in normal tag, default to being IT.
			if (gametype == GT_TAG && (leveltime > (hidetime * TICRATE)))
			{
				NetPacket.packet.newteam = 1; //minor hack, causes the "is it" message to be printed later.
				players[playernum].pflags |= PF_TAGIT; //make the player IT.
			}
		}
	}
	else if (G_GametypeHasTeams())
	{
		if (!NetPacket.packet.newteam)
		{
			players[playernum].ctfteam = 0;
			players[playernum].spectator = true;
		}
		else
		{
			players[playernum].ctfteam = NetPacket.packet.newteam;
			players[playernum].spectator = false;
		}
	}
	else if (G_GametypeHasSpectators())
	{
		if (!NetPacket.packet.newteam)
			players[playernum].spectator = true;
		else
			players[playernum].spectator = false;
	}

	if (NetPacket.packet.autobalance)
	{
		if (NetPacket.packet.newteam == 1)
			CONS_Printf(M_GetText("%s was autobalanced to the %c%s%c.\n"), player_names[playernum], '\x85', M_GetText("Red Team"), '\x80');
		else if (NetPacket.packet.newteam == 2)
			CONS_Printf(M_GetText("%s was autobalanced to the %c%s%c.\n"), player_names[playernum], '\x84', M_GetText("Blue Team"), '\x80');
	}
	else if (NetPacket.packet.scrambled)
	{
		if (NetPacket.packet.newteam == 1)
			CONS_Printf(M_GetText("%s was scrambled to the %c%s%c.\n"), player_names[playernum], '\x85', M_GetText("Red Team"), '\x80');
		else if (NetPacket.packet.newteam == 2)
			CONS_Printf(M_GetText("%s was scrambled to the %c%s%c.\n"), player_names[playernum], '\x84', M_GetText("Blue Team"), '\x80');
	}
	else if (NetPacket.packet.newteam == 1)
	{
		if (G_TagGametype())
			CONS_Printf(M_GetText("%s is now IT!\n"), player_names[playernum]);
		else
			CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[playernum], '\x85', M_GetText("Red Team"), '\x80');
	}
	else if (NetPacket.packet.newteam == 2)
	{
		if (G_TagGametype())
			CONS_Printf(M_GetText("%s is no longer IT!\n"), player_names[playernum]);
		else
			CONS_Printf(M_GetText("%s switched to the %c%s%c.\n"), player_names[playernum], '\x84', M_GetText("Blue Team"), '\x80');
	}
	else if (NetPacket.packet.newteam == 3)
		CONS_Printf(M_GetText("%s entered the game.\n"), player_names[playernum]);
	else
		CONS_Printf(M_GetText("%s became a spectator.\n"), player_names[playernum]);

	//reset view if you are changed, or viewing someone who was changed.
	if (playernum == consoleplayer || displayplayer == playernum)
		displayplayer = consoleplayer;

	if (G_GametypeHasTeams())
	{
		if (NetPacket.packet.newteam)
		{
			if (playernum == consoleplayer) //CTF and Team Match colors.
				CV_SetValue(&cv_playercolor, NetPacket.packet.newteam + 5);
			else if (playernum == secondarydisplayplayer)
				CV_SetValue(&cv_playercolor2, NetPacket.packet.newteam + 5);
		}
	}

	// Clear player score and rings if a spectator.
	if (players[playernum].spectator)
	{
		players[playernum].score = 0;
		players[playernum].health = 1;
		if (players[playernum].mo)
			players[playernum].mo->health = 1;
	}

	// In tag, check to see if you still have a game.
	if (G_TagGametype())
		P_CheckSurvivors();
}

//
// Attempts to make password system a little sane without
// rewriting the entire goddamn XD_file system
//
#define BASESALT "basepasswordstorage"

void D_SetPassword(const char *pw)
{
	D_MD5PasswordPass((const UINT8 *)pw, strlen(pw), BASESALT, &adminpassmd5);
	adminpasswordset = true;
}

// Remote Administration
static void Command_Changepassword_f(void)
{
#ifdef NOMD5
	// If we have no MD5 support then completely disable XD_LOGIN responses for security.
	CONS_Alert(CONS_NOTICE, "Remote administration commands are not supported in this build.\n");
#else
	if (client) // cannot change remotely
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("password <password>: change remote admin password\n"));
		return;
	}

	D_SetPassword(COM_Argv(1));
	CONS_Printf(M_GetText("Password set.\n"));
#endif
}

static void Command_Login_f(void)
{
#ifdef NOMD5
	// If we have no MD5 support then completely disable XD_LOGIN responses for security.
	CONS_Alert(CONS_NOTICE, "Remote administration commands are not supported in this build.\n");
#else
	const char *pw;

	if (!netgame)
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	// If the server uses login, it will effectively just remove admin privileges
	// from whoever has them. This is good.
	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("login <password>: Administrator login\n"));
		return;
	}

	pw = COM_Argv(1);

	// Do the base pass to get what the server has (or should?)
	D_MD5PasswordPass((const UINT8 *)pw, strlen(pw), BASESALT, &netbuffer->u.md5sum);

	// Do the final pass to get the comparison the server will come up with
	D_MD5PasswordPass(netbuffer->u.md5sum, 16, va("PNUM%02d", consoleplayer), &netbuffer->u.md5sum);

	CONS_Printf(M_GetText("Sending login... (Notice only given if password is correct.)\n"));

	netbuffer->packettype = PT_LOGIN;
	HSendPacket(servernode, true, 0, 16);
#endif
}

boolean IsPlayerAdmin(INT32 playernum)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
		if (playernum == adminplayers[i])
			return true;

	return false;
}

void SetAdminPlayer(INT32 playernum)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playernum == adminplayers[i])
			return; // Player is already admin

		if (adminplayers[i] == -1)
		{
			adminplayers[i] = playernum; // Set the player to a free spot
			break; // End the loop now. If it keeps going, the same player might get assigned to two slots.
		}


	}
}

void ClearAdminPlayers(void)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
		adminplayers[i] = -1;
}

void RemoveAdminPlayer(INT32 playernum)
{
	INT32 i;
	for (i = 0; i < MAXPLAYERS; i++)
		if (playernum == adminplayers[i])
			adminplayers[i] = -1;
}

static void Command_Verify_f(void)
{
	XBOXSTATIC char buf[8]; // Should be plenty
	char *temp;
	INT32 playernum;

	if (client)
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	if (!netgame)
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("promote <playernum>: give admin privileges to a node\n"));
		return;
	}

	strlcpy(buf, COM_Argv(1), sizeof (buf));

	playernum = atoi(buf);

	temp = buf;

	WRITEUINT8(temp, playernum);

	if (playeringame[playernum])
		SendNetXCmd(XD_VERIFIED, buf, 1);
}

static void Got_Verification(UINT8 **cp, INT32 playernum)
{
	INT16 num = READUINT8(*cp);

	if (playernum != serverplayer) // it's not from the server (hacker or bug)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal verification received from %s (serverplayer is %s)\n"), player_names[playernum], player_names[serverplayer]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	SetAdminPlayer(num);

	if (num != consoleplayer)
		return;

	CONS_Printf(M_GetText("You are now a server administrator.\n"));
}

static void Command_RemoveAdmin_f(void)
{
	XBOXSTATIC char buf[8]; // Should be plenty
	char *temp;
	INT32 playernum;

	if (client)
	{
		CONS_Printf(M_GetText("Only the server can use this.\n"));
		return;
	}

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("demote <player>: remove admin privileges from a node\n"));
		return;
	}

	strlcpy(buf, COM_Argv(1), sizeof(buf));

	playernum = atoi(buf);

	temp = buf;

	WRITEUINT8(temp, playernum);

	if (playeringame[playernum])
		SendNetXCmd(XD_DEMOTED, buf, 1);
}

static void Got_Removal(UINT8 **cp, INT32 playernum)
{
	INT16 num = READUINT8(*cp);

	if (playernum != serverplayer) // it's not from the server (hacker or bug)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal demotion received from %s (serverplayer is %s)\n"), player_names[playernum], player_names[serverplayer]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	RemoveAdminPlayer(num);

	if (num != consoleplayer)
		return;

	CONS_Printf(M_GetText("You are no longer a server administrator.\n"));
}

static void Command_MotD_f(void)
{
	size_t i, j;
	char *mymotd;

	if ((j = COM_Argc()) < 2)
	{
		CONS_Printf(M_GetText("motd <message>: Set a message that clients see upon join.\n"));
		return;
	}

	if (!(server || (IsPlayerAdmin(consoleplayer))))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	mymotd = Z_Malloc(sizeof(motd), PU_STATIC, NULL);

	strlcpy(mymotd, COM_Argv(1), sizeof motd);
	for (i = 2; i < j; i++)
	{
		strlcat(mymotd, " ", sizeof motd);
		strlcat(mymotd, COM_Argv(i), sizeof motd);
	}

	// Disallow non-printing characters and semicolons.
	for (i = 0; mymotd[i] != '\0'; i++)
		if (!isprint(mymotd[i]) || mymotd[i] == ';')
		{
			Z_Free(mymotd);
			return;
		}

	if ((netgame || multiplayer) && client)
		SendNetXCmd(XD_SETMOTD, mymotd, i); // send the actual size of the motd string, not the full buffer's size
	else
	{
		strcpy(motd, mymotd);
		CONS_Printf(M_GetText("Message of the day set.\n"));
	}

	Z_Free(mymotd);
}

static void Got_MotD_f(UINT8 **cp, INT32 playernum)
{
	char *mymotd = Z_Malloc(sizeof(motd), PU_STATIC, NULL);
	INT32 i;
	boolean kick = false;

	READSTRINGN(*cp, mymotd, sizeof(motd));

	// Disallow non-printing characters and semicolons.
	for (i = 0; mymotd[i] != '\0'; i++)
		if (!isprint(mymotd[i]) || mymotd[i] == ';')
			kick = true;

	if ((playernum != serverplayer && !IsPlayerAdmin(playernum)) || kick)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal motd change received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);

		Z_Free(mymotd);
		return;
	}

	strcpy(motd, mymotd);

	CONS_Printf(M_GetText("Message of the day set.\n"));

	Z_Free(mymotd);
}

static void Command_RunSOC(void)
{
	const char *fn;
	XBOXSTATIC char buf[255];
	size_t length = 0;

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("runsoc <socfile.soc> or <lumpname>: run a soc\n"));
		return;
	}
	else
		fn = COM_Argv(1);

	if (netgame && !(server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	if (!(netgame || multiplayer))
	{
		if (!P_RunSOC(fn))
			CONS_Printf(M_GetText("Could not find SOC.\n"));
		else
			G_SetGameModified(multiplayer);
		return;
	}

	nameonly(strcpy(buf, fn));
	length = strlen(buf)+1;

	SendNetXCmd(XD_RUNSOC, buf, length);
}

static void Got_RunSOCcmd(UINT8 **cp, INT32 playernum)
{
	char filename[256];
	filestatus_t ncs = FS_NOTFOUND;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal runsoc command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	READSTRINGN(*cp, filename, 255);

	// Maybe add md5 support?
	if (strstr(filename, ".soc") != NULL)
	{
		ncs = findfile(filename,NULL,true);

		if (ncs != FS_FOUND)
		{
			Command_ExitGame_f();
			if (ncs == FS_NOTFOUND)
			{
				CONS_Printf(M_GetText("The server tried to add %s,\nbut you don't have this file.\nYou need to find it in order\nto play on this server.\n"), filename);
				M_StartMessage(va("The server added a file\n(%s)\nthat you do not have.\n\nPress ESC\n",filename), NULL, MM_NOTHING);
			}
			else
			{
				CONS_Printf(M_GetText("Unknown error finding soc file (%s) the server added.\n"), filename);
				M_StartMessage(va("Unknown error trying to load a file\nthat the server added\n(%s).\n\nPress ESC\n",filename), NULL, MM_NOTHING);
			}
			return;
		}
	}

	P_RunSOC(filename);
	G_SetGameModified(true);
}

/** Adds a pwad at runtime.
  * Searches for sounds, maps, music, new images.
  */
static void Command_Addfile(void)
{
	const char *fn, *p;
	XBOXSTATIC char buf[256];
	char *buf_p = buf;
	INT32 i;
	int musiconly; // W_VerifyNMUSlumps isn't boolean

	if (COM_Argc() != 2)
	{
		CONS_Printf(M_GetText("addfile <wadfile.wad>: load wad file\n"));
		return;
	}
	else
		fn = COM_Argv(1);

	// Disallow non-printing characters and semicolons.
	for (i = 0; fn[i] != '\0'; i++)
		if (!isprint(fn[i]) || fn[i] == ';')
			return;

	musiconly = W_VerifyNMUSlumps(fn);

	if (!musiconly)
	{
		// ... But only so long as they contain nothing more then music and sprites.
		if (netgame && !(server || IsPlayerAdmin(consoleplayer)))
		{
			CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
			return;
		}
		G_SetGameModified(multiplayer);
	}

	// Add file on your client directly if it is trivial, or you aren't in a netgame.
	if (!(netgame || multiplayer) || musiconly)
	{
		P_AddWadFile(fn);
		return;
	}

	p = fn+strlen(fn);
	while(--p >= fn)
		if (*p == '\\' || *p == '/' || *p == ':')
			break;
	++p;
	// check total packet size and no of files currently loaded
	// See W_LoadWadFile in w_wad.c
	if ((numwadfiles >= MAX_WADFILES)
	|| ((packetsizetally + nameonlylength(fn) + 22) > MAXFILENEEDED*sizeof(UINT8)))
	{
		CONS_Alert(CONS_ERROR, M_GetText("Too many files loaded to add %s\n"), fn);
		return;
	}

	WRITESTRINGN(buf_p,p,240);

	// calculate and check md5
	{
		UINT8 md5sum[16];
#ifdef NOMD5
		memset(md5sum,0,16);
#else
		FILE *fhandle;

		if ((fhandle = W_OpenWadFile(&fn, true)) != NULL)
		{
			tic_t t = I_GetTime();
			CONS_Debug(DBG_SETUP, "Making MD5 for %s\n",fn);
			md5_stream(fhandle, md5sum);
			CONS_Debug(DBG_SETUP, "MD5 calc for %s took %f second\n", fn, (float)(I_GetTime() - t)/TICRATE);
			fclose(fhandle);
		}
		else // file not found
			return;

		for (i = 0; i < numwadfiles; i++)
		{
			if (!memcmp(wadfiles[i]->md5sum, md5sum, 16))
			{
				CONS_Alert(CONS_ERROR, M_GetText("%s is already loaded\n"), fn);
				return;
			}
		}
#endif
		WRITEMEM(buf_p, md5sum, 16);
	}

	if (IsPlayerAdmin(consoleplayer) && (!server)) // Request to add file
		SendNetXCmd(XD_REQADDFILE, buf, buf_p - buf);
	else
		SendNetXCmd(XD_ADDFILE, buf, buf_p - buf);
}

#ifdef DELFILE
/** removes the last added pwad at runtime.
  * Searches for sounds, maps, music and images to remove
  */
static void Command_Delfile(void)
{
	if (gamestate == GS_LEVEL)
	{
		CONS_Printf(M_GetText("You must NOT be in a level to use this.\n"));
		return;
	}

	if (netgame && !(server || adminplayer == consoleplayer))
	{
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		return;
	}

	if (numwadfiles <= mainwads)
	{
		CONS_Printf(M_GetText("No additional WADs are loaded.\n"));
		return;
	}

	if (!(netgame || multiplayer))
	{
		P_DelWadFile();
		if (mainwads == numwadfiles && modifiedgame)
			modifiedgame = false;
		return;
	}

	SendNetXCmd(XD_DELFILE, NULL, 0);
}
#endif

static void Got_RequestAddfilecmd(UINT8 **cp, INT32 playernum)
{
	char filename[241];
	filestatus_t ncs = FS_NOTFOUND;
	UINT8 md5sum[16];
	boolean kick = false;
	boolean toomany = false;
	INT32 i,j;
	serverinfo_pak *dummycheck = NULL;

	// Shut the compiler up.
	(void)dummycheck;

	READSTRINGN(*cp, filename, 240);
	READMEM(*cp, md5sum, 16);

	// Only the server processes this message.
	if (client)
		return;

	// Disallow non-printing characters and semicolons.
	for (i = 0; filename[i] != '\0'; i++)
		if (!isprint(filename[i]) || filename[i] == ';')
			kick = true;

	if ((playernum != serverplayer && !IsPlayerAdmin(playernum)) || kick)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal addfile command received from %s\n"), player_names[playernum]);
		SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	// See W_LoadWadFile in w_wad.c
	if ((numwadfiles >= MAX_WADFILES)
	|| ((packetsizetally + nameonlylength(filename) + 22) > MAXFILENEEDED*sizeof(UINT8)))
		toomany = true;
	else
		ncs = findfile(filename,md5sum,true);

	if (ncs != FS_FOUND || toomany)
	{
		char *message = Z_Malloc(512, PU_STATIC, NULL);

		if (toomany)
			snprintf(message, 512, M_GetText("Too many files loaded to add %s\n"), filename);
		else if (ncs == FS_NOTFOUND)
			snprintf(message, 512, M_GetText("The server doesn't have %s\n"), filename);
		else if (ncs == FS_MD5SUMBAD)
			snprintf(message, 512, M_GetText("Checksum mismatch on %s\n"), filename);
		else
			snprintf(message, 512, M_GetText("Unknown error finding wad file (%s)\n"), filename);

		CONS_Printf("%s",message);

		for (j = 0; j < MAXPLAYERS; j++)
			if (adminplayers[j])
				COM_BufAddText(va("sayto %d %s", adminplayers[j], message));

		Z_Free(message);
		return;
	}

	COM_BufAddText(va("addfile %s\n", filename));
}

#ifdef DELFILE
static void Got_Delfilecmd(UINT8 **cp, INT32 playernum)
{
	if (playernum != serverplayer && playernum != adminplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal delfile command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}
	(void)cp;

	if (numwadfiles <= mainwads) //sanity
		return;

	P_DelWadFile();
	if (mainwads == numwadfiles && modifiedgame)
		modifiedgame = false;
}
#endif

static void Got_Addfilecmd(UINT8 **cp, INT32 playernum)
{
	char filename[241];
	filestatus_t ncs = FS_NOTFOUND;
	UINT8 md5sum[16];

	READSTRINGN(*cp, filename, 240);
	READMEM(*cp, md5sum, 16);

	if (playernum != serverplayer)
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal addfile command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	ncs = findfile(filename,md5sum,true);

	if (ncs != FS_FOUND || !P_AddWadFile(filename))
	{
		Command_ExitGame_f();
		if (ncs == FS_FOUND)
		{
			CONS_Printf(M_GetText("The server tried to add %s,\nbut you have too many files added.\nRestart the game to clear loaded files\nand play on this server."), filename);
			M_StartMessage(va("The server added a file \n(%s)\nbut you have too many files added.\nRestart the game to clear loaded files.\n\nPress ESC\n",filename), NULL, MM_NOTHING);
		}
		else if (ncs == FS_NOTFOUND)
		{
			CONS_Printf(M_GetText("The server tried to add %s,\nbut you don't have this file.\nYou need to find it in order\nto play on this server."), filename);
			M_StartMessage(va("The server added a file \n(%s)\nthat you do not have.\n\nPress ESC\n",filename), NULL, MM_NOTHING);
		}
		else if (ncs == FS_MD5SUMBAD)
		{
			CONS_Printf(M_GetText("Checksum mismatch while loading %s.\nMake sure you have the copy of\nthis file that the server has.\n"), filename);
			M_StartMessage(va("Checksum mismatch while loading \n%s.\nThe server seems to have a\ndifferent version of this file.\n\nPress ESC\n",filename), NULL, MM_NOTHING);
		}
		else
		{
			CONS_Printf(M_GetText("Unknown error finding wad file (%s) the server added.\n"), filename);
			M_StartMessage(va("Unknown error trying to load a file\nthat the server added \n(%s).\n\nPress ESC\n",filename), NULL, MM_NOTHING);
		}
		return;
	}

	G_SetGameModified(true);
}

static void Command_ListWADS_f(void)
{
	INT32 i = numwadfiles;
	char *tempname;
	CONS_Printf(M_GetText("There are %d wads loaded:\n"),numwadfiles);
	for (i--; i >= 0; i--)
	{
		nameonly(tempname = va("%s", wadfiles[i]->filename));
		if (!i)
			CONS_Printf("\x82 IWAD\x80: %s\n", tempname);
		else if (i <= mainwads)
			CONS_Printf("\x82 * %.2d\x80: %s\n", i, tempname);
		else
			CONS_Printf("   %.2d: %s\n", i, tempname);
	}
}

// =========================================================================
//                            MISC. COMMANDS
// =========================================================================

/** Prints program version.
  */
static void Command_Version_f(void)
{

const char *platform = I_GetPlatform();

#ifdef DEVELOP
	CONS_Printf("Sonic Robo Blast 2 Legacy %s-%s (%s %s) ", NULL, Compbranch, comprevision, compdate, comptime);
#else
	CONS_Printf("Sonic Robo Blast 2 Legacy %s (%s %s %s %s) ", VERSIONSTRING, compdate, comptime, comprevision, compbranch);
#endif

	// Base library
#if defined( HAVE_SDL)
	CONS_Printf("SDL ");
#elif defined(_WINDOWS)
	CONS_Printf("DD ");
#endif

	// OS
	CONS_Printf("%s ", platform);

	// Bitness
	if (sizeof(void*) == 4)
		CONS_Printf("32-bit ");
	else if (sizeof(void*) == 8)
		CONS_Printf("64-bit ");
	else // 16-bit? 128-bit?
		CONS_Printf("Bits Unknown ");

	// Debug build
#ifdef _DEBUG
	CONS_Printf("\x85" "DEBUG " "\x80");
#endif

	// DEVELOP build
#ifdef DEVELOP
	CONS_Printf("\x87" "DEVELOP " "\x80");
#endif

	CONS_Printf("\n");
}

#ifdef UPDATE_ALERT
static void Command_ModDetails_f(void)
{
	CONS_Printf(M_GetText("Mod ID: %d\nMod Version: %d\nCode Base:%d\n"), MODID, MODVERSION, CODEBASE);
}
#endif

// Returns current gametype being used.
//
static void Command_ShowGametype_f(void)
{
	const char *gametypestr = NULL;

	if (!(netgame || multiplayer)) // print "Single player" instead of "Co-op"
	{
		CONS_Printf(M_GetText("Current gametype is %s\n"), M_GetText("Single player"));
		return;
	}

	// get name string for current gametype
	if (gametype >= 0 && gametype < NUMGAMETYPES)
		gametypestr = Gametype_Names[gametype];

	if (gametypestr)
		CONS_Printf(M_GetText("Current gametype is %s\n"), gametypestr);
	else // string for current gametype was not found above (should never happen)
		CONS_Printf(M_GetText("Unknown gametype set (%d)\n"), gametype);
}

/** Plays the intro.
  */
static void Command_Playintro_f(void)
{
	if (netgame)
		return;

	if (dirmenu)
		closefilemenu(true);

	F_StartIntro();
}

/** Quits the game immediately.
  */
FUNCNORETURN static ATTRNORETURN void Command_Quit_f(void)
{
	if (Playing())
		LUAh_GameQuit();
	I_Quit();
}

void ItemFinder_OnChange(void)
{
	if (!cv_itemfinder.value)
		return; // it's fine.

	if (!M_SecretUnlocked(SECRET_ITEMFINDER))
	{
		CONS_Printf(M_GetText("You haven't earned this yet.\n"));
		CV_StealthSetValue(&cv_itemfinder, 0);
		return;
	}
	else if (netgame || multiplayer)
	{
		CONS_Printf(M_GetText("This only works in single player.\n"));
		CV_StealthSetValue(&cv_itemfinder, 0);
		return;
	}
}

/** Deals with a pointlimit change by printing the change to the console.
  * If the gametype is single player, cooperative, or race, the pointlimit is
  * silently disabled again.
  *
  * Timelimit and pointlimit can be used at the same time.
  *
  * We don't check immediately for the pointlimit having been reached,
  * because you would get "caught" when turning it up in the menu.
  * \sa cv_pointlimit, TimeLimit_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void PointLimit_OnChange(void)
{
	// Don't allow pointlimit in Single Player/Co-Op/Race!
	if (server && Playing() && G_PlatformGametype())
	{
		if (cv_pointlimit.value)
			CV_StealthSetValue(&cv_pointlimit, 0);
		return;
	}

	if (cv_pointlimit.value)
	{
		CONS_Printf(M_GetText("Levels will end after %s scores %d point%s.\n"),
			G_GametypeHasTeams() ? M_GetText("a team") : M_GetText("someone"),
			cv_pointlimit.value,
			cv_pointlimit.value > 1 ? "s" : "");
	}
	else if (netgame || multiplayer)
		CONS_Printf(M_GetText("Point limit disabled\n"));
}

static void NumLaps_OnChange(void)
{
	// Just don't be verbose
	if (gametype == GT_RACE)
		CONS_Printf(M_GetText("Number of laps set to %d\n"), cv_numlaps.value);
}

static void NetTimeout_OnChange(void)
{
	connectiontimeout = (tic_t)cv_nettimeout.value;
}

static void JoinTimeout_OnChange(void)
{
	jointimeout = (tic_t)cv_jointimeout.value;
}

UINT32 timelimitintics = 0;

/** Deals with a timelimit change by printing the change to the console.
  * If the gametype is single player, cooperative, or race, the timelimit is
  * silently disabled again.
  *
  * Timelimit and pointlimit can be used at the same time.
  *
  * \sa cv_timelimit, PointLimit_OnChange
  */
static void TimeLimit_OnChange(void)
{
	// Don't allow timelimit in Single Player/Co-Op/Race!
	if (server && Playing() && cv_timelimit.value != 0 && G_PlatformGametype())
	{
		CV_SetValue(&cv_timelimit, 0);
		return;
	}

	if (cv_timelimit.value != 0)
	{
		CONS_Printf(M_GetText("Levels will end after %d minute%s.\n"),cv_timelimit.value,cv_timelimit.value == 1 ? "" : "s"); // Graue 11-17-2003
		timelimitintics = cv_timelimit.value * 60 * TICRATE;

		//add hidetime for tag too!
		if (G_TagGametype())
			timelimitintics += hidetime * TICRATE;

		// Note the deliberate absence of any code preventing
		//   pointlimit and timelimit from being set simultaneously.
		// Some people might like to use them together. It works.
	}
	else if (netgame || multiplayer)
		CONS_Printf(M_GetText("Time limit disabled\n"));
}

/** Adjusts certain settings to match a changed gametype.
  *
  * \param lastgametype The gametype we were playing before now.
  * \sa D_MapChange
  * \author Graue <graue@oceanbase.org>
  * \todo Get rid of the hardcoded stuff, ugly stuff, etc.
  */
void D_GameTypeChanged(INT32 lastgametype)
{
	if (netgame)
	{
		const char *oldgt = NULL, *newgt = NULL;

		if (lastgametype >= 0 && lastgametype < NUMGAMETYPES)
			oldgt = Gametype_Names[lastgametype];
		if (gametype >= 0 && lastgametype < NUMGAMETYPES)
			newgt = Gametype_Names[gametype];

		if (oldgt && newgt)
			CONS_Printf(M_GetText("Gametype was changed from %s to %s\n"), oldgt, newgt);
	}
	// Only do the following as the server, not as remote admin.
	// There will always be a server, and this only needs to be done once.
	if (server && (multiplayer || netgame))
	{
		if (gametype == GT_COMPETITION || gametype == GT_COOP)
			CV_SetValue(&cv_itemrespawn, 0);
		else if (!cv_itemrespawn.changed)
			CV_SetValue(&cv_itemrespawn, 1);

		switch (gametype)
		{
			case GT_MATCH:
			case GT_TEAMMATCH:
				if (!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
				{
					// default settings for match: timelimit 10 mins, no pointlimit
					CV_SetValue(&cv_pointlimit, 0);
					CV_SetValue(&cv_timelimit, 10);
				}
				if (!cv_itemrespawntime.changed)
					CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally
				break;
			case GT_TAG:
			case GT_HIDEANDSEEK:
				if (!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
				{
					// default settings for tag: 5 mins, no pointlimit
					// Note that tag mode also uses an alternate timing mechanism in tandem with timelimit.
					CV_SetValue(&cv_timelimit, 5);
					CV_SetValue(&cv_pointlimit, 0);
				}
				if (!cv_itemrespawntime.changed)
					CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally
				break;
			case GT_CTF:
				if (!cv_timelimit.changed && !cv_pointlimit.changed) // user hasn't changed limits
				{
					// default settings for CTF: no timelimit, pointlimit 5
					CV_SetValue(&cv_timelimit, 0);
					CV_SetValue(&cv_pointlimit, 5);
				}
				if (!cv_itemrespawntime.changed)
					CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue); // respawn normally
				break;
		}
	}
	else if (!multiplayer && !netgame)
	{
		gametype = GT_COOP;
		// These shouldn't matter anymore
		//CV_Set(&cv_itemrespawntime, cv_itemrespawntime.defaultvalue);
		//CV_SetValue(&cv_itemrespawn, 0);
	}

	// reset timelimit and pointlimit in race/coop, prevent stupid cheats
	if (server)
	{
		if (G_PlatformGametype())
		{
			if (cv_timelimit.value)
				CV_SetValue(&cv_timelimit, 0);
			if (cv_pointlimit.value)
				CV_SetValue(&cv_pointlimit, 0);
		}
		else if ((cv_pointlimit.changed || cv_timelimit.changed) && cv_pointlimit.value)
		{
			if (lastgametype != GT_CTF && gametype == GT_CTF)
				CV_SetValue(&cv_pointlimit, cv_pointlimit.value / 500);
			else if (lastgametype == GT_CTF && gametype != GT_CTF)
				CV_SetValue(&cv_pointlimit, cv_pointlimit.value * 500);
		}
	}

	// When swapping to a gametype that supports spectators,
	// make everyone a spectator initially.
	if (!splitscreen && (G_GametypeHasSpectators()))
	{
		INT32 i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
			{
				players[i].ctfteam = 0;
				players[i].spectator = true;
			}
	}

	// don't retain teams in other modes or between changes from ctf to team match.
	// also, stop any and all forms of team scrambling that might otherwise take place.
	if (G_GametypeHasTeams())
	{
		INT32 i;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i])
				players[i].ctfteam = 0;

		if (server || (IsPlayerAdmin(consoleplayer)))
		{
			CV_StealthSetValue(&cv_teamscramble, 0);
			teamscramble = 0;
		}
	}
}

static void Ringslinger_OnChange(void)
{
	if (!M_SecretUnlocked(SECRET_PANDORA) && !netgame && cv_ringslinger.value && !cv_debug)
	{
		CONS_Printf(M_GetText("You haven't earned this yet.\n"));
		CV_StealthSetValue(&cv_ringslinger, 0);
		return;
	}

	if (cv_ringslinger.value) // Only if it's been turned on
		G_SetGameModified(multiplayer);
}

static void Gravity_OnChange(void)
{
	if (!M_SecretUnlocked(SECRET_PANDORA) && !netgame && !cv_debug
		&& strcmp(cv_gravity.string, cv_gravity.defaultvalue))
	{
		CONS_Printf(M_GetText("You haven't earned this yet.\n"));
		CV_StealthSet(&cv_gravity, cv_gravity.defaultvalue);
		return;
	}
#ifndef NETGAME_GRAVITY
	if(netgame)
	{
		CV_StealthSet(&cv_gravity, cv_gravity.defaultvalue);
		return;
	}
#endif

	if (!CV_IsSetToDefault(&cv_gravity))
		G_SetGameModified(multiplayer);
	gravity = cv_gravity.value;
}

static void SoundTest_OnChange(void)
{
	if (cv_soundtest.value < 0)
	{
		CV_SetValue(&cv_soundtest, NUMSFX-1);
		return;
	}

	if (cv_soundtest.value >= NUMSFX)
	{
		CV_SetValue(&cv_soundtest, 0);
		return;
	}

	S_StopSounds();
	S_StartSound(NULL, cv_soundtest.value);
}

static void AutoBalance_OnChange(void)
{
	autobalance = (INT16)cv_autobalance.value;
}

static void TeamScramble_OnChange(void)
{
	INT16 i = 0, j = 0, playercount = 0;
	boolean repick = true;
	INT32 blue = 0, red = 0;
	INT32 maxcomposition = 0;
	INT16 newteam = 0;
	INT32 retries = 0;
	boolean success = false;

	// Don't trigger outside level or intermission!
	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
		return;

	if (!cv_teamscramble.value)
		teamscramble = 0;

	if (!G_GametypeHasTeams() && (server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("This command cannot be used in this gametype.\n"));
		CV_StealthSetValue(&cv_teamscramble, 0);
		return;
	}

	// If a team scramble is already in progress, do not allow another one to be started!
	if (teamscramble)
		return;

retryscramble:

	// Clear related global variables. These will get used again in p_tick.c/y_inter.c as the teams are scrambled.
	memset(&scrambleplayers, 0, sizeof(scrambleplayers));
	memset(&scrambleteams, 0, sizeof(scrambleplayers));
	scrambletotal = scramblecount = 0;
	blue = red = maxcomposition = newteam = playercount = 0;
	repick = true;

	// Put each player's node in the array.
	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && !players[i].spectator)
		{
			scrambleplayers[playercount] = i;
			playercount++;
		}
	}

	if (playercount < 2)
	{
		CV_StealthSetValue(&cv_teamscramble, 0);
		return; // Don't scramble one or zero players.
	}

	// Randomly place players on teams.
	if (cv_teamscramble.value == 1)
	{
		maxcomposition = playercount / 2;

		// Now randomly assign players to teams.
		// If the teams get out of hand, assign the rest to the other team.
		for (i = 0; i < playercount; i++)
		{
			if (repick)
				newteam = (INT16)((M_RandomByte() % 2) + 1);

			// One team has the most players they can get, assign the rest to the other team.
			if (red == maxcomposition || blue == maxcomposition)
			{
				if (red == maxcomposition)
					newteam = 2;
				else if (blue == maxcomposition)
					newteam = 1;

				repick = false;
			}

			scrambleteams[i] = newteam;

			if (newteam == 1)
				red++;
			else
				blue++;
		}
	}
	else if (cv_teamscramble.value == 2) // Same as before, except split teams based on current score.
	{
		// Now, sort the array based on points scored.
		for (i = 1; i < playercount; i++)
		{
			for (j = i; j < playercount; j++)
			{
				INT16 tempplayer = 0;

				if ((players[scrambleplayers[i-1]].score > players[scrambleplayers[j]].score))
				{
					tempplayer = scrambleplayers[i-1];
					scrambleplayers[i-1] = scrambleplayers[j];
					scrambleplayers[j] = tempplayer;
				}
			}
		}

		// Now assign players to teams based on score. Scramble in pairs.
		// If there is an odd number, one team will end up with the unlucky slob who has no points. =(
		for (i = 0; i < playercount; i++)
		{
			if (repick)
			{
				newteam = (INT16)((M_RandomByte() % 2) + 1);
				repick = false;
			}
			else
			{
				// We will only randomly pick the team for the first guy.
				// Otherwise, just alternate back and forth, distributing players.
				if (newteam == 1)
					newteam = 2;
				else
					newteam = 1;
			}

			scrambleteams[i] = newteam;
		}
	}

	// Check to see if our random selection actually
	// changed anybody. If not, we run through and try again.
	for (i = 0; i < playercount; i++)
	{
		if (players[scrambleplayers[i]].ctfteam != scrambleteams[i])
			success = true;
	}

	if (!success && retries < 5)
	{
		retries++;
		goto retryscramble; //try again
	}

	// Display a witty message, but only during scrambles specifically triggered by an admin.
	if (cv_teamscramble.value)
	{
		scrambletotal = playercount;
		teamscramble = (INT16)cv_teamscramble.value;

		if (!(gamestate == GS_INTERMISSION && cv_scrambleonchange.value))
			CONS_Printf(M_GetText("Teams will be scrambled next round.\n"));
	}
}

static void Hidetime_OnChange(void)
{
	if (Playing() && G_TagGametype() && leveltime >= (hidetime * TICRATE))
	{
		// Don't allow hidetime changes after it expires.
		CV_StealthSetValue(&cv_hidetime, hidetime);
		return;
	}
	hidetime = cv_hidetime.value;

	//uh oh, gotta change timelimitintics now too
	if (G_TagGametype())
		timelimitintics = (cv_timelimit.value * 60 * TICRATE) + (hidetime * TICRATE);
}

static void Command_Showmap_f(void)
{
	if (gamestate == GS_LEVEL)
	{
		if (mapheaderinfo[gamemap-1]->actnum)
			CONS_Printf("%s (%d): %s %d\n", G_BuildMapName(gamemap), gamemap, mapheaderinfo[gamemap-1]->lvlttl, mapheaderinfo[gamemap-1]->actnum);
		else
			CONS_Printf("%s (%d): %s\n", G_BuildMapName(gamemap), gamemap, mapheaderinfo[gamemap-1]->lvlttl);
	}
	else
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
}

static void Command_Mapmd5_f(void)
{
	if (gamestate == GS_LEVEL)
	{
		INT32 i;
		char md5tmp[33];
		for (i = 0; i < 16; ++i)
			sprintf(&md5tmp[i*2], "%02x", mapmd5[i]);
		CONS_Printf("%s: %s\n", G_BuildMapName(gamemap), md5tmp);
	}
	else
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
}

static void Command_ExitLevel_f(void)
{
	if (!(netgame || (multiplayer && gametype != GT_COOP)) && !cv_debug)
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
	else if (!(server || (IsPlayerAdmin(consoleplayer))))
		CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
	else if (( gamestate != GS_LEVEL && gamestate != GS_CREDITS ) || demoplayback)
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
	else
		SendNetXCmd(XD_EXITLEVEL, NULL, 0);
}

static void Got_ExitLevelcmd(UINT8 **cp, INT32 playernum)
{
	(void)cp;

	// Ignore duplicate XD_EXITLEVEL commands.
	if (gameaction == ga_completed)
		return;

	if (playernum != serverplayer && !IsPlayerAdmin(playernum))
	{
		CONS_Alert(CONS_WARNING, M_GetText("Illegal exitlevel command received from %s\n"), player_names[playernum]);
		if (server)
			SendKick(playernum, KICK_MSG_CON_FAIL);
		return;
	}

	G_ExitLevel();
}

/** Prints the number of the displayplayer.
  *
  * \todo Possibly remove this; it was useful for debugging at one point.
  */
static void Command_Displayplayer_f(void)
{
	CONS_Printf(M_GetText("Displayplayer is %d\n"), displayplayer);
}

/** Quits a game and returns to the title screen.
  *
  */
void Command_ExitGame_f(void)
{
	INT32 i;

	if (Playing())
		LUAh_GameQuit();

	D_QuitNetGame();
	CL_Reset();
	CV_ClearChangedFlags();

	for (i = 0; i < MAXPLAYERS; i++)
		CL_ClearPlayer(i);

	splitscreen = false;
	SplitScreen_OnChange();
	botingame = false;
	botskin = 0;
	cv_debug = 0;
	emeralds = 0;

	if (dirmenu)
		closefilemenu(true);

	if (!modeattacking)
		D_StartTitle();
}

void Command_Retry_f(void)
{
	if (!(gamestate == GS_LEVEL || gamestate == GS_INTERMISSION))
		CONS_Printf(M_GetText("You must be in a level to use this.\n"));
	else if (netgame || multiplayer)
		CONS_Printf(M_GetText("This only works in single player.\n"));
	else if (players[consoleplayer].lives <= 1)
		CONS_Printf(M_GetText("You can't retry without any lives remaining!\n"));
	else if (G_IsSpecialStage(gamemap))
		CONS_Printf(M_GetText("You can't retry special stages!\n"));
	else
	{
		M_ClearMenus(true);
		G_SetRetryFlag();
	}
}

#ifdef NETGAME_DEVMODE
// Allow the use of devmode in netgames.
static void Fishcake_OnChange(void)
{
	cv_debug = cv_fishcake.value;
	// consvar_t's get changed to default when registered
	// so don't make modifiedgame always on!
	if (cv_debug)
	{
		G_SetGameModified(multiplayer);
	}

	else if (cv_debug != cv_fishcake.value)
		CV_SetValue(&cv_fishcake, cv_debug);
}
#endif

/** Reports to the console whether or not the game has been modified.
  *
  * \todo Make it obvious, so a console command won't be necessary.
  * \sa modifiedgame
  * \author Graue <graue@oceanbase.org>
  */
static void Command_Isgamemodified_f(void)
{
	if (savemoddata)
		CONS_Printf(M_GetText("modifiedgame is true, but you can save emblem and time data in this mod.\n"));
	else if (modifiedgame)
		CONS_Printf(M_GetText("modifiedgame is true, secrets will not be unlocked\n"));
	else
		CONS_Printf(M_GetText("modifiedgame is false, you can unlock secrets\n"));
}

static void Command_Cheats_f(void)
{
	if (COM_CheckParm("off"))
	{
		if (!(server || (IsPlayerAdmin(consoleplayer))))
			CONS_Printf(M_GetText("Only the server or a remote admin can use this.\n"));
		else
			CV_ResetCheatNetVars();
		return;
	}

	if (CV_CheatsEnabled())
	{
		CONS_Printf(M_GetText("At least one CHEAT-marked variable has been changed -- Cheats are enabled.\n"));
		if (server || (IsPlayerAdmin(consoleplayer)))
			CONS_Printf(M_GetText("Type CHEATS OFF to reset all cheat variables to default.\n"));
	}
	else
		CONS_Printf(M_GetText("No CHEAT-marked variables are changed -- Cheats are disabled.\n"));
}

#ifdef _DEBUG
static void Command_Togglemodified_f(void)
{
	modifiedgame = !modifiedgame;
}

extern UINT8 *save_p;
static void Command_Archivetest_f(void)
{
	UINT8 *buf;
	UINT32 i, wrote;
	thinker_t *th;
	if (gamestate != GS_LEVEL)
	{
		CONS_Printf("This command only works in-game, you dummy.\n");
		return;
	}

	// assign mobjnum
	i = 1;
	for (th = thinkercap.next; th != &thinkercap; th = th->next)
		if (th->function == (actionf_p1)P_MobjThinker)
			((mobj_t *)th)->mobjnum = i++;

	// allocate buffer
	buf = save_p = ZZ_Alloc(1024);

	// test archive
	CONS_Printf("LUA_Archive...\n");
	LUA_Archive();
	WRITEUINT8(save_p, 0x7F);
	wrote = (UINT32)(save_p-buf);

	// clear Lua state, so we can really see what happens!
	CONS_Printf("Clearing state!\n");
	LUA_ClearExtVars();

	// test unarchive
	save_p = buf;
	CONS_Printf("LUA_UnArchive...\n");
	LUA_UnArchive();
	i = READUINT8(save_p);
	if (i != 0x7F || wrote != (UINT32)(save_p-buf))
		CONS_Printf("Savegame corrupted. (write %u, read %u)\n", wrote, (UINT32)(save_p-buf));

	// free buffer
	Z_Free(buf);
	CONS_Printf("Done. No crash.\n");
}
#endif

/** Makes a change to ::cv_forceskin take effect immediately.
  *
  * \todo Move the enforcement code out of SendNameAndColor() so this hack
  *       isn't needed.
  * \sa Command_SetForcedSkin_f, cv_forceskin, forcedskin
  * \author Graue <graue@oceanbase.org>
  */
static void ForceSkin_OnChange(void)
{
	if ((server || IsPlayerAdmin(consoleplayer)) && (cv_forceskin.value < -1 || cv_forceskin.value >= numskins))
	{
		if (cv_forceskin.value == -2)
			CV_SetValue(&cv_forceskin, numskins-1);
		else
		{
			// hack because I can't restrict this and still allow added skins to be used with forceskin.
			if (!menuactive)
				CONS_Printf(M_GetText("Valid skin numbers are 0 to %d (-1 disables)\n"), numskins - 1);
			CV_SetValue(&cv_forceskin, -1);
		}
		return;
	}

	// NOT in SP, silly!
	if (!(netgame || multiplayer))
		return;

	if (cv_forceskin.value < 0)
		CONS_Printf("The server has lifted the forced skin restrictions.\n");
	else
	{
		CONS_Printf("The server is restricting all players to skin \"%s\".\n",skins[cv_forceskin.value].name);
		ForceAllSkins(cv_forceskin.value);
	}
}

//Allows the player's name to be changed if cv_mute is off.
static void Name_OnChange(void)
{
	if (cv_mute.value && !(server || IsPlayerAdmin(consoleplayer)))
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You may not change your name when chat is muted.\n"));
		CV_StealthSet(&cv_playername, player_names[consoleplayer]);
	}
	else
		SendNameAndColor();

}

static void Name2_OnChange(void)
{
	if (cv_mute.value) //Secondary player can't be admin.
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You may not change your name when chat is muted.\n"));
		CV_StealthSet(&cv_playername2, player_names[secondarydisplayplayer]);
	}
	else
		SendNameAndColor2();
}

/** Sends a skin change for the console player, unless that player is moving.
  * \sa cv_skin, Skin2_OnChange, Color_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Skin_OnChange(void)
{
	if (!Playing())
		return; // do whatever you want

	if (!(cv_debug || devparm) && !(multiplayer || netgame) // In single player.
		&& (gamestate != GS_WAITINGPLAYERS)) // allows command line -warp x +skin y
	{
		CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);
		return;
	}

	if (CanChangeSkin(consoleplayer) && !P_PlayerMoving(consoleplayer))
		SendNameAndColor();
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You can't change your skin at the moment.\n"));
		CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);
	}
}

/** Sends a skin change for the secondary splitscreen player, unless that
  * player is moving.
  * \sa cv_skin2, Skin_OnChange, Color2_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Skin2_OnChange(void)
{
	if (!Playing() || !splitscreen)
		return; // do whatever you want

	if (CanChangeSkin(secondarydisplayplayer) && !P_PlayerMoving(secondarydisplayplayer))
		SendNameAndColor2();
	else
	{
		CONS_Alert(CONS_NOTICE, M_GetText("You can't change your skin at the moment.\n"));
		CV_StealthSet(&cv_skin2, skins[players[secondarydisplayplayer].skin].name);
	}
}

/** Sends a color change for the console player, unless that player is moving.
  * \sa cv_playercolor, Color2_OnChange, Skin_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Color_OnChange(void)
{
	if (!Playing())
		return; // do whatever you want

	if (!(cv_debug || devparm) && !(multiplayer || netgame)) // In single player.
	{
		CV_StealthSet(&cv_skin, skins[players[consoleplayer].skin].name);
		return;
	}

	if (!P_PlayerMoving(consoleplayer))
	{
		// Color change menu scrolling fix is no longer necessary
		SendNameAndColor();
	}
	else
	{
		CV_StealthSetValue(&cv_playercolor,
			players[consoleplayer].skincolor);
	}
}

/** Sends a color change for the secondary splitscreen player, unless that
  * player is moving.
  * \sa cv_playercolor2, Color_OnChange, Skin2_OnChange
  * \author Graue <graue@oceanbase.org>
  */
static void Color2_OnChange(void)
{
	if (!Playing() || !splitscreen)
		return; // do whatever you want

	if (!P_PlayerMoving(secondarydisplayplayer))
	{
		// Color change menu scrolling fix is no longer necessary
		SendNameAndColor2();
	}
	else
	{
		CV_StealthSetValue(&cv_playercolor2,
			players[secondarydisplayplayer].skincolor);
	}
}

/** Displays the result of the chat being muted or unmuted.
  * The server or remote admin should already know and be able to talk
  * regardless, so this is only displayed to clients.
  *
  * \sa cv_mute
  * \author Graue <graue@oceanbase.org>
  */
static void Mute_OnChange(void)
{
	if (server || (IsPlayerAdmin(consoleplayer)))
		return;

	if (cv_mute.value)
		CONS_Printf(M_GetText("Chat has been muted.\n"));
	else
		CONS_Printf(M_GetText("Chat is no longer muted.\n"));
}

/** Hack to clear all changed flags after game start.
  * A lot of code (written by dummies, obviously) uses COM_BufAddText() to run
  * commands and change consvars, especially on game start. This is problematic
  * because CV_ClearChangedFlags() needs to get called on game start \b after
  * all those commands are run.
  *
  * Here's how it's done: the last thing in COM_BufAddText() is "dummyconsvar
  * 1", so we end up here, where dummyconsvar is reset to 0 and all the changed
  * flags are set to 0.
  *
  * \todo Fix the aforementioned code and make this hack unnecessary.
  * \sa cv_dummyconsvar
  * \author Graue <graue@oceanbase.org>
  */
static void DummyConsvar_OnChange(void)
{
	if (cv_dummyconsvar.value == 1)
	{
		CV_SetValue(&cv_dummyconsvar, 0);
		CV_ClearChangedFlags();
	}
}

static void Command_ShowScores_f(void)
{
	UINT8 i;

	if (!(netgame || multiplayer))
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
			// FIXME: %lu? what's wrong with %u? ~Callum (produces warnings...)
			CONS_Printf(M_GetText("%s's score is %u\n"), player_names[i], players[i].score);
	}
	CONS_Printf(M_GetText("The pointlimit is %d\n"), cv_pointlimit.value);

}

static void Command_ShowTime_f(void)
{
	if (!(netgame || multiplayer))
	{
		CONS_Printf(M_GetText("This only works in a netgame.\n"));
		return;
	}

	CONS_Printf(M_GetText("The current time is %f.\nThe timelimit is %f\n"), (double)leveltime/TICRATE, (double)timelimitintics/TICRATE);
}
