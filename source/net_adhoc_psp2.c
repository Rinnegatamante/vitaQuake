/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_adhoc.c

#include "quakedef.h"
#include "net_adhoc.h"

#include <sys/types.h>
#include <vitasdk.h>
#include <errno.h>

#ifdef __sun__
#include <sys/filio.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif

extern cvar_t hostname;
extern void Log (const char *format, ...);

#define ADHOC_NET 29

typedef struct sockaddr_adhoc
{
	char   len;
	short family;
	uint16_t port;
	char mac[6];
	char zero[6];
} sockaddr_adhoc;

static int net_acceptsocket = -1;		// socket for fielding new connections
static int net_controlsocket;
static int net_broadcastsocket = 0;
static struct qsockaddr broadcastaddr;

#include "net_adhoc.h"

//=============================================================================

int AdHoc_Init (void)
{
	Log("ADHoc_Init called...");
	struct qsockaddr addr;
	char *colon;
	
	// if the quake hostname isn't set, set it to player nickname
	if (!Q_strcmp(hostname.string, "UNNAMED"))
	{
		SceAppUtilInitParam init_param;
		SceAppUtilBootParam boot_param;
		memset(&init_param, 0, sizeof(SceAppUtilInitParam));
		memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
		sceAppUtilInit(&init_param, &boot_param);
		char nick[SCE_SYSTEM_PARAM_USERNAME_MAXSIZE];
		sceAppUtilSystemParamGetString(SCE_SYSTEM_PARAM_ID_USERNAME, nick, SCE_SYSTEM_PARAM_USERNAME_MAXSIZE);
		Cvar_Set ("hostname", nick);
	}

	if ((net_controlsocket = AdHoc_OpenSocket (0)) == -1)
		Sys_Error("AdHoc_Init: Unable to open control socket\n");

	((struct sockaddr_adhoc *)&broadcastaddr)->family = ADHOC_NET;
	int i;
	for(i=0; i<6; i++) ((struct sockaddr_adhoc *)&broadcastaddr)->mac[i] = 0xFF;
	((struct sockaddr_adhoc *)&broadcastaddr)->port = sceNetHtons(net_hostport);

	AdHoc_GetSocketAddr (net_controlsocket, &addr);
	Q_strcpy(my_tcpip_address,  AdHoc_AddrToString (&addr));

	colon = Q_strrchr (my_tcpip_address, ':');
	if (colon)
		*colon = 0;
	
	AdHoc_Listen(1);
	
	Con_Printf("AdHoc Initialized as IP %s\n", my_tcpip_address);
	tcpipAvailable = true;

	return net_controlsocket;
}

//=============================================================================

void AdHoc_Shutdown (void)
{
	Log("AdHoc_Shutdown");
	AdHoc_Listen (false);
	AdHoc_CloseSocket (net_controlsocket);
	sceNetAdhocctlTerm();
	sceNetAdhocTerm();
}

//=============================================================================

void AdHoc_Listen (bool state)
{
	Log("AdHoc_Listen");
	// enable listening
	if (state)
	{
		if (net_acceptsocket != -1)
			return;
		if ((net_acceptsocket = AdHoc_OpenSocket (net_hostport)) == -1)
			Sys_Error ("AdHoc_Listen: Unable to open accept socket\n");
		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
		return;
	AdHoc_CloseSocket (net_acceptsocket);
	net_acceptsocket = -1;
}

//=============================================================================

int AdHoc_OpenSocket (int port)
{
	Log("AdHoc_OpenSocket(%ld)", port);
	uint8_t mac[8];
	sceNetAdhocctlGetEtherAddr(&mac);
	int rc = sceNetAdhocPdpCreate(mac, port, 0x2000, 0);
	if(rc < 0) return -1;
	return rc;
}

//=============================================================================

int AdHoc_CloseSocket (int socket)
{
	Log("AdHoc_CloseSocket");
	if (socket == net_broadcastsocket)
		net_broadcastsocket = 0;
	return sceNetAdhocPdpDelete(socket, 0);
}

//=============================================================================

int AdHoc_Connect (int socket, struct qsockaddr *addr)
{
	Log("AdHoc_Connect");
	return 0;
}

//=============================================================================

static struct SceNetAdhocPdpStat gPdpStat;
struct SceNetAdhocPdpStat *findPdpStat(int socket, struct SceNetAdhocPdpStat *pdpStat)
{
	if(socket == pdpStat->id) {
		memcpy(&gPdpStat, pdpStat, sizeof(struct SceNetAdhocPdpStat));
		return &gPdpStat;
	}
		if(pdpStat->next) return findPdpStat(socket, pdpStat->next);
		return (struct SceNetAdhocPdpStat *)-1;
}

int AdHoc_CheckNewConnections (void)
{
	Log("AdHoc_CheckNewConnections");
	struct SceNetAdhocPdpStat pdpStat[20];
	int len = sizeof(struct SceNetAdhocPdpStat) * 20;
	
	if (net_acceptsocket == -1) return -1;
	
	int err = sceNetAdhocGetPdpStat(&len, pdpStat);
	if(err < 0) return -1;
	
	struct SceNetAdhocPdpStat *tempPdp = findPdpStat(net_acceptsocket, pdpStat);
	if (tempPdp < 0) return -1;
	else if (tempPdp->rcv_sb_cc > 0) return net_acceptsocket;

	return -1;
}

//=============================================================================

int AdHoc_Read (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	unsigned short port;
	int datalength = len;
	int ret;
	
	ret = sceNetAdhocPdpRecv(socket, (unsigned char *)((sockaddr_adhoc *)addr)->mac, &port, buf, &datalength, 0, 1);
	Log("AdHoc_Read returned %ld",ret);
	if (ret == SCE_ERROR_NET_ADHOC_WOULD_BLOCK)
		return 0;
	else if (ret < 0)
		return -1;
	
	((sockaddr_adhoc *)addr)->port = port;
	
	return datalength;
}

//=============================================================================

int AdHoc_MakeSocketBroadcastCapable (int socket)
{
	Log("AdHoc_MakeSocketBroadcastCapable");
	net_broadcastsocket = socket;

	return 0;
}

//=============================================================================

int AdHoc_Broadcast (int socket, byte *buf, int len)
{
	Log("AdHoc_Broadcast");
	int ret;

	if (socket != net_broadcastsocket)
	{
		if (net_broadcastsocket != 0)
			Sys_Error("Attempted to use multiple broadcasts sockets\n");
		ret = AdHoc_MakeSocketBroadcastCapable (socket);
		if (ret == -1)
		{
			Con_Printf("Unable to make socket broadcast capable\n");
			return ret;
		}
	}

	return AdHoc_Write (socket, buf, len, &broadcastaddr);
}

//=============================================================================

int AdHoc_Write (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	int ret;

	ret = sceNetAdhocPdpSend(socket, (unsigned char*)((sockaddr_adhoc *)addr)->mac, ((sockaddr_adhoc *)addr)->port, buf, len, 0, 1);
	Log("AdHoc_Write returned %ld",ret);
	if (ret == SCE_ERROR_NET_ADHOC_WOULD_BLOCK)
		return 0;
	else if (ret < 0)
		return -1;
	return ret;
}

//=============================================================================

char *AdHoc_AddrToString (struct qsockaddr *addr)
{
	static char buffer[22];
	int haddr;

	sceNetEtherNtostr((unsigned char *)((sockaddr_adhoc *)addr)->mac, buffer, 22);
	sprintf(buffer + strlen(buffer), ":%d", ((sockaddr_adhoc *)addr)->port);
	Log("AdHoc_AddrToString returned %s",buffer);
	return buffer;
}

//=============================================================================

int AdHoc_StringToAddr (char *string, struct qsockaddr *addr)
{
	Log("AdHoc_StringToAddr(%s)",string);
	int ha1, ha2, ha3, ha4, ha5, ha6, hp;

	int r = sscanf(string, "%x:%x:%x:%x:%x:%x:%d", &ha1, &ha2, &ha3, &ha4, &ha5, &ha6, &hp);
	if (r < 7) hp = net_hostport;
	addr->sa_family = ADHOC_NET;
	((struct sockaddr_adhoc *)addr)->mac[0] = ha1 & 0xFF;
	((struct sockaddr_adhoc *)addr)->mac[1] = ha2 & 0xFF;
	((struct sockaddr_adhoc *)addr)->mac[2] = ha3 & 0xFF;
	((struct sockaddr_adhoc *)addr)->mac[3] = ha4 & 0xFF;
	((struct sockaddr_adhoc *)addr)->mac[4] = ha5 & 0xFF;
	((struct sockaddr_adhoc *)addr)->mac[5] = ha6 & 0xFF;
	((struct sockaddr_adhoc *)addr)->port = hp & 0xFFFF;
	return 0;
}

//=============================================================================

int AdHoc_GetSocketAddr (int socket, struct qsockaddr *addr)
{
	Log("AdHoc_GetSocketAddr");
	struct SceNetAdhocPdpStat pdpStat[20];
	int len = sizeof(struct SceNetAdhocPdpStat) * 20;

	int err = sceNetAdhocGetPdpStat(&len, pdpStat);
	if(err<0) return -1;

	struct SceNetAdhocPdpStat *tempPdp = findPdpStat(socket, pdpStat);
	if(tempPdp < 0) return -1;

	memcpy(((struct sockaddr_adhoc *)addr)->mac, &tempPdp->laddr, 6);
	((struct sockaddr_adhoc *)addr)->port = tempPdp->lport;
	addr->sa_family = ADHOC_NET;
	return 0;
}

//=============================================================================

int AdHoc_GetNameFromAddr (struct qsockaddr *addr, char *name)
{
	Q_strcpy (name, AdHoc_AddrToString (addr));
	return 0;
}

//=============================================================================

int AdHoc_GetAddrFromName(char *name, struct qsockaddr *addr)
{
	Log("AdHoc_GetAddrFromName(%s)", name);
	return AdHoc_StringToAddr(name, addr);
}

//=============================================================================

int AdHoc_AddrCompare (struct qsockaddr *addr1, struct qsockaddr *addr2)
{
	if (memcmp(((struct sockaddr_adhoc *)addr1)->mac, ((struct sockaddr_adhoc *)addr2)->mac, 6) != 0) return -1;
	if (((struct sockaddr_adhoc *)addr1)->port != ((struct sockaddr_adhoc *)addr2)->port) return 1;
	
	Log("AdHoc_AddrCompare returned 0");
	return 0;
}

//=============================================================================

int AdHoc_GetSocketPort (struct qsockaddr *addr)
{
	Log("AdHoc_GetSocketPort");
	return sceNetNtohs(((struct sockaddr_adhoc *)addr)->port);
}


int AdHoc_SetSocketPort (struct qsockaddr *addr, int port)
{
	Log("AdHoc_SetSocketPort");
	((struct sockaddr_adhoc *)addr)->port = sceNetHtons(port);
	return 0;
}

//=============================================================================
