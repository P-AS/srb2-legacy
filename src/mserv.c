// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2018 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  mserv.c
/// \brief Commands used for communicate with the master server

#ifdef __GNUC__
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#endif

#include <time.h>

#if (defined (NOMD5) || defined (NOMSERV)) && !defined (NONET)
#define NONET
#endif

#ifndef NONET

#ifndef NO_IPV6
#define HAVE_IPV6
#endif

#ifdef _WIN32
#define RPC_NO_WINDOWS_H
#ifdef HAVE_IPV6
#include <ws2tcpip.h>
#else
#include <winsock.h>     // socket(),...
#endif //!HAVE_IPV6

#else
#include <arpa/inet.h>
#ifdef __APPLE_CC__
#ifndef _BSD_SOCKLEN_T_
#define _BSD_SOCKLEN_T_
#endif
#endif
#include <sys/socket.h> // socket(),...
#include <netinet/in.h> // sockaddr_in
#include <netdb.h> // getaddrinfo(),...
#include <sys/ioctl.h>

#include <sys/time.h> // timeval,... (TIMEOUT)
#include <errno.h>
#endif // _WIN32

#endif // !NONET

#include "doomstat.h"
#include "doomdef.h"
#include "command.h"
#include "i_net.h"
#include "console.h"
#include "mserv.h"
#include "d_net.h"
#include "i_tcp.h"
#include "i_system.h"
#include "byteptr.h"
#include "m_menu.h"
#include "m_argv.h" // Alam is going to kill me <3
#include "m_misc.h" //  GetRevisionString()
#include "i_threads.h"

#include "i_addrinfo.h"

// ================================ DEFINITIONS ===============================

#define PACKET_SIZE 1024


#define  MS_NO_ERROR            	   0
#define  MS_SOCKET_ERROR        	-201
#define  MS_CONNECT_ERROR       	-203
#define  MS_WRITE_ERROR         	-210
#define  MS_READ_ERROR          	-211
#define  MS_CLOSE_ERROR         	-212
#define  MS_GETHOSTBYNAME_ERROR 	-220
#define  MS_GETHOSTNAME_ERROR   	-221
#define  MS_TIMEOUT_ERROR       	-231

// see master server code for the values
#define ADD_SERVER_MSG          	101
#define REMOVE_SERVER_MSG       	103
#define ADD_SERVERv2_MSG        	104
#define GET_SERVER_MSG          	200
#define GET_SHORT_SERVER_MSG    	205
#define ASK_SERVER_MSG          	206
#define ANSWER_ASK_SERVER_MSG   	207
#define ASK_SERVER_MSG          	206
#define ANSWER_ASK_SERVER_MSG   	207
#define GET_MOTD_MSG            	208
#define SEND_MOTD_MSG           	209
#define GET_ROOMS_MSG           	210
#define SEND_ROOMS_MSG          	211
#define GET_ROOMS_HOST_MSG      	212
#define GET_VERSION_MSG         	213
#define SEND_VERSION_MSG        	214
#define GET_BANNED_MSG          	215 // Someone's been baaaaaad!
#define PING_SERVER_MSG         	216
#define GET_EXT_SERVER_MSG      	217

#define HEADER_SIZE (sizeof (INT32)*4)

#define HEADER_MSG_POS    0
#define IP_MSG_POS       16
#define PORT_MSG_POS     32
#define HOSTNAME_MSG_POS 40

#define CONNECT_IPV4 1
#define CONNECT_IPV6 2

/** A message to be exchanged with the master server.
  */
typedef struct
{
	INT32 id;                  ///< Unused?
	INT32 type;                ///< Type of message.
	INT32 room;                ///< Because everyone needs a roomie.
	UINT32 length;             ///< Length of the message.
	char buffer[PACKET_SIZE]; ///< Actual contents of the message.
} ATTRPACK msg_t;

typedef struct Copy_CVarMS_t
{
	char ip[64];
	char port[8];
	char name[64];
} Copy_CVarMS_s;
static Copy_CVarMS_s registered_server;
static time_t MSLastPing;

typedef struct
{
	char ip[16];         // Big enough to hold a full address.
	UINT16 port;
	UINT8 padding1[2];
	tic_t time;
} ATTRPACK ms_holepunch_packet_t;

// win32
#ifdef _WIN32
#define ioctl ioctlsocket
#define close closesocket
#ifdef _WIN32
#undef errno
#define errno h_errno // some very strange things happen when not using h_error
#endif
#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0x00000400
#endif
#endif

#ifndef NONET
static void Command_Listserv_f(void);
#endif
static void MasterServer_OnChange(void);
static void ServerName_OnChange(void);

#define DEF_PORT "28900"
consvar_t cv_masterserver = CVAR_INIT ("masterserver", "ms.srb2classic.net:"DEF_PORT, "The URL for the Master Server", CV_SAVE, NULL, MasterServer_OnChange);
consvar_t cv_servername = CVAR_INIT ("servername", "SRB2 server", "Name of the server as displayed on the server listing", CV_SAVE|CV_CALL|CV_NOINIT, NULL, ServerName_OnChange);

INT16 ms_RoomId = -1;

enum { MSCS_NONE, MSCS_REGISTERED, MSCS_FAILED };
static int con_state = MSCS_NONE;
static int con6_state = MSCS_NONE;

UINT16 current_port = 0;

#if defined (_WIN32) && !defined (NONET)
typedef SOCKET SOCKET_TYPE;
#define ERRSOCKET (SOCKET_ERROR)
#else
#if defined (__unix__) || defined (__APPLE__) || defined (__HAIKU__)
typedef int SOCKET_TYPE;
#else
typedef unsigned long SOCKET_TYPE;
#endif
#define ERRSOCKET (-1)
#endif

#ifdef USE_WINSOCK1
typedef int socklen_t;
#endif

#ifndef NONET
static size_t recvfull(SOCKET_TYPE s, char *buf, size_t len, int flags);
#endif

// Room list is an external variable now.
// Avoiding having to get info ten thousand times...
msg_rooms_t room_list[NUM_LIST_ROOMS+1]; // +1 for easy test

/** Adds variables and commands relating to the master server.
  *
  * \sa cv_masterserver, cv_servername,
  *     Command_Listserv_f
  */
void AddMServCommands(void)
{
#ifndef NONET
	CV_RegisterVar(&cv_masterserver);
	CV_RegisterVar(&cv_servername);
	COM_AddCommand("listserv", NULL,  Command_Listserv_f);
#endif
}

/** Closes the connection to the master server.
  *
  * \todo Fix for Windows?
  */
static void CloseConnection(SOCKET_TYPE fd)
{
#ifndef NONET
	close(fd);
#endif
}

//
// MS_Write():
//
static INT32 MS_Write(SOCKET_TYPE fd, msg_t *msg)
{
#ifdef NONET
	(void)msg;
	return MS_WRITE_ERROR;
#else
	size_t len;

	len = msg->length + HEADER_SIZE;

	msg->type = htonl(msg->type);
	msg->length = htonl(msg->length);
	msg->room = htonl(msg->room);

	if ((size_t)send(fd, (char *)msg, (int)len, 0) != len)
		return MS_WRITE_ERROR;
	return 0;
#endif
}

//
// MS_Read():
//
static INT32 MS_Read(SOCKET_TYPE fd, msg_t *msg)
{
#ifdef NONET
	(void)msg;
	return MS_READ_ERROR;
#else
	if (recvfull(fd, (char *)msg, HEADER_SIZE, 0) != HEADER_SIZE)
		return MS_READ_ERROR;

	msg->type = ntohl(msg->type);
	msg->length = ntohl(msg->length);
	msg->room = ntohl(msg->room);

	if (!msg->length) // fix a bug in Windows 2000
		return 0;

	if (recvfull(fd, (char *)msg->buffer, msg->length, 0) != msg->length)
		return MS_READ_ERROR;
	return 0;
#endif
}

#ifndef NONET
/** Gets a list of game servers from the master server.
  */
static INT32 GetServersList(SOCKET_TYPE fd)
{
	msg_t msg;
	INT32 count = 0;

	msg.type = GET_SERVER_MSG;
	msg.length = 0;
	msg.room = 0;
	if (MS_Write(fd, &msg) < 0)
		return MS_WRITE_ERROR;

	while (MS_Read(fd, &msg) >= 0)
	{
		if (!msg.length)
		{
			if (!count)
				CONS_Alert(CONS_NOTICE, M_GetText("No servers currently running.\n"));
			return MS_NO_ERROR;
		}
		count++;
		CONS_Printf("%s",msg.buffer);
	}

	return MS_READ_ERROR;
}
#endif

//
// MS_Connect()
//
static INT32 MS_Connect(const char *ip_addr, const char *str_port, int flags)
{
#ifdef NONET
	(void)ip_addr;
	(void)str_port;
#else
	struct my_addrinfo *ai, *runp, hints;
	int gaie;

	memset (&hints, 0x00, sizeof(hints));
#ifdef AI_ADDRCONFIG
	hints.ai_flags = AI_ADDRCONFIG;
#endif
#ifdef HAVE_IPV6
	if (flags & CONNECT_IPV4)
		hints.ai_family = AF_INET;
	else if (flags & CONNECT_IPV6)
		hints.ai_family = AF_INET6;
	else
		hints.ai_family = AF_UNSPEC;
#else
	hints.ai_family = AF_INET;
#endif
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//I_InitTcpNetwork(); this is already done on startup in D_SRB2Main()
	if (!I_InitTcpDriver()) // this is done only if not already done
		return ERRSOCKET;

	gaie = I_getaddrinfo(ip_addr, str_port, &hints, &ai);
	if (gaie != 0)
		return ERRSOCKET;
	else
		runp = ai;

	while (runp != NULL)
	{
		SOCKET_TYPE fd = socket(runp->ai_family, runp->ai_socktype, runp->ai_protocol);
		if (fd != (SOCKET_TYPE)ERRSOCKET)
		{
			if (connect(fd, runp->ai_addr, (socklen_t)runp->ai_addrlen) != ERRSOCKET)
			{
				I_freeaddrinfo(ai);
				return fd;
			}

			CloseConnection(fd);
		}
		runp = runp->ai_next;
	}
	I_freeaddrinfo(ai);
#endif
	return ERRSOCKET;
}

#define NUM_LIST_SERVER MAXSERVERLIST
const msg_ext_server_t *GetShortServersList(INT32 room)
{
	static msg_ext_server_t server_list[NUM_LIST_SERVER+1]; // +1 for easy test
	msg_t msg;
	INT32 i;

	// we must be connected to the master server before writing to it
	SOCKET_TYPE fd = MS_Connect(GetMasterServerIP(), GetMasterServerPort(), 0);
	if (fd == ERRSOCKET)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot connect to the Master Server\n"));
		M_StartMessage(M_GetText("There was a problem connecting to\nthe Master Server\n"), NULL, MM_NOTHING);
		return NULL;
	}

	msg.type = GET_EXT_SERVER_MSG;
	msg.length = 0;
	msg.room = room;
	if (MS_Write(fd, &msg) < 0)
	{
		CloseConnection(fd);
		return NULL;
	}

	for (i = 0; i < NUM_LIST_SERVER && MS_Read(fd, &msg) >= 0; i++)
	{
		if (!msg.length)
		{
			server_list[i].header.buffer[0] = 0;
			CloseConnection(fd);
			return server_list;
		}
		M_Memcpy(&server_list[i], msg.buffer, sizeof (msg_ext_server_t));
		server_list[i].header.buffer[0] = 1;
	}
	CloseConnection(fd);
	if (i == NUM_LIST_SERVER)
	{
		server_list[i].header.buffer[0] = 0;
		return server_list;
	}
	else
		return NULL;
}

INT32 GetRoomsList(boolean hosting)
{
	static msg_ban_t banned_info[1];
	msg_t msg;
	INT32 i;

	// we must be connected to the master server before writing to it
	SOCKET_TYPE fd = MS_Connect(GetMasterServerIP(), GetMasterServerPort(), 0);
	if (fd == ERRSOCKET)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot connect to the Master Server\n"));
		M_StartMessage(M_GetText("There was a problem connecting to\nthe Master Server\n"), NULL, MM_NOTHING);
		return -1;
	}

	if (hosting)
		msg.type = GET_ROOMS_HOST_MSG;
	else
		msg.type = GET_ROOMS_MSG;
	msg.length = 0;
	msg.room = 0;
	if (MS_Write(fd, &msg) < 0)
	{
		room_list[0].id = 1;
		strcpy(room_list[0].motd,"Master Server Offline.");
		strcpy(room_list[0].name,"Offline");
		CloseConnection(fd);
		return -1;
	}

	for (i = 0; i < NUM_LIST_ROOMS && MS_Read(fd, &msg) >= 0; i++)
	{
		if(msg.type == GET_BANNED_MSG)
		{
			char banmsg[1000];
			M_Memcpy(&banned_info[0], msg.buffer, sizeof (msg_ban_t));
			if (hosting)
				sprintf(banmsg, M_GetText("You have been banned from\nhosting netgames.\n\nUnder the following IP Range:\n%s - %s\n\nFor the following reason:\n%s\n\nYour ban will expire on:\n%s"),banned_info[0].ipstart,banned_info[0].ipend,banned_info[0].reason,banned_info[0].endstamp);
			else
				sprintf(banmsg, M_GetText("You have been banned from\njoining netgames.\n\nUnder the following IP Range:\n%s - %s\n\nFor the following reason:\n%s\n\nYour ban will expire on:\n%s"),banned_info[0].ipstart,banned_info[0].ipend,banned_info[0].reason,banned_info[0].endstamp);
			M_StartMessage(banmsg, NULL, MM_NOTHING);
			CloseConnection(fd);
			ms_RoomId = -1;
			return -2;
		}
		if (!msg.length)
		{
			room_list[i].header.buffer[0] = 0;
			CloseConnection(fd);
			return 1;
		}
		M_Memcpy(&room_list[i], msg.buffer, sizeof (msg_rooms_t));
		room_list[i].header.buffer[0] = 1;
	}

	CloseConnection(fd);
	if (i == NUM_LIST_ROOMS)
	{
		room_list[i].header.buffer[0] = 0;
		return 1;
	}
	else
	{
		room_list[0].id = 1;
		strcpy(room_list[0].motd,M_GetText("Master Server Offline."));
		strcpy(room_list[0].name,M_GetText("Offline"));
		return -1;
	}
}

#ifdef UPDATE_ALERT
const char *GetMODVersion(void)
{
	static msg_t msg;

	// we must be connected to the master server before writing to it
	SOCKET_TYPE fd = MS_Connect(GetMasterServerIP(), GetMasterServerPort(), 0);
	if (fd == ERRSOCKET)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot connect to the Master Server\n"));
		M_StartMessage(M_GetText("There was a problem connecting to\nthe Master Server\n"), NULL, MM_NOTHING);
		return NULL;
	}

	msg.type = GET_VERSION_MSG;
	msg.room = MODID; // Might as well use it for something.
	msg.length = sprintf(msg.buffer,"%d",MODVERSION);
	if (MS_Write(fd, &msg) < 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Could not send to the Master Server\n"));
		M_StartMessage(M_GetText("Could not send to the Master Server\n"), NULL, MM_NOTHING);
		CloseConnection(fd);
		return NULL;
	}

	if (MS_Read(fd, &msg) < 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("No reply from the Master Server\n"));
		M_StartMessage(M_GetText("No reply from the Master Server\n"), NULL, MM_NOTHING);
		CloseConnection(fd);
		return NULL;
	}

	CloseConnection(fd);

	if(strcmp(msg.buffer,"NULL") != 0)
	{
		return msg.buffer;
	}
	else
		return NULL;
}

// Console only version of the above (used before game init)
void GetMODVersion_Console(void)
{
	static msg_t msg;

	// we must be connected to the master server before writing to it
	SOCKET_TYPE fd = MS_Connect(GetMasterServerIP(), GetMasterServerPort(), 0);
	if (fd == ERRSOCKET)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot connect to the Master Server\n"));
		return;
	}

	msg.type = GET_VERSION_MSG;
	msg.room = MODID; // Might as well use it for something.
	msg.length = sprintf(msg.buffer,"%d",MODVERSION);
	if (MS_Write(fd, &msg) < 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Could not send to the Master Server\n"));
		CloseConnection(fd);
		return;
	}

	if (MS_Read(fd, &msg) < 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("No reply from the Master Server\n"));
		CloseConnection(fd);
		return;
	}

	CloseConnection(fd);

	if (strcmp(msg.buffer,"NULL") != 0)
		I_Error(UPDATE_ALERT_STRING_CONSOLE, VERSIONSTRING, msg.buffer);
}
#endif

#ifndef NONET
/** Gets a list of game servers. Called from console.
  */
static void Command_Listserv_f(void)
{
	SOCKET_TYPE fd;
	CONS_Printf(M_GetText("Retrieving server list...\n"));

	fd = MS_Connect(GetMasterServerIP(), GetMasterServerPort(), 0);
	if (fd == ERRSOCKET)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot connect to the Master Server\n"));
		return;
	}

	if (GetServersList(fd))
		CONS_Alert(CONS_ERROR, M_GetText("Cannot get server list\n"));

	CloseConnection(fd);
}
#endif

FUNCMATH static const char *int2str(INT32 n)
{
	INT32 i;
	static char res[16];

	res[15] = '\0';
	res[14] = (char)((char)(n%10)+'0');
	for (i = 13; (n /= 10); i--)
		res[i] = (char)((char)(n%10)+'0');

	return &res[i+1];
}

#ifndef NONET
static void ConnectionFailed(void)
{
	con_state = MSCS_FAILED;
	CONS_Alert(CONS_ERROR, M_GetText("Connection to Master Server failed\n"));
}
#endif

/** Tries to register the local game server on the master server.
  */
static void AddToMasterServer(void *userdata)
{
#ifdef NONET
	(void)userdata;
#else
	int *state = userdata != NULL ? &con6_state : &con_state;
	int i;
	socklen_t j;
	msg_t msg;
	msg_server_t *info = (msg_server_t *)msg.buffer;
	INT32 room = -1;
	UINT32 signature, tmp;
	const char *insname;

	SOCKET_TYPE fd = MS_Connect(GetMasterServerIP(), GetMasterServerPort(), userdata != NULL ? CONNECT_IPV6 : CONNECT_IPV4);
	if (fd == ERRSOCKET)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Master Server socket error #%u: %s\n"), errno, strerror(errno));
		ConnectionFailed();
		return;
	}

	// so, the socket is writable, but what does that mean, that the connection is
	// ok, or bad... let see that!
	j = (socklen_t)sizeof (i);
	getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&i, &j);
	if (i) // it was bad
	{
		CONS_Alert(CONS_ERROR, M_GetText("Master Server socket error #%u: %s\n"), errno, strerror(errno));
		CloseConnection(fd);
		ConnectionFailed();
		return;
	}

#ifdef PARANOIA
	if (ms_RoomId <= 0)
		I_Error("Attmepted to host in room \"All\"!\n");
#endif
	room = ms_RoomId;

	for(signature = 0, insname = cv_servername.string; *insname; signature += *insname++);
	tmp = (UINT32)(signature * (size_t)&MSLastPing);
	signature *= tmp;
	signature &= 0xAAAAAAAA;
	M_Memcpy(&info->header.signature, &signature, sizeof (UINT32));

	strcpy(info->ip, "");
	strcpy(info->port, int2str(current_port));
	strcpy(info->name, cv_servername.string);
	M_Memcpy(&info->room, & room, sizeof (INT32));
#ifndef DEVELOP
	strcpy(info->version, SRB2VERSION);
#else // Trunk build, send revision info
	strcpy(info->version, GetRevisionString());
#endif
	strcpy(registered_server.name, cv_servername.string);

	if(*state != MSCS_REGISTERED)
		msg.type = ADD_SERVER_MSG;
	else
		msg.type = PING_SERVER_MSG;

	msg.length = (UINT32)sizeof (msg_server_t);
	msg.room = 0;
	if (MS_Write(fd, &msg) < 0)
	{
		CloseConnection(fd);
		ConnectionFailed();
		return;
	}

	if(*state != MSCS_REGISTERED)
		CONS_Printf(M_GetText("Master Server update successful.\n"));

	*state = MSCS_REGISTERED;
	CloseConnection(fd);
#ifdef HAVE_IPV6
	// just pass something to it to indicate it's ipv6's turn to register
	if (userdata == NULL)
		AddToMasterServer(&msg);
#endif
#endif
}

static void RemoveFromMasterServer(void *userdata)
{
	msg_t msg;
	msg_server_t *info = (msg_server_t *)msg.buffer;

	SOCKET_TYPE fd = MS_Connect(registered_server.ip, registered_server.port, userdata != NULL ? CONNECT_IPV6 : CONNECT_IPV4);
	if (fd == ERRSOCKET)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot connect to the Master Server\n"));
		return;
	}

	strcpy(info->header.buffer, "");
	strcpy(info->ip, "");
	strcpy(info->port, int2str(current_port));
	strcpy(info->name, registered_server.name);
	strcpy(info->version, SRB2VERSION);

	msg.type = REMOVE_SERVER_MSG;
	msg.length = (UINT32)sizeof (msg_server_t);
	msg.room = 0;
	if (MS_Write(fd, &msg) < 0)
	{
		CONS_Alert(CONS_ERROR, M_GetText("Cannot remove this server from the Master Server\n"));
		return;
	}

	CloseConnection(fd);

#ifdef HAVE_IPV6
	// just pass something to it to indicate it's ipv6's turn to deregister
	if (userdata == NULL)
		RemoveFromMasterServer(&msg);
#endif
}

const char *GetMasterServerPort(void)
{
	const char *t = cv_masterserver.string;

	while ((*t != ':') && (*t != '\0'))
		t++;

	if (*t)
		return ++t;
	else
		return DEF_PORT;
}

/** Gets the IP address of the master server. Actually, it seems to just
  * return the hostname, instead; the lookup is done elsewhere.
  *
  * \return Hostname of the master server, without port number on the end.
  * \todo Rename function?
  */
const char *GetMasterServerIP(void)
{
	static char str_ip[64];
	char *t = str_ip;

	if (strstr(cv_masterserver.string, "srb2.ssntails.org:28910")
	 || strstr(cv_masterserver.string, "srb2.servegame.org:28910")
	 || strstr(cv_masterserver.string, "srb2.servegame.org:28900")
	 || strstr(cv_masterserver.string, "ms.srb2.org:28900")
	   )
	{
		// replace it with the current default one
		CV_Set(&cv_masterserver, cv_masterserver.defaultvalue);
	}

	strcpy(t, cv_masterserver.string);

	while ((*t != ':') && (*t != '\0'))
		t++;
	*t = '\0';

	return str_ip;
}

static inline void SendPingToMasterServer(void)
{
	time_t timestamp = time(NULL);

	// Here, have a simpler MS Ping... - Cue
	if(timestamp > (MSLastPing+(60*2)) && con_state != MSCS_NONE)
	{
#ifdef HAVE_THREADS
		I_SpawnThread("ms_register", AddToMasterServer, NULL);
#else
		AddToMasterServer(NULL);
#endif
		MSLastPing = timestamp;
	}
}

void RegisterServer(void)
{
	CONS_Printf(M_GetText("Registering this server on the Master Server...\n"));

	strcpy(registered_server.ip, GetMasterServerIP());
	strcpy(registered_server.port, GetMasterServerPort());

#ifdef HAVE_THREADS
	I_SpawnThread("ms_register", AddToMasterServer, NULL);
#else
	AddToMasterServer(NULL);
#endif
}

void UnregisterServer(void)
{
	CONS_Printf(M_GetText("Removing this server from the Master Server...\n"));
	if (con_state == MSCS_REGISTERED)
	{
#ifdef HAVE_THREADS
		I_SpawnThread("ms_unregister", RemoveFromMasterServer, NULL);
#else
		RemoveFromMasterServer(NULL);
#endif
	}
	con_state = MSCS_NONE;
	con6_state = MSCS_NONE;

	MSLastPing = 0;
}

void MasterClient_Ticker(void)
{
	if (server && ms_RoomId > 0)
		SendPingToMasterServer();
}

static void ServerName_OnChange(void)
{
	if (con_state == MSCS_REGISTERED)
#ifdef HAVE_THREADS
		I_SpawnThread("ms_register", AddToMasterServer, NULL);
#else
		AddToMasterServer(NULL);
#endif
}

static void MasterServer_OnChange(void)
{
	UnregisterServer();
	RegisterServer();
}

#ifndef NONET
// Like recv, but waits until we've got enough data to fill the buffer.
static size_t recvfull(SOCKET_TYPE s, char *buf, size_t len, int flags)
{
	/* Total received. */
	size_t totallen = 0;

	while(totallen < len)
	{
		ssize_t ret = (ssize_t)recv(s, buf + totallen, (int)(len - totallen), flags);

		/* Error. */
		if(ret == -1)
			return (size_t)-1;

		totallen += ret;
	}

	return totallen;
}
#endif
