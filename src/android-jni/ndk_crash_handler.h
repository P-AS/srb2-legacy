// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by SRB2 Mobile Project.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  ndk_crash_handler.h
/// \brief Android crash handler

#ifdef __cplusplus
extern "C" {
#endif
void NDKCrashHandler_ReportSignal(const char *sigmsg);
#ifdef __cplusplus
}
#endif
