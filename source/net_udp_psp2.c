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
// net_udp.c

#include "quakedef.h"
#include "net_udp.h"

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

#define MAX_NAME 512
struct hostent{
  char  *h_name;         /* official (cannonical) name of host               */
  char **h_aliases;      /* pointer to array of pointers of alias names      */
  int    h_addrtype;     /* host address type: AF_INET                       */
  int    h_length;       /* length of address: 4                             */
  char **h_addr_list;    /* pointer to array of pointers with IPv4 addresses */
};
#define h_addr h_addr_list[0]

// Since using standard sceNet structs seems to cause problems, let's re-implement linux-like interface
#define AF_INET SCE_NET_AF_INET
#define SOCK_DGRAM SCE_NET_SOCK_DGRAM
#define IPPROTO_UDP SCE_NET_IPPROTO_UDP
#define SOL_SOCKET SCE_NET_SOL_SOCKET
#define MSG_PEEK SCE_NET_MSG_PEEK
#define INADDR_BROADCAST SCE_NET_INADDR_BROADCAST
#define SO_BROADCAST SCE_NET_SO_BROADCAST
#define INADDR_ANY SCE_NET_INADDR_ANY
#define SO_NBIO SCE_NET_SO_NBIO

struct in_addr {
    unsigned long s_addr;  // load with inet_aton()
};

struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
};

struct sockaddr {
    unsigned short    sa_family;    // address family, AF_xxx
    char              sa_data[14];  // 14 bytes of protocol address
};

inline int convertSceNetSockaddrIn(struct SceNetSockaddrIn* src, struct sockaddr_in* dst){
	if (dst == NULL || src == NULL) return -1;
	dst->sin_family = src->sin_family;
	dst->sin_port = src->sin_port;
	dst->sin_addr.s_addr = src->sin_addr.s_addr;
	return 0;
}

inline int convertSockaddrIn(struct SceNetSockaddrIn* dst, const struct sockaddr_in* src){
	if (dst == NULL || src == NULL) return -1;
	dst->sin_family = src->sin_family;
	dst->sin_port = src->sin_port;
	dst->sin_addr.s_addr = src->sin_addr.s_addr;
	return 0;
}

inline int convertSceNetSockaddr(struct SceNetSockaddr* src, struct sockaddr* dst){
	if (dst == NULL || src == NULL) return -1;
	dst->sa_family = src->sa_family;
	memcpy(dst->sa_data,src->sa_data,14);
	return 0;
}

inline int convertSockaddr(struct SceNetSockaddr* dst, const struct sockaddr* src){
	if (dst == NULL || src == NULL) return -1;
	dst->sa_family = src->sa_family;
	memcpy(dst->sa_data,src->sa_data,14);
	return 0;
}

inline int socket(int domain, int type, int protocol){
	return sceNetSocket("Socket", domain, type, protocol);
}

inline int recvfrom(int sockfd, void* buf, long len, int flags, struct sockaddr* src_addr, unsigned int* addrlen){
	struct SceNetSockaddr tmp;
	int res = sceNetRecvfrom(sockfd, buf, len, flags, &tmp, addrlen);
	if (src_addr != NULL) convertSceNetSockaddr(&tmp, src_addr);
	return res;
}

inline int getsockname(int sockfd, struct sockaddr *addr, unsigned int *addrlen){
	struct SceNetSockaddr tmp;
	convertSockaddr(&tmp, addr);
	int res = sceNetGetsockname(sockfd, &tmp, addrlen);
	convertSceNetSockaddr(&tmp, addr);
	return res;
}

inline int bind(int sockfd, const struct sockaddr* addr, unsigned int addrlen){
	struct SceNetSockaddr tmp;
	convertSockaddr(&tmp, addr);
	return sceNetBind(sockfd, &tmp, addrlen);
}

inline int close(int sockfd){
	return sceNetSocketClose(sockfd);
}

inline int setsockopt(int sockfd, int level, int optname, const void *optval, unsigned int optlen){
	return sceNetSetsockopt(sockfd, level, optname, optval, optlen);
}

inline unsigned int sendto(int sockfd, const void *buf, unsigned int len, int flags, const struct sockaddr *dest_addr, unsigned int addrlen){
	struct SceNetSockaddr tmp;
	convertSockaddr(&tmp, dest_addr);
	return sceNetSendto(sockfd, buf, len, flags, &tmp, addrlen);
}

// Copy-pasted from xyz code
static struct hostent *gethostbyname(const char *name)
{
    static struct hostent ent;
    static char sname[MAX_NAME] = "";
    static struct SceNetInAddr saddr = { 0 };
    static char *addrlist[2] = { (char *) &saddr, NULL };

    int rid;
    int err;
    rid = sceNetResolverCreate("resolver", NULL, 0);
    if(rid < 0) {
        return NULL;
    }

    err = sceNetResolverStartNtoa(rid, name, &saddr, 0, 0, 0);
    sceNetResolverDestroy(rid);
    if(err < 0) {
        return NULL;
    }

    ent.h_name = sname;
    ent.h_aliases = 0;
    ent.h_addrtype = SCE_NET_AF_INET;
    ent.h_length = sizeof(struct SceNetInAddr);
    ent.h_addr_list = addrlist;
    ent.h_addr = addrlist[0];

    return &ent;
}

static int net_acceptsocket = -1;		// socket for fielding new connections
static int net_controlsocket;
static int net_broadcastsocket = 0;
static struct qsockaddr broadcastaddr;

static unsigned long myAddr;

#include "net_udp.h"

//=============================================================================
static void *net_memory = NULL;
#define NET_INIT_SIZE 1*1024*1024

int UDP_Init (void)
{
	Log("UDP_Init called...");
	struct hostent *local;
	char	buff[15];
	struct qsockaddr addr;
	char *colon;
	SceNetInitParam initparam;
	
	if (COM_CheckParm ("-noudp"))
		return -1;
	
	// Start SceNet & SceNetCtl
	int ret = sceNetShowNetstat();
	if (ret == SCE_NET_ERROR_ENOTINIT) {
		net_memory = malloc(NET_INIT_SIZE);

		initparam.memory = net_memory;
		initparam.size = NET_INIT_SIZE;
		initparam.flags = 0;

		ret = sceNetInit(&initparam);
		if (ret < 0) return -1;
		
	}	
	ret = sceNetCtlInit();
	if (ret < 0){
		sceNetTerm();
		free(net_memory);
		return -1;
	}
	
	// Getting IP address
	SceNetCtlInfo info;
	sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
	sceNetInetPton(SCE_NET_AF_INET, info.ip_address, &myAddr);
	
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

	if ((net_controlsocket = UDP_OpenSocket (0)) == -1)
		Sys_Error("UDP_Init: Unable to open control socket\n");

	((struct sockaddr_in *)&broadcastaddr)->sin_family = AF_INET;
	((struct sockaddr_in *)&broadcastaddr)->sin_addr.s_addr = INADDR_BROADCAST;
	((struct sockaddr_in *)&broadcastaddr)->sin_port = sceNetHtons(net_hostport);

	UDP_GetSocketAddr (net_controlsocket, &addr);
	Q_strcpy(my_tcpip_address,  UDP_AddrToString (&addr));

	colon = Q_strrchr (my_tcpip_address, ':');
	if (colon)
		*colon = 0;
	
	Con_Printf("UDP Initialized as IP %s\n", my_tcpip_address);
	tcpipAvailable = true;

	return net_controlsocket;
}

//=============================================================================

void UDP_Shutdown (void)
{
	Log("UDP_Shutdown");
	UDP_Listen (false);
	UDP_CloseSocket (net_controlsocket);
	sceNetCtlTerm();
	sceNetTerm();
	free(net_memory);
}

//=============================================================================

void UDP_Listen (bool state)
{
	Log("UDP_Listen");
	// enable listening
	if (state)
	{
		if (net_acceptsocket != -1)
			return;
		if ((net_acceptsocket = UDP_OpenSocket (net_hostport)) == -1)
			Sys_Error ("UDP_Listen: Unable to open accept socket\n");
		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
		return;
	UDP_CloseSocket (net_acceptsocket);
	net_acceptsocket = -1;
}

//=============================================================================

int UDP_OpenSocket (int port)
{
	Log("UDP_OpenSocket(%ld)",port);
	int newsocket;
	struct sockaddr_in address;
	uint32_t _true = true;

	if ((newsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		return -1;

	if (setsockopt(newsocket, SOL_SOCKET, SO_NBIO, (char *)&_true, sizeof(uint32_t)) == -1)
		goto ErrorReturn;

	memset(&address, 0, sizeof(struct sockaddr_in)); // JPG 1.05 - fix by JDC
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = myAddr;
	address.sin_port = sceNetHtons(port);
	if( bind(newsocket, (void*)&address, sizeof(address)) == -1)
		goto ErrorReturn;

	return newsocket;

ErrorReturn:
	close(newsocket);
	return -1;
}

//=============================================================================

int UDP_CloseSocket (int socket)
{
	Log("UDP_CloseSocket");
	if (socket == net_broadcastsocket)
		net_broadcastsocket = 0;
	return close(socket);
}


//=============================================================================
/*
============
PartialIPAddress

this lets you type only as much of the net address as required, using
the local network components to fill in the rest
============
*/
static int PartialIPAddress (char *in, struct qsockaddr *hostaddr)
{
	char buff[256];
	char *b;
	int addr;
	int num;
	int mask;
	int run;
	int port;

	buff[0] = '.';
	b = buff;
	strcpy(buff+1, in);
	if (buff[1] == '.')
		b++;

	addr = 0;
	mask=-1;
	while (*b == '.')
	{
		b++;
		num = 0;
		run = 0;
		while (!( *b < '0' || *b > '9'))
		{
		  num = num*10 + *b++ - '0';
		  if (++run > 3)
		  	return -1;
		}
		if ((*b < '0' || *b > '9') && *b != '.' && *b != ':' && *b != 0)
			return -1;
		if (num < 0 || num > 255)
			return -1;
		mask<<=8;
		addr = (addr<<8) + num;
	}

	if (*b++ == ':')
		port = Q_atoi(b);
	else
		port = net_hostport;
	
	Log("PartialIPAddress(%s): port: %ld",in,port);
	hostaddr->sa_family = SCE_NET_AF_INET;
	((struct sockaddr_in *)hostaddr)->sin_port = sceNetHtons((short)port);
	((struct sockaddr_in *)hostaddr)->sin_addr.s_addr = (myAddr & sceNetHtonl(mask)) | sceNetHtonl(addr);

	return 0;
}
//=============================================================================

int UDP_Connect (int socket, struct qsockaddr *addr)
{
	Log("UDP_Connect");
	return 0;
}

//=============================================================================

int UDP_CheckNewConnections (void)
{
	Log("UDP_CheckNewConnections");
	char buf[4096];
	
	if (net_acceptsocket == -1)
		return -1;

	if (recvfrom(net_acceptsocket, buf, sizeof(buf), MSG_PEEK, NULL, NULL) >= 0)
		return net_acceptsocket;

	return -1;
}

//=============================================================================

int UDP_Read (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	int addrlen = sizeof (struct qsockaddr);
	int ret;
	
	ret = recvfrom(socket, buf, len, 0, (struct sockaddr *)addr, &addrlen);
	Log("UDP_Read returned %ld",ret);
	if (ret == SCE_NET_ERROR_EAGAIN || ret == SCE_NET_ERROR_ECONNREFUSED)
		return 0;
	else if (ret < 0)
		return -1;
	return ret;
}

//=============================================================================

int UDP_MakeSocketBroadcastCapable (int socket)
{
	int				i = 1;
	Log("UDP_MakeSocketBroadcastCapable");
	// make this socket broadcast capable
	if (setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) < 0)
		return -1;
	net_broadcastsocket = socket;

	return 0;
}

//=============================================================================

int UDP_Broadcast (int socket, byte *buf, int len)
{
	Log("UDP_Broadcast");
	int ret;

	if (socket != net_broadcastsocket)
	{
		if (net_broadcastsocket != 0)
			Sys_Error("Attempted to use multiple broadcasts sockets\n");
		ret = UDP_MakeSocketBroadcastCapable (socket);
		if (ret == -1)
		{
			Con_Printf("Unable to make socket broadcast capable\n");
			return ret;
		}
	}

	return UDP_Write (socket, buf, len, &broadcastaddr);
}

//=============================================================================

int UDP_Write (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	int ret;

	ret = sendto(socket, buf, len, 0, (struct sockaddr *)addr, sizeof(struct qsockaddr));
	Log("UDP_Write returned %ld",ret);
	if (ret == SCE_NET_ERROR_EAGAIN)
		return 0;
	else if (ret < 0)
		return -1;
	return ret;
}

//=============================================================================

char *UDP_AddrToString (struct qsockaddr *addr)
{
	static char buffer[22];
	int haddr;

	haddr = sceNetNtohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);
	sprintf(buffer, "%d.%d.%d.%d:%d", (haddr >> 24) & 0xff, (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff, sceNetNtohs(((struct sockaddr_in *)addr)->sin_port));
	Log("UDP_AddrToString returned %s",buffer);
	return buffer;
}

//=============================================================================

int UDP_StringToAddr (char *string, struct qsockaddr *addr)
{
	Log("UDP_StringToAddr(%s)",string);
	int ha1, ha2, ha3, ha4, hp;
	int ipaddr;

	sscanf(string, "%d.%d.%d.%d:%d", &ha1, &ha2, &ha3, &ha4, &hp);
	ipaddr = (ha1 << 24) | (ha2 << 16) | (ha3 << 8) | ha4;

	addr->sa_family = AF_INET;
	((struct sockaddr_in *)addr)->sin_addr.s_addr = sceNetHtonl(ipaddr);
	((struct sockaddr_in *)addr)->sin_port = sceNetHtons(hp);
	return 0;
}

//=============================================================================

int UDP_GetSocketAddr (int socket, struct qsockaddr *addr)
{
	Log("UDP_GetSocketAddr");
	int addrlen = sizeof(struct qsockaddr);
	unsigned int a, tmp;

	Q_memset(addr, 0, sizeof(struct qsockaddr));
	int ret = getsockname(socket, (struct sockaddr *)addr, &addrlen);
	Log("getsockname returned %ld", ret);
	a = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
	sceNetInetPton(SCE_NET_AF_INET, "127.0.0.1", &tmp);
	if (a == 0 || a == tmp)
		((struct sockaddr_in *)addr)->sin_addr.s_addr = myAddr;

	return 0;
}

//=============================================================================

int UDP_GetNameFromAddr (struct qsockaddr *addr, char *name)
{
	Q_strcpy (name, UDP_AddrToString (addr));
	return 0;
}

//=============================================================================

int UDP_GetAddrFromName(char *name, struct qsockaddr *addr)
{
	Log("UDP_GetAddrFromName(%s)",name);
	struct hostent *hostentry;

	if (name[0] >= '0' && name[0] <= '9')
		return PartialIPAddress (name, addr);

	hostentry = gethostbyname (name);
	if (!hostentry)
		return -1;

	addr->sa_family = SCE_NET_AF_INET;
	((struct sockaddr_in *)addr)->sin_port = sceNetHtons(net_hostport);
	((struct sockaddr_in *)addr)->sin_addr.s_addr = *(int *)hostentry->h_addr_list[0];

	return 0;
}

//=============================================================================

int UDP_AddrCompare (struct qsockaddr *addr1, struct qsockaddr *addr2)
{
	if (addr1->sa_family != addr2->sa_family){
		Log("UDP_AddrCompare returned -1 (1st case)");
		return -1;
	}
	
	if (((struct sockaddr_in *)addr1)->sin_addr.s_addr != ((struct sockaddr_in *)addr2)->sin_addr.s_addr){
		Log("UDP_AddrCompare returned -1 (2nd case)");
		return -1;
	}
	
	if (((struct sockaddr_in *)addr1)->sin_port != ((struct sockaddr_in *)addr2)->sin_port){
		Log("UDP_AddrCompare returned 1");
		return 1;
	}
	
	Log("UDP_AddrCompare returned 0");
	return 0;
}

//=============================================================================

int UDP_GetSocketPort (struct qsockaddr *addr)
{
	Log("UDP_GetSocketPort");
	return sceNetNtohs(((struct sockaddr_in *)addr)->sin_port);
}


int UDP_SetSocketPort (struct qsockaddr *addr, int port)
{
	Log("UDP_SetSocketPort");
	((struct sockaddr_in *)addr)->sin_port = sceNetHtons(port);
	return 0;
}

//=============================================================================
