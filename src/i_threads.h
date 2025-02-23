// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020-2023 by James R.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  i_threads.h
/// \brief Multithreading abstraction

#ifdef HAVE_THREADS

#ifndef I_THREADS_H
#define I_THREADS_H

typedef void (*thread_fn_t)(void *userdata);

typedef void *mutex_t;
typedef void *cond_t;

void I_StartThreads(void);
void I_StopThreads(void);

void I_SpawnThread(const char *name, thread_fn_t, void *userdata);

/* check in your thread whether to return early */
int I_ThreadIsStopped(void);

void I_LockMutex(mutex_t *);
void I_UnlockMutex(mutex_t);

void I_HoldCond(cond_t *, mutex_t);

void I_WakeOneCond(cond_t *);
void I_WakeAllCond(cond_t *);

#endif/*I_THREADS_H*/
#endif/*HAVE_THREADS*/
