// Emacs style mode select   -*- C++ -*-
//
// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 2014-2018 by Sonic Team Junior.
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
// Changes by Graue <graue@oceanbase.org> are in the public domain.
//
//-----------------------------------------------------------------------------
/// \file
/// \brief SRB2 system stuff for NDS

#include "../doomdef.h"
#include "../doomstat.h"
#include "../d_clisrv.h"
#include "../d_main.h"
#include "../d_netcmd.h"
#include "../filesrch.h"
#include "../g_state.h"
#include "../g_game.h"
#include "../m_misc.h"
#include "../i_system.h"
#include "../i_video.h"
#include "../i_sound.h"
#include "../i_joy.h"
#include "../z_zone.h"

#include <nds.h>
#include <filesystem.h>

UINT8 graphics_started = 0;
UINT8 keyboard_started = 0;

static volatile tic_t ticcount;

#define timers2ms(tlow,thigh) ((tlow>>5)+(thigh<<11))

// Handy DSdev.org timer functions
u32 GetTicks(void)
{
	return timers2ms(TIMER0_DATA, TIMER1_DATA);
} 

void Pause(u32 ms)
{
	u32 now;
	now=timers2ms(TIMER0_DATA, TIMER1_DATA);
	while((u32)timers2ms(TIMER0_DATA, TIMER1_DATA)<now+ms);
}

void I_Sleep(UINT32 ms)
{
	Pause(ms/1000);
}

int ms_to_next_tick;

// !!
tic_t I_GetTime(void)
{
    int t = GetTicks();
    int i = t*(TICRATE/5)/200;
    ms_to_next_tick = (i+1)*200/(TICRATE/5) - t;
    if (ms_to_next_tick > 1000/TICRATE || ms_to_next_tick<1) ms_to_next_tick = 1;
    return i;
}

void I_SleepDuration(precise_t duration)
{
	UINT64 precision = I_GetPrecisePrecision();
	INT32 sleepvalue = cv_sleep.value;
	UINT64 delaygranularity;
	precise_t cur;
	precise_t dest;

	{
		double gran = round(((double)(precision / 1000) * sleepvalue * 2.1));
		delaygranularity = (UINT64)gran;
	}

	cur = I_GetPreciseTime();
	dest = cur + duration;

	// the reason this is not dest > cur is because the precise counter may wrap
	// two's complement arithmetic is our friend here, though!
	// e.g. cur 0xFFFFFFFFFFFFFFFE = -2, dest 0x0000000000000001 = 1
	// 0x0000000000000001 - 0xFFFFFFFFFFFFFFFE = 3
	while ((INT64)(dest - cur) > 0)
	{
		// If our cv_sleep value exceeds the remaining sleep duration, use the
		// hard sleep function.
		if (sleepvalue > 0 && (dest - cur) > delaygranularity)
		{
			I_Sleep(sleepvalue);
		}

		// Otherwise, this is a spinloop.

		cur = I_GetPreciseTime();
	}
}

size_t I_GetFreeMem(size_t *total)
{
	*total = 12*1024*1024;
	return 12*1024*1024;
}

precise_t I_GetPreciseTime(void) {
	return 0;
}

UINT64 I_GetPrecisePrecision(void) {
	return 1000000000;
}

// this is really long
void I_GetEvent(void)
{
    static touchPosition last_touch_position;
	scanKeys();
	u16 keys = keysDown();
	
	event_t e_w;

	if (keys & KEY_A) {
		event_t event;
		event.type = ev_keydown;
		event.data1 = KEY_ENTER;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_B) {
		event_t event;
		event.type = ev_keydown;
		event.data1 = KEY_LSHIFT;
		D_PostEvent(&event);
	}

	if (keys & KEY_Y) {
		event_t event;
		event.type = ev_keydown;
		event.data1 = 'v';
		D_PostEvent(&event);
	}
	
	if (keys & KEY_START) {
		event_t event;
		event.type = ev_keydown;
		event.data1 = KEY_ESCAPE;
		D_PostEvent(&event);
	}

	if (keys & KEY_UP) {
		event_t event;
		event.type = ev_keydown;
		event.data1 = KEY_UPARROW;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_DOWN) {
		event_t event;
		event.type = ev_keydown;
		event.data1 = KEY_DOWNARROW;
		D_PostEvent(&event);
	}

	if (keys & KEY_LEFT) {
		event_t event;
		event.type = ev_keydown;
		event.data1 = KEY_LEFTARROW;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_RIGHT) {
		event_t event;
		event.type = ev_keydown;
		event.data1 = KEY_RIGHTARROW;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_L) {
		event_t event;
		event.type = ev_keydown;
			event.data1 = 'a';
		D_PostEvent(&event);
	}
	
	if (keys & KEY_R) {
		event_t event;
		event.type = ev_keydown;
		event.data1 = 'd';
		D_PostEvent(&event);
	}

    // lowk took this from srb2_3ds
	if(keysHeld() & KEY_TOUCH) {
        event_t event;
		touchPosition current_touch_position;
		touchRead(&current_touch_position);
		if (!(keysDown() & KEY_TOUCH)) {
			event.type = ev_mouse;
			event.data1 = 0;
			event.data2 = (current_touch_position.px - last_touch_position.px);
			event.data3 = -(current_touch_position.py - last_touch_position.py);
			D_PostEvent(&event);
		}
		last_touch_position = current_touch_position;
	}
	
	keys = keysUp();
	
	if (keys & KEY_A) {
		event_t event;
		event.type = ev_keyup;
		event.data1 = KEY_ENTER;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_B) {
		event_t event;
		event.type = ev_keyup;
		event.data1 = KEY_LSHIFT;
		D_PostEvent(&event);
	}

	if (keys & KEY_Y) {
		event_t event;
		event.type = ev_keydown;
		event.data1 = 'v';
		D_PostEvent(&event);
	}
	
	if (keys & KEY_START) {
		event_t event;
		event.type = ev_keyup;
		event.data1 = KEY_ESCAPE;
		D_PostEvent(&event);
	}

	if (keys & KEY_UP) {
		event_t event;
		event.type = ev_keyup;
		event.data1 = KEY_UPARROW;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_DOWN) {
		event_t event;
		event.type = ev_keyup;
		event.data1 = KEY_DOWNARROW;
		D_PostEvent(&event);
	}

	if (keys & KEY_LEFT) {
		event_t event;
		event.type = ev_keyup;
		event.data1 = KEY_LEFTARROW;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_RIGHT) {
		event_t event;
		event.type = ev_keyup;
		event.data1 = KEY_RIGHTARROW;
		D_PostEvent(&event);
	}
	
	if (keys & KEY_L) {
		event_t event;
		event.type = ev_keyup;
		event.data1 = 'a';
		D_PostEvent(&event);
	}
	
	if (keys & KEY_R) {
		event_t event;
		event.type = ev_keyup;
		event.data1 = 'd';
		D_PostEvent(&event);
	}

	// keyboard
	int16_t c = keyboardUpdate();
	if (c != -1)
	{
		event_t event;

		// backspace
		if (c == '\b')
		{
			event.type = ev_keydown;
			event.data1 = KEY_BACKSPACE;
			D_PostEvent(&event);

			event.type = ev_keyup;
			event.data1 = KEY_BACKSPACE;
			D_PostEvent(&event);
		}
		else if (c >= 32)
		{
			// key down
			event.type = ev_keydown;
			event.data1 = c;
			D_PostEvent(&event);

			// key up
			event.type = ev_keyup;
			event.data1 = c;
			D_PostEvent(&event);
		}
	}
}

void I_OsPolling(void)
{
	I_GetEvent();
}

static ticcmd_t emptycmd;

ticcmd_t *I_BaseTiccmd(void)
{
	return &emptycmd;
}

static ticcmd_t emptycmd2;

ticcmd_t *I_BaseTiccmd2(void)
{
	return &emptycmd2;
}

void I_Quit(void)
{
	exit(0);
}

void I_Error(const char *error, ...)
{
    // Format the error string
    va_list args;
    va_start(args, error);

    int len = vsnprintf(NULL, 0, error, args);
    va_end(args);

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), error, args);
    printf("I_Error: %s\n", buffer);


    M_SaveConfig(NULL);
    D_QuitNetGame();
    I_ShutdownGraphics();
    I_ShutdownSound();
    I_ShutdownMusic();
    I_ShutdownSystem();

    while(1) 
	{
        swiWaitForVBlank();
    }
}

void I_Tactile(FFType Type, const JoyFF_t *Effect)
{
	(void)Type;
	(void)Effect;
}

void I_Tactile2(FFType Type, const JoyFF_t *Effect)
{
	(void)Type;
	(void)Effect;
}

void I_CursedWindowMovement(int xd, int yd)
{
	(void)xd;
	(void)yd;
}

const char *I_GetPlatform(void)
{
	return "Nintendo DS";
}

void I_JoyScale(void){}

void I_JoyScale2(void){}

void I_InitJoystick(void){}

void I_InitJoystick2(void){}

INT32 I_NumJoys(void)
{
	return 0;
}

const char *I_GetJoyName(INT32 joyindex)
{
	(void)joyindex;
	return NULL;
}

#ifndef NOMUMBLE
void I_UpdateMumble(const mobj_t *mobj, const listener_t listener)
{
	(void)mobj;
	(void)listener;
}
#endif

void I_OutputMsg(const char *error, ...)
{
	va_list args;
	va_start(args, error);

	int len = vsnprintf(NULL, 0, error, args);
	va_end(args);

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), error, args);

	printf(buffer);
}

void I_StartupMouse(void){}

void I_StartupMouse2(void){}

void I_StartupKeyboard(void){}

INT32 I_GetKey(void)
{
	return 0;
}

static void NDS_VBlankHandler(void)
{
	ticcount++;
}

void I_StartupTimer(void)
{
	irqSet(IRQ_VBLANK, NDS_VBlankHandler);
}

void I_AddExitFunc(void (*func)())
{
	(void)func;
}

void I_RemoveExitFunc(void (*func)())
{
	(void)func;
}

INT32 I_StartupSystem(void)
{
	return -1;
}

void I_ShutdownSystem(void){}

void I_GetDiskFreeSpace(INT64* freespace)
{
	*freespace = 0;
}

char *I_GetUserName(void)
{
	return NULL;
}

INT32 I_mkdir(const char *dirname, INT32 unixright)
{
	(void)dirname;
	(void)unixright;
	return -1;
}

const char *I_LocateWad(void)
{
	chdir("nitro:/");
	return "nitro:/";
}

void I_GetJoystickEvents(void){}

void I_GetJoystick2Events(void){}

void I_GetMouseEvents(void){}

void I_UpdateMouseGrab(void){}

void I_SetTextInputMode(boolean active)
{
	(void)active;
}

boolean I_GetTextInputMode(void)
{
	return false;
}

char *I_GetEnv(const char *name)
{
	(void)name;
	return NULL;
}

INT32 I_PutEnv(char *variable)
{
	(void)variable;
	return -1;
}

INT32 I_ClipboardCopy(const char *data, size_t size)
{
	(void)data;
	(void)size;
	return -1;
}

const char *I_ClipboardPaste(void)
{
	return NULL;
}

const char *I_ConfigDir(void)
{
	return "";
}

size_t I_GetRandomBytes(char *destination, size_t amount)
{
	return 0;
}

void I_RegisterSysCommands(void) {}

int I_OpenURL(const char *url)
{
	(void)url;
	return -1;
}

#include "../sdl/dosstr.c"
