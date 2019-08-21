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
// protocol.h -- communications protocols

/*
================
Here is a short list of known protocol used by other NetQuake ports:
- 15: Default NetQuake protocol.
- 666: FitzQuake protocol (also used by QuakeSpasm)
- 35, 36, 37, 38: ProQuake protocol
================
*/
#define	PROTOCOL_NETQUAKE	15	// Official NetQuake protocol used.
#define PROTOCOL_FITZQUAKE	666 // Protocol for FitzQuake 0.85

// if the high bit of the servercmd is set, the low bits are fast update flags:
#define	U_MOREBITS	BIT(0)
#define	U_ORIGIN1	BIT(1)
#define	U_ORIGIN2	BIT(2)
#define	U_ORIGIN3	BIT(3)
#define	U_ANGLE2	BIT(4)
#define	U_NOLERP	BIT(5)		// don't interpolate movement
#define	U_FRAME		BIT(6)
#define U_SIGNAL	BIT(7)		// just differentiates from other updates

// svc_update can pass all of the fast update bits, plus more
#define	U_ANGLE1	BIT(8)
#define	U_ANGLE3	BIT(9)
#define	U_MODEL		BIT(10)
#define	U_COLORMAP	BIT(11)
#define	U_SKIN		BIT(12)
#define	U_EFFECTS	BIT(13)
#define	U_LONGENTITY	BIT(14)

// FitzQuake -- Protocol extender
#define U_EXTEND1		BIT(15)
#define U_ALPHA			BIT(16) // 1 byte, uses ENTALPHA_ENCODE, not sent if equal to baseline
#define U_FRAME2		BIT(17) // 1 byte, this is .frame & 0xFF00 (second byte)
#define U_MODEL2		BIT(18) // 1 byte, this is .modelindex & 0xFF00 (second byte)
#define U_LERPFINISH	BIT(19) // 1 byte, 0.0-1.0 maps to 0-255, not sent if exactly 0.1, this is ent->v.nextthink - sv.time, used for lerping
#define U_SCALE			BIT(20) // 1 byte, for PROTOCOL_RMQ PRFL_EDICTSCALE, currently read but ignored
#define U_UNUSED21		BIT(21)
#define U_UNUSED22		BIT(22)
#define U_EXTEND2		BIT(23) // another byte to follow, future expansion

#define	SU_VIEWHEIGHT	BIT(0)
#define	SU_IDEALPITCH	BIT(1)
#define	SU_PUNCH1		BIT(2)
#define	SU_PUNCH2		BIT(3)
#define	SU_PUNCH3		BIT(4)
#define	SU_VELOCITY1	BIT(5)
#define	SU_VELOCITY2	BIT(6)
#define	SU_VELOCITY3	BIT(7)
//define	SU_AIMENT		BIT(8)  AVAILABLE BIT
#define	SU_ITEMS		BIT(9)
#define	SU_ONGROUND		BIT(10)		// no data follows, the bit is it
#define	SU_INWATER		BIT(11)		// no data follows, the bit is it
#define	SU_WEAPONFRAME	BIT(12)
#define	SU_ARMOR		BIT(13)
#define	SU_WEAPON		BIT(14)

// FitzQuake -- Protocol extender 
#define SU_EXTEND1		BIT(15) // another byte to follow
#define SU_WEAPON2		BIT(16) // 1 byte, this is .weaponmodel & 0xFF00 (second byte)
#define SU_ARMOR2		BIT(17) // 1 byte, this is .armorvalue & 0xFF00 (second byte)
#define SU_AMMO2		BIT(18) // 1 byte, this is .currentammo & 0xFF00 (second byte)
#define SU_SHELLS2		BIT(19) // 1 byte, this is .ammo_shells & 0xFF00 (second byte)
#define SU_NAILS2		BIT(20) // 1 byte, this is .ammo_nails & 0xFF00 (second byte)
#define SU_ROCKETS2		BIT(21) // 1 byte, this is .ammo_rockets & 0xFF00 (second byte)
#define SU_CELLS2		BIT(22) // 1 byte, this is .ammo_cells & 0xFF00 (second byte)
#define SU_EXTEND2		BIT(23) // another byte to follow
#define SU_WEAPONFRAME2	BIT(24) // 1 byte, this is .weaponframe & 0xFF00 (second byte)
#define SU_WEAPONALPHA	BIT(25) // 1 byte, this is alpha for weaponmodel, uses ENTALPHA_ENCODE, not sent if ENTALPHA_DEFAULT
#define SU_UNUSED26		BIT(26)
#define SU_UNUSED27		BIT(27)
#define SU_UNUSED28		BIT(28)
#define SU_UNUSED29		BIT(29)
#define SU_UNUSED30		BIT(30)
#define SU_EXTEND3		BIT(31) // another byte to follow, future expansion


// a sound with no channel is a local only sound
#define	SND_VOLUME		BIT(0)		// a byte
#define	SND_ATTENUATION	BIT(1)		// a byte
#define	SND_LOOPING		BIT(2)		// a long


// defaults for clientinfo messages
#define	DEFAULT_VIEWHEIGHT	22


// game types sent by serverinfo
// these determine which intermission screen plays
#define	GAME_COOP			0
#define	GAME_DEATHMATCH		1

//==================
// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse
//==================

//
// server to client
//
#define	svc_bad				0
#define	svc_nop				1
#define	svc_disconnect		2
#define	svc_updatestat		3	// [byte] [long]
#define	svc_version			4	// [long] server version
#define	svc_setview			5	// [short] entity number
#define	svc_sound			6	// <see code>
#define	svc_time			7	// [float] server time
#define	svc_print			8	// [string] null terminated string
#define	svc_stufftext		9	// [string] stuffed into client's console buffer
								// the string should be \n terminated
#define	svc_setangle		10	// [angle3] set the view angle to this absolute value
	
#define	svc_serverinfo		11	// [long] version
						// [string] signon string
						// [string]..[0]model cache
						// [string]...[0]sounds cache
#define	svc_lightstyle		12	// [byte] [string]
#define	svc_updatename		13	// [byte] [string]
#define	svc_updatefrags		14	// [byte] [short]
#define	svc_clientdata		15	// <shortbits + data>
#define	svc_stopsound		16	// <see code>
#define	svc_updatecolors	17	// [byte] [byte]
#define	svc_particle		18	// [vec3] <variable>
#define	svc_damage			19
	
#define	svc_spawnstatic		20
//	svc_spawnbinary		21
#define	svc_spawnbaseline	22
	
#define	svc_temp_entity		23

#define	svc_setpause		24	// [byte] on / off
#define	svc_signonnum		25	// [byte]  used for the signon sequence

#define	svc_centerprint		26	// [string] to put in center of the screen

#define	svc_killedmonster	27
#define	svc_foundsecret		28

#define	svc_spawnstaticsound	29	// [coord3] [byte] samp [byte] vol [byte] aten

#define	svc_intermission	30		// [string] music
#define	svc_finale			31		// [string] music [string] text

#define	svc_cdtrack			32		// [byte] track [byte] looptrack
#define svc_sellscreen		33

#define svc_cutscene		34

//
// client to server
//
#define	clc_bad			0
#define	clc_nop 		1
#define	clc_disconnect	2
#define	clc_move		3			// [usercmd_t]
#define	clc_stringcmd	4		// [string] message


//
// temp entity events
//
#define	TE_SPIKE			0
#define	TE_SUPERSPIKE		1
#define	TE_GUNSHOT			2
#define	TE_EXPLOSION		3
#define	TE_TAREXPLOSION		4
#define	TE_LIGHTNING1		5
#define	TE_LIGHTNING2		6
#define	TE_WIZSPIKE			7
#define	TE_KNIGHTSPIKE		8
#define	TE_LIGHTNING3		9
#define	TE_LAVASPLASH		10
#define	TE_TELEPORT			11
#define TE_EXPLOSION2		12

// PGM 01/21/97 
#define TE_BEAM				13
// PGM 01/21/97 

#ifdef QUAKE2
#define TE_IMPLOSION		14
#define TE_RAILTRAIL		15
#endif
