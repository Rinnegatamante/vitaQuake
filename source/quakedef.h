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
// quakedef.h -- primary header for client

//#define	GLTEST			// experimental stuff

#define m_none 0
#define m_main 1
#define m_singleplayer 2
#define m_load 3
#define m_save 4
#define m_multiplayer 5
#define m_setup 6
#define m_net 7
#define m_options 8
#define m_video 9
#define m_keys 10
#define m_help 11
#define m_quit 12
#define m_lanconfig 13
#define m_gameoptions 14
#define m_search 15
#define m_slist 16
#define m_onlineserverlist 17
#define m_benchmark 18
#define m_mods 19

extern int m_state;

#define	QUAKE_GAME			// as opposed to utilities
//#define	DEBUG

#define ENGINE_NAME			"vitaQuake"
#define	VERSION				3.00
#define VERSION_PROQUAKE	3.50
#define	GLQUAKE_VERSION	    1.00

//define	PARANOID			// speed sapping error checking

#define	GAMENAME_DIR	"id1"

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "neon_mathfun.h"

#if defined(_WIN32) && !defined(WINDED)

#if defined(_M_IX86)
#define __i386__	1
#endif

void	VID_LockBuffer (void);
void	VID_UnlockBuffer (void);

#else

#define	VID_LockBuffer()
#define	VID_UnlockBuffer()

#endif

#if defined __i386__ // && !defined __sun__
#define id386	1
#else
#define id386	0
#endif

#if id386
#define UNALIGNED_OK	1	// set to 0 if unaligned accesses are not supported
#else
#define UNALIGNED_OK	0
#endif

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE	32		// used to align key data structures

#define	MINIMUM_MEMORY			0x550000
#define	MINIMUM_MEMORY_LEVELPAK	(MINIMUM_MEMORY + 0x100000)

#define MAX_NUM_ARGVS	50

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2


#define	MAX_QPATH		64			// max length of a quake game pathname
#define	MAX_OSPATH		128			// max length of a filesystem pathname

#define	ON_EPSILON		0.1			// point on plane side epsilon

#define	MAX_MSGLEN		8000		// max length of a reliable message
#define	MAX_DATAGRAM	1024		// max length of unreliable message

//
// per-level limits
//
#define	MAX_EDICTS		600			// FIXME: ouch! ouch! ouch!
#define	MAX_LIGHTSTYLES	64
#define	MAX_MODELS		256			// these are sent over the net as bytes
#define	MAX_SOUNDS		256			// so they cannot be blindly increased

#define	SAVEGAME_COMMENT_LENGTH	39

#define	MAX_STYLESTRING	64

#define BIT(x) (1<<x)	// Ch0wW: To ease things slightly

//
// stats are integers communicated to the client by the server
//
#define	MAX_CL_STATS		32
#define	STAT_HEALTH			0
#define	STAT_FRAGS			1
#define	STAT_WEAPON			2
#define	STAT_AMMO			3
#define	STAT_ARMOR			4
#define	STAT_WEAPONFRAME	5
#define	STAT_SHELLS			6
#define	STAT_NAILS			7
#define	STAT_ROCKETS		8
#define	STAT_CELLS			9
#define	STAT_ACTIVEWEAPON	10
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		// bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		// bumped by svc_killedmonster

// stock defines

#define	IT_SHOTGUN				BIT(0)
#define	IT_SUPER_SHOTGUN		BIT(1)
#define	IT_NAILGUN				BIT(2)
#define	IT_SUPER_NAILGUN		BIT(3)
#define	IT_GRENADE_LAUNCHER		BIT(4)
#define	IT_ROCKET_LAUNCHER		BIT(5)
#define	IT_LIGHTNING			BIT(6)
#define IT_SUPER_LIGHTNING      BIT(7)
#define IT_SHELLS               BIT(8)
#define IT_NAILS                BIT(9)
#define IT_ROCKETS              BIT(10)
#define IT_CELLS                BIT(11)
#define IT_AXE                  BIT(12)
#define IT_ARMOR1               BIT(13)
#define IT_ARMOR2               BIT(14)
#define IT_ARMOR3               BIT(15)
#define IT_SUPERHEALTH          BIT(16)
#define IT_KEY1                 BIT(17)
#define IT_KEY2                 BIT(18)
#define	IT_INVISIBILITY			BIT(19)
#define	IT_INVULNERABILITY		BIT(20)
#define	IT_SUIT					BIT(21)
#define	IT_QUAD					BIT(22)
// [......]
#define IT_SIGIL1               BIT(27)
#define IT_SIGIL2               BIT(28)
#define IT_SIGIL3               BIT(29)
#define IT_SIGIL4               BIT(30)

//===========================================
//rogue changed and added defines

#define RIT_SHELLS              128
#define RIT_NAILS               256
#define RIT_ROCKETS             512
#define RIT_CELLS               1024
#define RIT_AXE                 2048
#define RIT_LAVA_NAILGUN        4096
#define RIT_LAVA_SUPER_NAILGUN  8192
#define RIT_MULTI_GRENADE       16384
#define RIT_MULTI_ROCKET        32768
#define RIT_PLASMA_GUN          65536
#define RIT_ARMOR1              8388608
#define RIT_ARMOR2              16777216
#define RIT_ARMOR3              33554432
#define RIT_LAVA_NAILS          67108864
#define RIT_PLASMA_AMMO         134217728
#define RIT_MULTI_ROCKETS       268435456
#define RIT_SHIELD              536870912
#define RIT_ANTIGRAV            1073741824
#define RIT_SUPERHEALTH         2147483648

//MED 01/04/97 added hipnotic defines
//===========================================
//hipnotic added defines
#define HIT_PROXIMITY_GUN_BIT 16
#define HIT_MJOLNIR_BIT       7
#define HIT_LASER_CANNON_BIT  23
#define HIT_PROXIMITY_GUN   (1<<HIT_PROXIMITY_GUN_BIT)
#define HIT_MJOLNIR         (1<<HIT_MJOLNIR_BIT)
#define HIT_LASER_CANNON    (1<<HIT_LASER_CANNON_BIT)
#define HIT_WETSUIT         (1<<(23+2))
#define HIT_EMPATHY_SHIELDS (1<<(23+3))

//===========================================

#define	MAX_SCOREBOARD		16
#define	MAX_SCOREBOARDNAME	32

#define	SOUND_CHANNELS		8

// This makes anyone on id's net privileged
// Use for multiplayer testing only - VERY dangerous!!!
// #define IDGODS

#include "common.h"
#include "bspfile.h"
#include "vid.h"
#include "sys.h"
#include "zone.h"
#include "mathlib.h"

typedef struct
{
	vec3_t	origin;
	vec3_t	angles;
	int		modelindex;
	int		frame;
	int		colormap;
	int		skin;
	int		effects;
} entity_state_t;


#include "wad.h"
#include "draw.h"
#include "cvar.h"
#include "screen.h"
#include "net.h"
#include "protocol.h"
#include "cmd.h"
#include "sbar.h"
#include "sound.h"
#include "render.h"
#include "client.h"
#include "progs.h"
#include "server.h"

#ifdef GLQUAKE
#include "gl_model.h"
#else
#include "model.h"
#include "d_iface.h"
#endif

#include "input.h"
#include "world.h"
#include "keys.h"
#include "console.h"
#include "view.h"
#include "menu.h"
#include "crc.h"
#include "cdaudio.h"

#ifdef GLQUAKE
#include "glquake.h"
#endif

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	char	*basedir;
	char	*cachedir;		// for development over ISDN lines
	int		argc;
	char	**argv;
	void	*membase;
	int		memsize;
} quakeparms_t;


//=============================================================================



extern bool noclip_anglehack;


//
// host
//
extern	quakeparms_t host_parms;

extern	cvar_t		sys_ticrate;
extern	cvar_t		sys_nostdout;
extern	cvar_t		developer;

extern	bool	host_initialized;		// true if into command execution
extern	double		host_frametime;
extern	byte		*host_basepal;
extern	byte		*host_colormap;
extern	int			host_framecount;	// incremented every frame, never reset
extern	double		realtime;			// not bounded in any way, changed at
										// start of every frame, never reset

void Host_ClearMemory (void);
void Host_ServerFrame (void);
void Host_InitCommands (void);
void Host_Init (quakeparms_t *parms);
void Host_Shutdown(void);
void Host_Error (char *error, ...);
void Host_EndGame (char *message, ...);
void Host_Frame (float time);
void Host_Quit_f (void);
void Host_ClientCommands (char *fmt, ...);
void Host_ShutdownServer (bool crash);

extern bool		msg_suppress_1;		// suppresses resolution and cache size console output
										//  an fullscreen DIB focus gain/loss
extern int			current_skill;		// skill level for currently loaded level (in case
										//  the user changes the cvar while the level is
										//  running, this reflects the level actually in use)

extern bool		isDedicated;

extern int			minimum_memory;

//
// chase
//
extern	cvar_t	chase_active;

void Chase_Init (void);
void Chase_Reset (void);
void Chase_Update (void);

void DrawQuad_NoTex(GLfloat x, GLfloat y, GLfloat w, GLfloat h);
void DrawQuad(GLfloat x, GLfloat y, GLfloat w, GLfloat h, GLfloat u, GLfloat v, GLfloat uw, GLfloat vh);

void GL_SubdivideSurface (msurface_t *fa);
void GL_MakeAliasModelDisplayLists (model_t *m, aliashdr_t *hdr);
int R_LightPoint (vec3_t p);
void R_DrawBrushModel (entity_t *e);
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees );
void R_AnimateLight (void);
void V_CalcBlend (void);
void R_DrawWorld (void);
void R_RenderDlights(void);
void R_DrawParticles (void);
void R_DrawWaterSurfaces (void);
void R_RenderBrushPoly (msurface_t *fa);
void R_InitParticles (void);
void GL_Upload8_EXT (byte *data, int width, int height,  bool mipmap, bool alpha);
void R_ClearParticles (void);
void GL_BuildLightmaps (void);
void EmitWaterPolys (msurface_t *fa);
void EmitSkyPolys (msurface_t *fa);
void EmitBothSkyLayers (msurface_t *fa);
void R_DrawSkyChain (msurface_t *s);
bool R_CullBox (vec3_t mins, vec3_t maxs);
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);
void R_RotateForEntity (entity_t *e);
void R_StoreEfrags (efrag_t **ppefrag);
void GL_Set2D (void);
void GL_DrawFPS(void);
void GL_DrawBenchmark(void);
void GL_SelectTexture (GLenum target);

#define VERTEXARRAYSIZE 18360
extern float gVertexBuffer[VERTEXARRAYSIZE];
extern float gColorBuffer[VERTEXARRAYSIZE];
extern float gTexCoordBuffer[VERTEXARRAYSIZE];

// Fragment shaders
#define MODULATE_WITH_COLOR  0
#define MODULATE             1
#define REPLACE              2
#define MONO_COLOR           4
#define MODULATE_COLOR_A     5
#define MODULATE_A           6
#define RGBA_A               7
#define REPLACE_A            8

// Vertex shaders
#define TEXTURE2D            0
#define TEXTURE2D_WITH_COLOR 1
#define COLOR                2
#define VERTEX_ONLY          3

// Shader programs
#define TEX2D_REPL      0
#define TEX2D_MODUL     1
#define TEX2D_MODUL_CLR 2
#define RGBA_COLOR      3
#define NO_COLOR        4
#define TEX2D_REPL_A    5
#define TEX2D_MODUL_A   6
#define RGBA_CLR_A      7
#define FULL_A          8

extern GLuint fs[9];
extern GLuint vs[4];
extern GLuint programs[9];
extern GLint monocolor;
extern GLint modulcolor[2];

void GL_EnableState(GLenum state);
void GL_DisableState(GLenum state);
void GL_DrawPolygon(GLenum prim, int num);
void GL_Color(float r, float g, float b, float a);
void GL_ResetShaders();

extern float sintablef[17];
extern float costablef[17];

// Mods support
typedef struct ModsList{
	char name[256];
	struct ModsList* next;
}ModsList;
