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
// net_adhoc.h

int  AdHoc_Init (void);
void AdHoc_Shutdown (void);
void AdHoc_Listen (bool state);
int  AdHoc_OpenSocket (int port);
int  AdHoc_CloseSocket (int socket);
int  AdHoc_Connect (int socket, struct qsockaddr *addr);
int  AdHoc_CheckNewConnections (void);
int  AdHoc_Read (int socket, byte *buf, int len, struct qsockaddr *addr);
int  AdHoc_Write (int socket, byte *buf, int len, struct qsockaddr *addr);
int  AdHoc_Broadcast (int socket, byte *buf, int len);
char *AdHoc_AddrToString (struct qsockaddr *addr);
int  AdHoc_StringToAddr (char *string, struct qsockaddr *addr);
int  AdHoc_GetSocketAddr (int socket, struct qsockaddr *addr);
int  AdHoc_GetNameFromAddr (struct qsockaddr *addr, char *name);
int  AdHoc_GetAddrFromName (char *name, struct qsockaddr *addr);
int  AdHoc_AddrCompare (struct qsockaddr *addr1, struct qsockaddr *addr2);
int  AdHoc_GetSocketPort (struct qsockaddr *addr);
int  AdHoc_SetSocketPort (struct qsockaddr *addr, int port);
