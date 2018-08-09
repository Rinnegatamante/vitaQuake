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
#include <stdarg.h>
#include <stdio.h>
#include <vitasdk.h>
#include <vitaGL.h>

#include "quakedef.h"

extern cvar_t vid_vsync;
extern bool benchmark;
unsigned short	d_8to16table[256];
unsigned	d_8to24table[256];
unsigned char d_15to8table[65536];
CVAR (show_fps, 0, CVAR_ARCHIVE)
CVAR (gl_fog, 0, CVAR_ARCHIVE)
extern int isKeyboard;

int num_shades=32;

int	d_con_indirect = 0;

int		svgalib_inited=0;
int		UseMouse = 1;
int		UseKeyboard = 1;

CVAR (vid_mode, 5, CVAR_NONE)
CVAR (vid_redrawfull, 0, CVAR_NONE)
CVAR (vid_waitforrefresh, 1, CVAR_ARCHIVE)
 
signed char *framebuffer_ptr;

int     mouse_buttons;
int     mouse_buttonstate;
int     mouse_oldbuttonstate;
float   mouse_x, mouse_y;
float	old_mouse_x, old_mouse_y;
int		mx, my;

int scr_width = 960, scr_height = 544;

/*-----------------------------------------------------------------------*/

//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
//int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

int		texture_extension_number = 1;

float		gldepthmin, gldepthmax;

CVAR (gl_ztrick, 0, CVAR_NONE) // Default now OFF. KH

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;

static float vid_gamma = 1.0;

bool isPermedia = false;

float sintablef[17] = {
	 0.000000f, 0.382683f, 0.707107f,
	 0.923879f, 1.000000f, 0.923879f,
	 0.707107f, 0.382683f, 0.000000f,
	-0.382683f,-0.707107f,-0.923879f,
	-1.000000f,-0.923879f,-0.707107f,
	-0.382683f, 0.000000f
};
	
float costablef[17] = {
	 1.000000f, 0.923879f, 0.707107f,
	 0.382683f, 0.000000f,-0.382683f,
	-0.707107f,-0.923879f,-1.000000f,
	-0.923879f,-0.707107f,-0.382683f,
	 0.000000f, 0.382683f, 0.707107f,
	 0.923879f, 1.000000f
};

/*-----------------------------------------------------------------------*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect (int x, int y, int width, int height)
{
}

void VID_Shutdown(void)
{
}

void VID_ShiftPalette(unsigned char *p)
{
//	VID_SetPalette(p);
}

void VID_ChangeRes(float scale){
	
}

void	VID_SetPalette (unsigned char *palette)
{
	byte	*pal;
	unsigned r,g,b;
	unsigned v;
	int     r1,g1,b1;
	int		j,k,l,m;
	unsigned short i;
	unsigned	*table;
	FILE *f;
	signed char s[255];
	int dist, bestdist;
	static bool palflag = false;

//
// 8 8 8 encoding
//
	pal = palette;
	table = d_8to24table;
	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;
		
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		*table++ = v;
		
	}
	d_8to24table[255] &= 0xffffff;	// 255 is transparent
	
	// JACK: 3D distance calcs - k is last closest, l is the distance.
	for (i=0; i < (1<<15); i++) {
		/* Maps
		000000000000000
		000000000011111 = Red  = 0x1F
		000001111100000 = Blue = 0x03E0
		111110000000000 = Grn  = 0x7C00
		*/
		r = ((i & 0x1F) << 3)+4;
		g = ((i & 0x03E0) >> 2)+4;
		b = ((i & 0x7C00) >> 7)+4;
		pal = (unsigned char *)d_8to24table;
		for (v=0,k=0,bestdist=10000*10000; v<256; v++,pal+=4) {
			r1 = (int)r - (int)pal[0];
			g1 = (int)g - (int)pal[1];
			b1 = (int)b - (int)pal[2];
			dist = (r1*r1)+(g1*g1)+(b1*b1);
			if (dist < bestdist) {
				k=v;
				bestdist = dist;
			}
		}
		d_15to8table[i]=k;
	}
	
}

#define MAX_INDICES 4096
uint16_t* indices;

GLuint fs[9];
GLuint vs[4];
GLuint programs[9];

void* GL_LoadShader(const char* filename, GLuint idx, GLboolean fragment){
	FILE* f = fopen(filename, "rb");
	fseek(f, 0, SEEK_END);
	long int size = ftell(f);
	fseek(f, 0, SEEK_SET);
	void* res = malloc(size);
	fread(res, 1, size, f);
	fclose(f);
	if (fragment) glShaderBinary(1, &fs[idx], 0, res, size);
	else glShaderBinary(1, &vs[idx], 0, res, size);
	free(res);
}

static int state_mask = 0;
static int texenv_mask = 0;
static int texcoord_state = 0;
static int alpha_state = 0;
static int color_state = 0;
GLint monocolor;
GLint modulcolor[2];

void GL_SetProgram(){
	switch (state_mask + texenv_mask){
		case 0x00: // Everything off
		case 0x04: // Modulate
		case 0x08: // Alpha Test
		case 0x0C: // Alpha Test + Modulate
			glUseProgram(programs[NO_COLOR]);
			break;
		case 0x01: // Texcoord
		case 0x03: // Texcoord + Color
			glUseProgram(programs[TEX2D_REPL]);
			break;
		case 0x02: // Color
		case 0x06: // Color + Modulate
			glUseProgram(programs[RGBA_COLOR]);
			break;
		case 0x05: // Modulate + Texcoord
			glUseProgram(programs[TEX2D_MODUL]);
			break;
		case 0x07: // Modulate + Texcoord + Color
			glUseProgram(programs[TEX2D_MODUL_CLR]);
			break;
		case 0x09: // Alpha Test + Texcoord
		case 0x0B: // Alpha Test + Color + Texcoord
			glUseProgram(programs[TEX2D_REPL_A]);
			break;
		case 0x0A: // Alpha Test + Color
		case 0x0E: // Alpha Test + Modulate + Color
			glUseProgram(programs[RGBA_CLR_A]);
			break;
		case 0x0D: // Alpha Test + Modulate + Texcoord
			glUseProgram(programs[TEX2D_MODUL_A]);
			break;
		case 0x0F: // Alpha Test + Modulate + Texcood + Color
			glUseProgram(programs[FULL_A]);
			break;
		default:
			break;
	}
}

void GL_EnableState(GLenum state){	
	switch (state){
		case GL_TEXTURE_COORD_ARRAY:
			if (!texcoord_state){
				state_mask += 0x01;
				texcoord_state = 1;
			}
			break;
		case GL_COLOR_ARRAY:
			if (!color_state){
				state_mask += 0x02;
				color_state = 1;
			}
			break;
		case GL_MODULATE:
			texenv_mask = 0x04;
			break;
		case GL_REPLACE:
			texenv_mask = 0;
			break;
		case GL_ALPHA_TEST:
			if (!alpha_state){
				state_mask += 0x08;
				alpha_state = 1;
			} 
			break;
	}
	GL_SetProgram();
}

void GL_DisableState(GLenum state){	
	switch (state){
		case GL_TEXTURE_COORD_ARRAY:
			if (texcoord_state){
				state_mask -= 0x01;
				texcoord_state = 0;
			}
			break;
		case GL_COLOR_ARRAY:
			if (color_state){
				state_mask -= 0x02;
				color_state = 0;
			}
			break;
		case GL_ALPHA_TEST:
			if (alpha_state){
				state_mask -= 0x08;
				alpha_state = 0;
			} 
			break;
		default:
			break;
	}
	GL_SetProgram();
}

static float cur_clr[4];

void GL_DrawPolygon(GLenum prim, int num){
	if ((state_mask + texenv_mask) == 0x05) glUniform4fv(modulcolor[0], 1, cur_clr);
	else if ((state_mask + texenv_mask) == 0x0D) glUniform4fv(modulcolor[1], 1, cur_clr);
	vglDrawObjects(prim, num, GL_TRUE);
}

void GL_Color(float r, float g, float b, float a){
	cur_clr[0] = r;
	cur_clr[1] = g;
	cur_clr[2] = b;
	cur_clr[3] = a;
}

bool shaders_set = false;
void GL_ResetShaders(){
	glFinish();
	int i;
	if (shaders_set){
		for (i=0;i<9;i++){
			glDeleteProgram(programs[i]);
		}
		for (i=0;i<9;i++){
			glDeleteShader(fs[i]);
		}
		for (i=0;i<4;i++){
			glDeleteShader(vs[i]);
		}
	}else shaders_set = true; 
	
	// Loading shaders
	for (i=0;i<9;i++){
		fs[i] = glCreateShader(GL_FRAGMENT_SHADER);
	}
	for (i=0;i<4;i++){
		vs[i] = glCreateShader(GL_VERTEX_SHADER);
	}
	
	if (gl_fog.value){
		GL_LoadShader("app0:shaders/modulate_fog_f.gxp", MODULATE, GL_TRUE);
		GL_LoadShader("app0:shaders/modulate_rgba_fog_f.gxp", MODULATE_WITH_COLOR, GL_TRUE);
		GL_LoadShader("app0:shaders/replace_fog_f.gxp", REPLACE, GL_TRUE);
		GL_LoadShader("app0:shaders/modulate_alpha_fog_f.gxp", MODULATE_A, GL_TRUE);
		GL_LoadShader("app0:shaders/modulate_rgba_alpha_fog_f.gxp", MODULATE_COLOR_A, GL_TRUE);
		GL_LoadShader("app0:shaders/replace_alpha_fog_f.gxp", REPLACE_A, GL_TRUE);
		GL_LoadShader("app0:shaders/texture2d_fog_v.gxp", TEXTURE2D, GL_FALSE);
		GL_LoadShader("app0:shaders/texture2d_rgba_fog_v.gxp", TEXTURE2D_WITH_COLOR, GL_FALSE);
	}else{
		GL_LoadShader("app0:shaders/modulate_f.gxp", MODULATE, GL_TRUE);
		GL_LoadShader("app0:shaders/modulate_rgba_f.gxp", MODULATE_WITH_COLOR, GL_TRUE);
		GL_LoadShader("app0:shaders/replace_f.gxp", REPLACE, GL_TRUE);
		GL_LoadShader("app0:shaders/modulate_alpha_f.gxp", MODULATE_A, GL_TRUE);
		GL_LoadShader("app0:shaders/modulate_rgba_alpha_f.gxp", MODULATE_COLOR_A, GL_TRUE);
		GL_LoadShader("app0:shaders/replace_alpha_f.gxp", REPLACE_A, GL_TRUE);
		GL_LoadShader("app0:shaders/texture2d_v.gxp", TEXTURE2D, GL_FALSE);
		GL_LoadShader("app0:shaders/texture2d_rgba_v.gxp", TEXTURE2D_WITH_COLOR, GL_FALSE);
	}
	
	GL_LoadShader("app0:shaders/rgba_f.gxp", RGBA_COLOR, GL_TRUE);
	GL_LoadShader("app0:shaders/vertex_f.gxp", MONO_COLOR, GL_TRUE);
	GL_LoadShader("app0:shaders/rgba_alpha_f.gxp", RGBA_A, GL_TRUE);
	GL_LoadShader("app0:shaders/rgba_v.gxp", COLOR, GL_FALSE);
	GL_LoadShader("app0:shaders/vertex_v.gxp", VERTEX_ONLY, GL_FALSE);
	
	// Setting up programs
	for (i=0;i<9;i++){
		programs[i] = glCreateProgram();
		switch (i){
			case TEX2D_REPL:
				glAttachShader(programs[i], fs[REPLACE]);
				glAttachShader(programs[i], vs[TEXTURE2D]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				break;
			case TEX2D_MODUL:
				glAttachShader(programs[i], fs[MODULATE]);
				glAttachShader(programs[i], vs[TEXTURE2D]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				modulcolor[0] = glGetUniformLocation(programs[i], "vColor");
				break;
			case TEX2D_MODUL_CLR:
				glAttachShader(programs[i], fs[MODULATE_WITH_COLOR]);
				glAttachShader(programs[i], vs[TEXTURE2D_WITH_COLOR]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				vglBindAttribLocation(programs[i], 2, "color", 4, GL_FLOAT);
				break;
			case RGBA_COLOR:
				glAttachShader(programs[i], fs[RGBA_COLOR]);
				glAttachShader(programs[i], vs[COLOR]);
				vglBindAttribLocation(programs[i], 0, "aPosition", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "aColor", 4, GL_FLOAT);
				break;
			case NO_COLOR:
				glAttachShader(programs[i], fs[MONO_COLOR]);
				glAttachShader(programs[i], vs[VERTEX_ONLY]);
				vglBindAttribLocation(programs[i], 0, "aPosition", 3, GL_FLOAT);
				monocolor = glGetUniformLocation(programs[i], "color");
				break;
			case TEX2D_REPL_A:
				glAttachShader(programs[i], fs[REPLACE_A]);
				glAttachShader(programs[i], vs[TEXTURE2D]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				break;
			case TEX2D_MODUL_A:
				glAttachShader(programs[i], fs[MODULATE_A]);
				glAttachShader(programs[i], vs[TEXTURE2D]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				modulcolor[1] = glGetUniformLocation(programs[i], "vColor");
				break;
			case FULL_A:
				glAttachShader(programs[i], fs[MODULATE_COLOR_A]);
				glAttachShader(programs[i], vs[TEXTURE2D_WITH_COLOR]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				vglBindAttribLocation(programs[i], 2, "color", 4, GL_FLOAT);
				break;
			case RGBA_CLR_A:
				glAttachShader(programs[i], fs[RGBA_A]);
				glAttachShader(programs[i], vs[COLOR]);
				vglBindAttribLocation(programs[i], 0, "aPosition", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "aColor", 4, GL_FLOAT);
				break;
		}
		glLinkProgram(programs[i]);
	}
}

static void Callback_Fog_f(cvar_t *var)
{
	if (gl_fog.value) 
		Cvar_SetValue("r_fullbright", 1);
	else
		Cvar_SetValue("r_fullbright", 0);

	GL_ResetShaders();
}

/*
===============
GL_Init
===============
*/
void GL_Init (void)
{
	gl_vendor = glGetString (GL_VENDOR);
	Con_Printf ("GL_VENDOR: %s\n", gl_vendor);
	gl_renderer = glGetString (GL_RENDERER);
	Con_Printf ("GL_RENDERER: %s\n", gl_renderer);

	gl_version = glGetString (GL_VERSION);
	Con_Printf ("GL_VERSION: %s\n", gl_version);
	gl_extensions = glGetString (GL_EXTENSIONS);
	Con_Printf ("GL_EXTENSIONS: %s\n", gl_extensions);
	
//	Con_Printf ("%s %s\n", gl_renderer, gl_version);

	glClearColor (1,0,0,0);
	glCullFace(GL_FRONT);

	//->glShadeModel (GL_FLAT);
	Cvar_RegisterVariable (&gl_fog);
	Cvar_SetCallback(&gl_fog, &Callback_Fog_f);
	GL_ResetShaders();
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	GL_EnableState(GL_ALPHA_TEST);
	GL_EnableState(GL_TEXTURE_COORD_ARRAY);
	
	glPolygonOffset(1, 14);
	
	Cvar_RegisterVariable (&show_fps); // muff
	Cvar_RegisterVariable(&vid_vsync);
	
	int i;
	indices = (uint16_t*)malloc(sizeof(uint16_t*)*MAX_INDICES);
	for (i=0;i<MAX_INDICES;i++){
		indices[i] = i;
	}
	
}

/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	*x = *y = 0;
	*width = scr_width;
	*height = scr_height;

	vglStartRendering();
	vglIndexPointerMapped(indices);
}

void GL_EndRendering (void)
{
	if (benchmark) GL_DrawBenchmark ();
	else GL_DrawFPS ();
	
	//->glFlush();

	if (isKeyboard){
		vglStopRenderingInit();
		vglUpdateCommonDialog();
		vglStopRenderingTerm();
	}else vglStopRendering();
		
}

static void Check_Gamma (unsigned char *pal)
{
	float	f, inf;
	unsigned char	palette[768];
	int		i;

	if ((i = COM_CheckParm("-gamma")) == 0) {
		vid_gamma = 0.7; // default to 0.7 on non-3dfx hardware
	} else
		vid_gamma = Q_atof(com_argv[i+1]);

	for (i=0 ; i<768 ; i++)
	{
		f = pow ( (pal[i]+1)/256.0 , vid_gamma );
		inf = f*255 + 0.5;
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		palette[i] = inf;
	}

	memcpy (pal, palette, sizeof(palette));
}

void VID_Init(unsigned char *palette)
{
	int i;
	char	gldir[MAX_OSPATH];
	int width = scr_width, height = scr_height;

	Cvar_RegisterVariable (&vid_mode);
	Cvar_RegisterVariable (&vid_redrawfull);
	Cvar_RegisterVariable (&vid_waitforrefresh);
	Cvar_RegisterVariable (&gl_ztrick);
	
	vid.maxwarpwidth = width;
	vid.maxwarpheight = height;
	vid.colormap = host_colormap;
	vid.fullbright = 0xFFFF;
	vid.aspect = (float) width / (float) height;
	vid.numpages = 2;
	vid.rowbytes = 2 * width;
	vid.width = width;
	vid.height = height;

	vid.conwidth = width;
	vid.conheight = height;
	
	GL_Init();
	
	sprintf (gldir, "%s/glquake", com_gamedir);
	Sys_mkdir (gldir);

	Check_Gamma(palette);
	VID_SetPalette(palette);

	Con_SafePrintf ("Video mode %dx%d initialized.\n", width, height);

	vid.recalc_refdef = 1;				// force a surface cache flush
}

void Force_CenterView_f (void)
{
	cl.viewangles[PITCH] = 0;
}