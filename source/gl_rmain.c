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
// r_main.c

#include "quakedef.h"

entity_t	r_worldentity;

bool	r_cache_thrash;		// compatability

vec3_t		modelorg, r_entorigin;
entity_t	*currententity;

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

mplane_t	frustum[4];

int			c_brush_polys, c_alias_polys;

bool	envmap;				// true during envmap command capture 

int			currenttexture = -1;		// to avoid unnecessary texture sets

int			cnttextures[2] = {-1, -1};     // cached

int			particletexture;	// little dot for particles
int			playertextures;		// up to 16 color translated skins

int			mirrortexturenum;	// quake texturenum, not gltexturenum
bool	mirror;
mplane_t	*mirror_plane;

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

extern	cvar_t		v_gamma; // muff

// idea originally nicked from LordHavoc
// re-worked + extended - muff 5 Feb 2001
// called inside polyblend
void DoGamma()
{

	if (v_gamma.value < 0.2)
		v_gamma.value = 0.2;

	if (v_gamma.value >= 1)
	{
		v_gamma.value = 1;
		return;
	}

	//believe it or not this actually does brighten the picture!!
	GL_DisableState(GL_TEXTURE_COORD_ARRAY);
	glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
	float color[4] = {1, 1, 1, v_gamma.value};
	
	float vertices[3*4] = {
		10, 100, 100,
		10,-100, 100,
		10,-100,-100,
		10, 100,-100
	};
	
	glUniform4fv(monocolor, 1, color);
	vglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 4, vertices);
	GL_DrawPolygon(GL_TRIANGLE_FAN, 4);
	
	//if we do this twice, we double the brightening effect for a wider range of gamma's
	GL_DrawPolygon(GL_TRIANGLE_FAN, 4);
	GL_EnableState(GL_TEXTURE_COORD_ARRAY);
}

//
// screen size info
//
refdef_t	r_refdef;

mleaf_t		*r_viewleaf, *r_oldviewleaf;

texture_t	*r_notexture_mip;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value


void R_MarkLeaves (void);

CVAR	(r_norefresh, 0, CVAR_NONE)
CVAR	(r_drawentities, 1, CVAR_NONE)
CVAR	(r_drawviewmodel, 1, CVAR_ARCHIVE)
CVAR	(r_speeds, 0, CVAR_NONE)
CVAR	(r_fullbright, 0, CVAR_NONE)
CVAR	(r_lightmap, 0, CVAR_NONE)
CVAR	(r_shadows, 1, CVAR_ARCHIVE) 
CVAR	(r_mirroralpha, 1.0, CVAR_NONE)
CVAR	(r_wateralpha, 1, CVAR_NONE)
CVAR	(r_dynamic, 1, CVAR_ARCHIVE)
CVAR	(r_novis, 0, CVAR_NONE)
CVAR	(gl_xflip, 0, CVAR_ARCHIVE)

// fenix@io.com: model interpolation
CVAR	(r_interpolate_model_animation, 1, CVAR_ARCHIVE)
CVAR	(r_interpolate_model_transform, 1, CVAR_ARCHIVE)

CVAR	(gl_finish, 0, CVAR_NONE)
CVAR	(gl_clear, 0, CVAR_NONE)
CVAR	(gl_cull, 1, CVAR_NONE)
CVAR	(gl_texsort, 1, CVAR_NONE)
CVAR	(gl_smoothmodels, 1, CVAR_NONE)
CVAR	(gl_affinemodels, 1, CVAR_NONE)
CVAR	(gl_polyblend, 1, CVAR_NONE)
CVAR	(gl_flashblend, 0, CVAR_NONE)
CVAR	(gl_playermip, 0, CVAR_NONE)
CVAR	(gl_nocolors, 0, CVAR_NONE)
CVAR	(gl_keeptjunctions, 1, CVAR_NONE)
CVAR	(gl_reporttjunctions, 0, CVAR_NONE)
CVAR	(gl_doubleeyes, 1, CVAR_NONE)

// Torch flares. KH
CVAR	(gl_torchflares, 1, CVAR_ARCHIVE)

extern	cvar_t	gl_ztrick;
extern bool gl_warp;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
bool R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	for (i=0 ; i<4 ; i++)
		if (BoxOnPlaneSide (mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}


void R_RotateForEntity (entity_t *e)
{
    glTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);

    glRotatef (e->angles[1],  0, 0, 1);
    glRotatef (-e->angles[0],  0, 1, 0);
    glRotatef (e->angles[2],  1, 0, 0);
}


/*
=============
R_BlendedRotateForEntity

fenix@io.com: model transform interpolation
=============
*/
void R_BlendedRotateForEntity (entity_t *e)
{
	float timepassed;
	float blend;
	vec3_t d;
	int i;

	// positional interpolation

	timepassed = realtime - e->translate_start_time; 

	if (e->translate_start_time == 0 || timepassed > 1)
	{
		e->translate_start_time = realtime;
		VectorCopy (e->origin, e->origin1);
		VectorCopy (e->origin, e->origin2);
	}

	if (!VectorCompare (e->origin, e->origin2))
	{
		e->translate_start_time = realtime;
		VectorCopy (e->origin2, e->origin1);
		VectorCopy (e->origin,  e->origin2);
		blend = 0;
	}else{
		blend =  timepassed / 0.1;

		if (cl.paused || blend > 1) blend = 1;
	}

	VectorSubtract (e->origin2, e->origin1, d);

	glTranslatef (
		e->origin1[0] + (blend * d[0]),
		e->origin1[1] + (blend * d[1]),
		e->origin1[2] + (blend * d[2]));

	// orientation interpolation (Euler angles, yuck!)

	timepassed = realtime - e->rotate_start_time; 

	if (e->rotate_start_time == 0 || timepassed > 1)
	{
		e->rotate_start_time = realtime;
		VectorCopy (e->angles, e->angles1);
		VectorCopy (e->angles, e->angles2);
	}

	if (!VectorCompare (e->angles, e->angles2))
	{
		e->rotate_start_time = realtime;
		VectorCopy (e->angles2, e->angles1);
		VectorCopy (e->angles,  e->angles2);
		blend = 0;
	}else{
		blend = timepassed / 0.1;
 
		if (cl.paused || blend > 1) blend = 1;
	}

	VectorSubtract (e->angles2, e->angles1, d);

	// always interpolate along the shortest path
	for (i = 0; i < 3; i++) 
	{
		if (d[i] > 180){
			d[i] -= 360;
		}else if (d[i] < -180){
			d[i] += 360;
		}
	}

	glRotatef ( e->angles1[1] + ( blend * d[1]),  0, 0, 1);
	glRotatef (-e->angles1[0] + (-blend * d[0]),  0, 1, 0);
	glRotatef ( e->angles1[2] + ( blend * d[2]),  1, 0, 0);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*
================
R_GetSpriteFrame
================
*/
mspriteframe_t *R_GetSpriteFrame (entity_t *currententity)
{
	msprite_t		*psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;

	psprite = currententity->model->cache.data;
	frame = currententity->frame;

	if ((frame >= psprite->numframes) || (frame < 0))
	{
		Con_Printf ("R_DrawSprite: no such frame %d\n", frame);
		frame = 0;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else
	{
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		time = cl.time + currententity->syncbase;

	// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i=0 ; i<(numframes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		pspriteframe = pspritegroup->frames[i];
	}

	return pspriteframe;
}


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	vec3_t	point;
	mspriteframe_t	*frame;
	float		*up, *right;
	vec3_t		v_forward, v_right, v_up;
	msprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache
	frame = R_GetSpriteFrame (e);
	psprite = currententity->model->cache.data;

	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
		AngleVectors (currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
	{	// normal sprite
		up = vup;
		right = vright;
	}

	GL_Color(1,1,1,1);

    GL_Bind(frame->gl_texturenum);
	GL_EnableState(GL_ALPHA_TEST);

	float* pPoint = gVertexBuffer;
	float texCoords[] = {
		0, 1,
		0, 0,
		1, 0,
		1, 1
	};
	
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->left, right, pPoint);
	pPoint += 3;

	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->left, right, pPoint);
	pPoint += 3;

	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->right, right, pPoint);
	pPoint += 3;

	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->right, right, pPoint);

	vglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 4, gVertexBuffer);
	vglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 4, texCoords);
	GL_DrawPolygon(GL_TRIANGLE_FAN, 4);

	GL_DisableState(GL_ALPHA_TEST);

}

/*
=============================================================

  ALIAS MODELS

=============================================================
*/


#define NUMVERTEXNORMALS	162

const float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

vec3_t	shadevector;
extern vec3_t lightcolor; // LordHavoc: .lit support to the definitions at the top

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
const float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anorm_dots.h"
;

float	*shadedots = (float*)r_avertexnormal_dots[0];

int	lastposenum;

// fenix@io.com: model animation interpolation
int lastposenum0;

/*
=============
GL_DrawAliasFrame
=============
*/
void GL_DrawAliasFrame (aliashdr_t *paliashdr, int posenum)
{
	float 	l;
	trivertx_t *verts;
	int		*order;
	int		count;
	
	GL_EnableState(GL_COLOR_ARRAY);
	
	lastposenum = posenum;

	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;
	order = (int *)((byte *)paliashdr + paliashdr->commands);
	
	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		
		int primType;
		float* pColor;
		float* pTexCoord;
		float* pPos;
		if (count < 0)
		{
			count = -count;
			primType = GL_TRIANGLE_FAN;
		}
		else
			primType = GL_TRIANGLE_STRIP;
		
		pColor = gColorBuffer;
		pPos = gVertexBuffer;
		pTexCoord = gTexCoordBuffer;
		int c;
		for (c = 0; c < count; ++c)
		{
			// texture coordinates come from the draw list
			*pTexCoord++ = ((float *)order)[0];
			*pTexCoord++ = ((float *)order)[1];
			order += 2;
			
			if (r_fullbright.value || !cl.worldmodel->lightdata)
				l = 1;
			else
				l = shadedots[verts->lightnormalindex];
			
			*pColor++ = l * lightcolor[0];
			*pColor++ = l * lightcolor[1];
			*pColor++ = l * lightcolor[2];
			*pColor++ = 1.0f;
			*pPos++ = verts->v[0];
			*pPos++ = verts->v[1];
			*pPos++ = verts->v[2];
			++verts;
		}
		
		vglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, count, gVertexBuffer);
		vglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, count, gTexCoordBuffer);
		vglVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, count, gColorBuffer);
		GL_DrawPolygon(primType, count);
		
	}
	
	GL_DisableState(GL_COLOR_ARRAY);
	
}

/*
=============
GL_DrawAliasBlendedFrame

fenix@io.com: model animation interpolation
=============
*/
void GL_DrawAliasBlendedFrame (aliashdr_t *paliashdr, int pose1, int pose2, float blend)
{
	float	   l;
	trivertx_t* verts1;
	trivertx_t* verts2;
	int*		order;
	int		 count;
	vec3_t	  d;
	extern vec3_t lightcolor; // LordHavoc: .lit support to the definitions at the top
	
	GL_EnableState(GL_COLOR_ARRAY);	
		
	lastposenum0 = pose1;
	lastposenum  = pose2;
		
	verts1  = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts2  = verts1;

	verts1 += pose1 * paliashdr->poseverts;
	verts2 += pose2 * paliashdr->poseverts;

	order = (int *)((byte *)paliashdr + paliashdr->commands);
			
	for (;;)
	{
		// get the vertex count and primitive type
		count = *order++;
		
		if (!count) break;
		
		int primType;
		int c;
		float* pColor;
		float* pTexCoord;
		float* pPos;
		if (count < 0)
		{
			count = -count;
			primType = GL_TRIANGLE_FAN;
		}else{
			primType = GL_TRIANGLE_STRIP;
		}

		pColor = gColorBuffer;
		pPos = gVertexBuffer;
		pTexCoord = gTexCoordBuffer;
		c = count;
		do
		{
			// texture coordinates come from the draw list
			*pTexCoord++ = ((float *)order)[0];
			*pTexCoord++ = ((float *)order)[1];
			order += 2;

			// normals and vertexes come from the frame list
			// blend the light intensity from the two frames together
			d[0] = shadedots[verts2->lightnormalindex] -
				shadedots[verts1->lightnormalindex];

			l = (shadedots[verts1->lightnormalindex] + (blend * d[0]));
			*pColor++ = l * lightcolor[0];
			*pColor++ = l * lightcolor[1];
			*pColor++ = l * lightcolor[2];
			*pColor++ = 1.0f;

			VectorSubtract(verts2->v, verts1->v, d);

			// blend the vertex positions from each frame together
			*pPos++ = verts1->v[0] + (blend * d[0]);
			*pPos++ = verts1->v[1] + (blend * d[1]);
			*pPos++ = verts1->v[2] + (blend * d[2]);

			verts1++;
			verts2++;
		} while (--count);
		vglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, c, gVertexBuffer);
		vglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, c, gTexCoordBuffer);
		vglVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, c, gColorBuffer);
		GL_DrawPolygon(primType, c);
	}
	GL_DisableState(GL_COLOR_ARRAY);
}

/*
=============
GL_DrawAliasShadow
=============
*/
extern	vec3_t			lightspot;

void GL_DrawAliasShadow (aliashdr_t *paliashdr, int posenum)
{
	float	s, t, l;
	int		i, j;
	int		index;
	trivertx_t	*v, *verts;
	int		list;
	int		*order;
	vec3_t	point;
	float	*normal;
	float	height, lheight;
	int		count;

	lheight = currententity->origin[2] - lightspot[2];

	height = 0;
	verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts += posenum * paliashdr->poseverts;
	order = (int *)((byte *)paliashdr + paliashdr->commands);

	height = -lheight + 1.0;

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		
		int primType;
		int c;
		float* pVertex;
		
		if (count < 0)
		{
			count = -count;
			primType = GL_TRIANGLE_FAN;
		}else
			primType = GL_TRIANGLE_STRIP;
		
		pVertex = gVertexBuffer;
		for(c = 0; c < count; c++)
		{
			// texture coordinates come from the draw list
			// (skipped for shadows) glTexCoord2fv ((float *)order);
			order += 2;

			// normals and vertexes come from the frame list
			pVertex[0] = verts->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
			pVertex[1] = verts->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
			pVertex[2] = verts->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];

			pVertex[0] -= shadevector[0]*(pVertex[2]+lheight);
			pVertex[1] -= shadevector[1]*(pVertex[2]+lheight);
			pVertex[2] = height;
	//		height -= 0.001;

			pVertex += 3;
			verts++;
			
		}

		GL_DisableState(GL_TEXTURE_COORD_ARRAY);
		const float color[] = {0,0,0,0.5f};
		glUniform4fv(monocolor, 1, color);
		vglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, count, gVertexBuffer);
		GL_DrawPolygon(primType, count);
		GL_EnableState(GL_TEXTURE_COORD_ARRAY);
	}	
}

/*
=============
GL_DrawAliasBlendedShadow

fenix@io.com: model animation interpolation
=============
*/
void GL_DrawAliasBlendedShadow (aliashdr_t *paliashdr, int pose1, int pose2, entity_t* e)
{
	trivertx_t* verts1;
	trivertx_t* verts2;
	int*		  order;
	vec3_t		point1;
	vec3_t		point2;
	vec3_t		d;
	float		 height;
	float		 lheight;
	int count;
	float		 blend;

	GL_DisableState(GL_TEXTURE_COORD_ARRAY);
	
	blend = (realtime - e->frame_start_time) / e->frame_interval;

	if (blend > 1) blend = 1;

	lheight = e->origin[2] - lightspot[2];
	height  = -lheight + 1.0;

	verts1 = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
	verts2 = verts1;

	verts1 += pose1 * paliashdr->poseverts;
	verts2 += pose2 * paliashdr->poseverts;

	order = (int *)((byte *)paliashdr + paliashdr->commands);

	for (;;){
		// get the vertex count and primitive type
		count = *order++;

		if (!count) break;

		int primType;
		int c;
		float* pVertex;
		
		if (count < 0){
			count = -count;
			primType = GL_TRIANGLE_FAN;
		}else{
			primType = GL_TRIANGLE_STRIP;
		}

		pVertex = gVertexBuffer;
		c = count;
		do{
			order += 2;

			point1[0] = verts1->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
			point1[1] = verts1->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
			point1[2] = verts1->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];
		  
			point1[0] -= shadevector[0]*(point1[2]+lheight);
			point1[1] -= shadevector[1]*(point1[2]+lheight);

			point2[0] = verts2->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
			point2[1] = verts2->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
			point2[2] = verts2->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];

			point2[0] -= shadevector[0]*(point2[2]+lheight);
			point2[1] -= shadevector[1]*(point2[2]+lheight);

			VectorSubtract(point2, point1, d);
			
			pVertex[0] = point1[0] + (blend * d[0]);
			pVertex[1] = point1[1] + (blend * d[1]);
			pVertex[2] = height;
			pVertex += 3;
			
			verts1++;
			verts2++;
		} while (--count);

		const float color[4] = {0,0,0,0.5f};
		glUniform4fv(monocolor, 1, color);
		vglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, c, gVertexBuffer);
		GL_DrawPolygon(primType, c);
	}
	GL_EnableState(GL_TEXTURE_COORD_ARRAY);
}

/*
=================
R_SetupAliasFrame

=================
*/
void R_SetupAliasFrame (int frame, aliashdr_t *paliashdr)
{
	int				pose, numposes;
	float			interval;

	if ((frame >= paliashdr->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", frame);
		frame = 0;
	}

	pose = paliashdr->frames[frame].firstpose;
	numposes = paliashdr->frames[frame].numposes;

	if (numposes > 1)
	{
		interval = paliashdr->frames[frame].interval;
		pose += (int)(cl.time / interval) % numposes;
	}

	GL_DrawAliasFrame (paliashdr, pose);
}

/*
=================
R_SetupAliasBlendedFrame

fenix@io.com: model animation interpolation
=================
*/
void R_SetupAliasBlendedFrame (int frame, aliashdr_t *paliashdr, entity_t* e)
{
	int	pose;
	int	numposes;
	float blend;

	if ((frame >= paliashdr->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_AliasSetupFrame: no such frame %d\n", frame);
		frame = 0;
	}

	pose = paliashdr->frames[frame].firstpose;
	numposes = paliashdr->frames[frame].numposes;

	if (numposes > 1)
	{
		e->frame_interval = paliashdr->frames[frame].interval;
		pose += (int)(cl.time / e->frame_interval) % numposes;
	}else {
		/* One tenth of a second is a good for most Quake animations.
		If the nextthink is longer then the animation is usually meant to pause
		(e.g. check out the shambler magic animation in shambler.qc).  If its
		shorter then things will still be smoothed partly, and the jumps will be
		less noticable because of the shorter time.  So, this is probably a good
		assumption. */
		e->frame_interval = 0.1;
	}

	if (e->pose2 != pose)
	{
		e->frame_start_time = realtime;
		e->pose1 = e->pose2;
		e->pose2 = pose;
		blend = 0;
	}else{
		blend = (realtime - e->frame_start_time) / e->frame_interval;
	}
		
	// wierd things start happening if blend passes 1
	if (cl.paused || blend > 1) blend = 1;
	
	GL_DrawAliasBlendedFrame (paliashdr, e->pose1, e->pose2, blend);
}

/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel (entity_t *e)
{
	int			i, j;
	int			lnum;
	vec3_t		dist;
	float		add;
	model_t		*clmodel;
	vec3_t		mins, maxs;
	aliashdr_t	*paliashdr;
	trivertx_t	*verts, *v;
	int			index;
	float		s, t, an;
	int			anim;
	
	bool    torch = false; // Flags is this model is a torch

	clmodel = currententity->model;

	VectorAdd (currententity->origin, clmodel->mins, mins);
	VectorAdd (currententity->origin, clmodel->maxs, maxs);

	if (R_CullBox (mins, maxs))
		return;


	VectorCopy (currententity->origin, r_entorigin);
	VectorSubtract (r_origin, r_entorigin, modelorg);
	
	//
	// get lighting information
	//
	// LordHavoc: .lit support begin
	R_LightPoint(currententity->origin); // LordHavoc: lightcolor is all that matters from this
	// LordHavoc: .lit support end

	// LordHavoc: .lit support begin
	if (e == &cl.viewent)
	{
		if (lightcolor[0] < 24)
			lightcolor[0] = 24;
		if (lightcolor[1] < 24)
			lightcolor[1] = 24;
		if (lightcolor[2] < 24)
			lightcolor[2] = 24;
	}
	// LordHavoc: .lit support end

	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if (cl_dlights[lnum].die >= cl.time)
		{
			VectorSubtract (currententity->origin,
							cl_dlights[lnum].origin,
							dist);
			add = cl_dlights[lnum].radius - Length(dist);
			// LordHavoc: .lit support begin
			if (add > 0)
			{
				lightcolor[0] += add * cl_dlights[lnum].color[0];
				lightcolor[1] += add * cl_dlights[lnum].color[1];
				lightcolor[2] += add * cl_dlights[lnum].color[2];
			}
			// LordHavoc: .lit support end
		}
	}

	// ZOID: never allow players to go totally black
	i = currententity - cl_entities;
	if (i >= 1 && i<=cl.maxclients /* && !strcmp (currententity->model->name, "progs/player.mdl") */)
	{
		// LordHavoc: .lit support begin
		if (lightcolor[0] < 8)
			lightcolor[0] = 8;
		if (lightcolor[1] < 8)
			lightcolor[1] = 8;
		if (lightcolor[2] < 8)
			lightcolor[2] = 8;
		// LordHavoc: .lit support end
	}

	// HACK HACK HACK -- no fullbright colors, so make torches full light
	if (!strcmp (clmodel->name, "progs/flame2.mdl")
		|| !strcmp (clmodel->name, "progs/flame.mdl") )
	{
		torch = true; // This model is a torch. KH
		// LordHavoc: .lit support begin
		lightcolor[0] = lightcolor[1] = lightcolor[2] = 256;
		// LordHavoc: .lit support end
	}
	
	shadedots = (float*)r_avertexnormal_dots[((int)(e->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
	// LordHavoc: .lit support begin
	VectorScale(lightcolor, 1.0f / 200.0f, lightcolor);
	// LordHavoc: .lit support end
	
	an = e->angles[1]/180*M_PI;
	shadevector[0] = cosf(-an);
	shadevector[1] = sinf(-an);
	shadevector[2] = 1;
	VectorNormalize (shadevector);

	//
	// locate the proper data
	//
	paliashdr = (aliashdr_t *)Mod_Extradata (currententity->model);

	c_alias_polys += paliashdr->numtris;

	//
	// draw all the triangles
	//

    glPushMatrix ();
	
	// fenix@io.com: model transform interpolation
	if (r_interpolate_model_transform.value){
		R_BlendedRotateForEntity (e);
	}else{
		R_RotateForEntity (e);
	}

	if (!strcmp (clmodel->name, "progs/eyes.mdl") && gl_doubleeyes.value) {
		glTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2] - 30);
// double size of eyes, since they are really hard to see in gl
		glScalef (paliashdr->scale[0]*2, paliashdr->scale[1]*2, paliashdr->scale[2]*2);
	} else {
		glTranslatef (paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
		glScalef (paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);
	}

	anim = (int)(cl.time*10) & 3;
    GL_Bind(paliashdr->gl_texturenum[currententity->skinnum][anim]);

	// we can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (currententity->colormap != vid.colormap && !gl_nocolors.value)
	{
		i = currententity - cl_entities;
		if (i >= 1 && i<=cl.maxclients /* && !strcmp (currententity->model->name, "progs/player.mdl") */)
		    GL_Bind(playertextures - 1 + i);
	}

	//->if (gl_smoothmodels.value)
	//->	glShadeModel (GL_SMOOTH);
	GL_EnableState(GL_MODULATE);

	//->if (gl_affinemodels.value)
	//->	glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	// fenix@io.com: model animation interpolation
	if (r_interpolate_model_animation.value)
	{
		R_SetupAliasBlendedFrame (currententity->frame, paliashdr, currententity);
	}else{
		R_SetupAliasFrame (currententity->frame, paliashdr);
	}

	GL_EnableState(GL_REPLACE);

	//->glShadeModel (GL_FLAT);
	//->if (gl_affinemodels.value)
	//->	glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glPopMatrix ();
	
	if (torch && gl_torchflares.value) {
		// Draw torch flares. KH
		// NOTE: It would be better if we batched these up.
		//       All those state changes are not nice. KH
		
		// This relies on unchanged game code!
		const int TORCH_STYLE = 1; // Flicker.

		vec3_t  lightorigin;    // Origin of torch.
		vec3_t  v;              // Vector to torch.
		float   radius;         // Radius of torch flare.
		float   distance;       // Vector distance to torch.
		float   intensity;      // Intensity of torch flare.

		// NOTE: I don't think this is centered on the model.
		VectorCopy(currententity->origin, lightorigin);

		radius = 20.0f;
		VectorSubtract(lightorigin, r_origin, v);

		// See if view is outside the light.
		distance = Length(v);
		if (distance > radius) {
		    glDepthMask (0);
		    //->glShadeModel (GL_SMOOTH);
		    glEnable (GL_BLEND);
		    glBlendFunc (GL_ONE, GL_ONE);

		    // Translate the glow to coincide with the flame. KH
		    glPushMatrix();
		    glTranslatef(0.0f, 0.0f, 8.0f);
			
			GL_DisableState(GL_TEXTURE_COORD_ARRAY);
			GL_EnableState(GL_COLOR_ARRAY);
			float* pPos = gVertexBuffer;
			float* pColor = gColorBuffer;
			
		    // Diminish torch flare inversely with distance.
		    intensity = (1024.0f - distance) / 1024.0f;

		    // Invert (fades as you approach).
		    intensity = (1.0f - intensity);

		    // Clamp, but don't let the flare disappear.
		    if (intensity > 1.0f) intensity = 1.0f;
		    if (intensity < 0.0f) intensity = 0.0f;

		    // Now modulate with flicker.
		    i = (int)(cl.time*10);
		    if (!cl_lightstyle[TORCH_STYLE].length) {
		        j = 256;
		    } else {
		        j = i % cl_lightstyle[TORCH_STYLE].length;
		        j = cl_lightstyle[TORCH_STYLE].map[j] - 'a';
		        j = j*22;
		    }
		    intensity *= ((float)j / 255.0f);

		    // Set yellow intensity			
			*pColor++ = 0.8f*intensity;
			*pColor++ = 0.4f*intensity;
			*pColor++ = 0.1f;
			*pColor++ = 1.0f;
			
		    for (i=0 ; i<3 ; i++)
		        *pPos++ = lightorigin[i] - vpn[i]*radius;
			
		    for (i=16; i>=0; i--) {
				*pColor++ = 0.0f;
				*pColor++ = 0.0f;
				*pColor++ = 0.0f;
				*pColor++ = 0.0f;
		        for (j=0; j<3; j++)
		            *pPos++ =  lightorigin[j] + 
		            vright[j]*costablef[i]*radius +
		            vup[j]*sintablef[i]*radius;
		    }
			
			vglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 18, gVertexBuffer);
			vglVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 18, gColorBuffer);
		    GL_DrawPolygon(GL_TRIANGLE_FAN, 18);
			GL_EnableState(GL_TEXTURE_COORD_ARRAY);
			GL_DisableState(GL_COLOR_ARRAY);
			
		    // Restore previous matrix! KH
		    glPopMatrix();

		    GL_Color(1,1,1,1);
		    glDisable (GL_BLEND);
		    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		    glDepthMask (1);
		}
	}

	if (r_shadows.value) {
		//
		// Test for models that we don't want to shadow. KH
		// Not a nice way to do it...
		//

		// Torches. Early-out to avoid the strcmp's. KH
		if (torch)
		    return;
		// Grenades. KH
		if (!strcmp (clmodel->name, "progs/grenade.mdl"))
		    return;
		// Lightning bolts. KH
		if (!strncmp (clmodel->name, "progs/bolt", 10))
		return;
	
		glPushMatrix ();
		
		// fenix@io.com: model transform interpolation
		if (r_interpolate_model_transform.value){
			R_BlendedRotateForEntity (e);
		}else{
			R_RotateForEntity (e);
		}
			
		GL_DisableState(GL_TEXTURE_COORD_ARRAY);
		glEnable (GL_BLEND);
		
		// fenix@io.com: model animation interpolation
		if (r_interpolate_model_animation.value){
			GL_DrawAliasBlendedShadow (paliashdr, lastposenum0, lastposenum, currententity);
		}else{
			GL_DrawAliasShadow (paliashdr, lastposenum);
		}
			
		GL_EnableState(GL_TEXTURE_COORD_ARRAY);
		glDisable (GL_BLEND);
		GL_Color(1,1,1,1);
		glPopMatrix ();
	}

}

//==================================================================================

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;

	if (!r_drawentities.value)
		return;
	
	// draw sprites seperately, because of alpha blending
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];
		if (currententity == &cl_entities[cl.viewentity]) currententity->angles[0] *= 0.3;
		switch (currententity->model->type)
		{
		case mod_alias:
			R_DrawAliasModel (currententity);
			break;
		case mod_brush:
			glEnable(GL_POLYGON_OFFSET_FILL);
			R_DrawBrushModel (currententity);
			glDisable(GL_POLYGON_OFFSET_FILL);
			break;
		default:
			break;
		}
	}
	
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = cl_visedicts[i];

		switch (currententity->model->type)
		{
		case mod_sprite:
			R_DrawSpriteModel (currententity);
			break;
		}
	}

	//glEnable (GL_DEPTH_TEST);

}
/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel (void)
{
	// fenix@io.com: model transform interpolation
	float old_interpolate_model_transform;
	
	if (!r_drawviewmodel.value)
		return;

	if (chase_active.value)
		return;

	if (envmap)
		return;

	if (!r_drawentities.value)
		return;

	if (cl.items & IT_INVISIBILITY)
		return;

	if (cl.stats[STAT_HEALTH] <= 0)
		return;

	currententity = &cl.viewent;
	if (!currententity->model)
		return;

	// hack the depth range to prevent view model from poking into walls
	glDepthRangef (gldepthmin, gldepthmin + 0.3f*(gldepthmax-gldepthmin));
	
	// fenix@io.com: model transform interpolation
	old_interpolate_model_transform = r_interpolate_model_transform.value;
	r_interpolate_model_transform.value = false;
	R_DrawAliasModel (currententity);
	r_interpolate_model_transform.value = old_interpolate_model_transform;
	
	glDepthRangef (gldepthmin, gldepthmax);
}


/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (!gl_polyblend.value)
		return;

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL_DisableState(GL_ALPHA_TEST);
	glEnable (GL_BLEND);
	glDisable (GL_DEPTH_TEST);
	GL_DisableState(GL_TEXTURE_COORD_ARRAY);

    glLoadIdentity ();

    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up

	GL_Color(v_blend[0],v_blend[1],v_blend[2],v_blend[3]);
	
	if (v_blend[3]){
	
		float vertex[3*4] = {
			10, 100, 100,
			10, -100, 100,
			10, -100, -100,
			10, 100, -100
		};

		vglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 4, vertex);
		glUniform4fv(monocolor, 1, v_blend);
		GL_DrawPolygon(GL_TRIANGLE_FAN, 4);
		
	}
	
	//gamma trick based on LordHavoc - muff
	if (v_gamma.value != 1)
		DoGamma();
	
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable (GL_BLEND);
	GL_EnableState(GL_TEXTURE_COORD_ARRAY);
	GL_EnableState(GL_ALPHA_TEST);
}


int SignbitsForPlane (mplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


void R_SetFrustum (void)
{
	int		i;

	if (r_refdef.fov_x == 90) 
	{
		// front side is visible

		VectorAdd (vpn, vright, frustum[0].normal);
		VectorSubtract (vpn, vright, frustum[1].normal);

		VectorAdd (vpn, vup, frustum[2].normal);
		VectorSubtract (vpn, vup, frustum[3].normal);
	}
	else
	{
		// rotate VPN right by FOV_X/2 degrees
		RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_refdef.fov_x / 2 ) );
		// rotate VPN left by FOV_X/2 degrees
		RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_refdef.fov_x / 2 );
		// rotate VPN up by FOV_X/2 degrees
		RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_refdef.fov_y / 2 );
		// rotate VPN down by FOV_X/2 degrees
		RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_refdef.fov_y / 2 ) );
	}

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}



/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int				edgecount;
	vrect_t			vrect;
	float			w, h;

// don't allow cheats in multiplayer
	if (cl.maxclients > 1)
		Cvar_Set ("r_fullbright", "0");

	R_AnimateLight ();

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, r_origin);

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

// current viewleaf
	r_oldviewleaf = r_viewleaf;
	r_viewleaf = Mod_PointInLeaf (r_origin, cl.worldmodel);

	V_SetContentsColor (r_viewleaf->contents);
	V_CalcBlend ();

	r_cache_thrash = false;

	c_brush_polys = 0;
	c_alias_polys = 0;

}


void MYgluPerspective( GLdouble fovy, GLdouble aspect,
		     GLdouble zNear, GLdouble zFar )
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan( fovy * M_PI / 360.0 );
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

   glFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
	float	yfov;
	int		i;
	extern	int glwidth, glheight;
	int		x, x2, y2, y, w, h;

	//
	// set up viewpoint
	//
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	x = r_refdef.vrect.x * glwidth/vid.width;
	x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * glwidth/vid.width;
	y = (vid.height-r_refdef.vrect.y) * glheight/vid.height;
	y2 = (vid.height - (r_refdef.vrect.y + r_refdef.vrect.height)) * glheight/vid.height;

	// fudge around because of frac screen scale
	if (x > 0)
		x--;
	if (x2 < glwidth)
		x2++;
	if (y2 < 0)
		y2--;
	if (y < glheight)
		y++;

	w = x2 - x;
	h = y - y2;

	if (envmap)
	{
		x = y2 = 0;
		w = h = 256;
	}

	glViewport (glx + x, gly + y2, w, h);
    screenaspect = (float)r_refdef.vrect.width/r_refdef.vrect.height;
//	yfov = 2*atan((float)r_refdef.vrect.height/r_refdef.vrect.width)*180/M_PI;
    MYgluPerspective (r_refdef.fov_y,  screenaspect,  4,  4096);

	if (mirror)
	{
		if (mirror_plane->normal[2])
			glScalef (1, -1, 1);
		else
			glScalef (-1, 1, 1);
		glCullFace(GL_BACK);
	}
	else
		glCullFace(GL_FRONT);

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();

    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up
	if (gl_xflip.value){
		glScalef (1, -1, 1);
		glCullFace(GL_BACK);
	}
    glRotatef (-r_refdef.viewangles[2],  1, 0, 0);
    glRotatef (-r_refdef.viewangles[0],  0, 1, 0);
    glRotatef (-r_refdef.viewangles[1],  0, 0, 1);
    glTranslatef (-r_refdef.vieworg[0],  -r_refdef.vieworg[1],  -r_refdef.vieworg[2]);

	glGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//
	if (gl_cull.value)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);

	glDisable(GL_BLEND);
	GL_DisableState(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
}

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void R_RenderScene (void)
{
	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

	R_DrawWorld ();		// adds static entities to the list

	S_ExtraUpdate ();	// don't let sound get messed up if going slow
		
	R_DrawEntitiesOnList ();

	R_RenderDlights ();
	
	R_DrawParticles ();
	
#ifdef GLTEST
	Test_Draw ();
#endif

}


/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	if (r_mirroralpha.value != 1.0)
	{
		
		if (gl_clear.value){
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}else{
			glClear (GL_DEPTH_BUFFER_BIT);
		}
		gldepthmin = 0;
		gldepthmax = 0.5;
		glDepthFunc (GL_LEQUAL);
	}
	else if (gl_ztrick.value)
	{
		static int trickframe;

		if (gl_clear.value){
			glClear (GL_COLOR_BUFFER_BIT);
		}
		
		trickframe++;
		if (trickframe & 1)
		{
			gldepthmin = 0;
			gldepthmax = 0.49999;
			glDepthFunc (GL_LEQUAL);
		}
		else
		{
			gldepthmin = 1;
			gldepthmax = 0.5;
			glDepthFunc (GL_GEQUAL);
		}
	}
	else
	{
		if (gl_clear.value){
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}else{
			glClear (GL_DEPTH_BUFFER_BIT);
		}
		gldepthmin = 0;
		gldepthmax = 1;
		glDepthFunc (GL_LEQUAL);
	}

	glDepthRangef (gldepthmin, gldepthmax);
}

/*
=============
R_Mirror
=============
*/
void R_Mirror (void)
{
	float		d;
	msurface_t	*s;
	entity_t	*ent;

	if (!mirror)
		return;

	memcpy (r_base_world_matrix, r_world_matrix, sizeof(r_base_world_matrix));

	d = DotProduct (r_refdef.vieworg, mirror_plane->normal) - mirror_plane->dist;
	VectorMA (r_refdef.vieworg, -2*d, mirror_plane->normal, r_refdef.vieworg);

	d = DotProduct (vpn, mirror_plane->normal);
	VectorMA (vpn, -2*d, mirror_plane->normal, vpn);

	r_refdef.viewangles[0] = -asin (vpn[2])/M_PI*180;
	r_refdef.viewangles[1] = atan2 (vpn[1], vpn[0])/M_PI*180;
	r_refdef.viewangles[2] = -r_refdef.viewangles[2];

	ent = &cl_entities[cl.viewentity];
	if (cl_numvisedicts < MAX_VISEDICTS)
	{
		cl_visedicts[cl_numvisedicts] = ent;
		cl_numvisedicts++;
	}

	gldepthmin = 0.5f;
	gldepthmax = 1.0f;
	glDepthRangef (gldepthmin, gldepthmax);
	glDepthFunc (GL_LEQUAL);

	R_RenderScene ();
	R_DrawWaterSurfaces ();

	gldepthmin = 0.0f;
	gldepthmax = 0.5f;
	glDepthRangef (gldepthmin, gldepthmax);
	glDepthFunc (GL_LEQUAL);

	// blend on top
	glEnable (GL_BLEND);
	
	//mirror fix - from QER
	GL_EnableState(GL_MODULATE);
	//mirror fix
	
	glMatrixMode(GL_PROJECTION);
	if (mirror_plane->normal[2])
		glScalef (1,-1,1);
	else
		glScalef (-1,1,1);
	glCullFace(GL_FRONT);
	glMatrixMode(GL_MODELVIEW);

	glLoadMatrixf (r_base_world_matrix);

	GL_Color(1,1,1,r_mirroralpha.value);
	s = cl.worldmodel->textures[mirrortexturenum]->texturechain;
	for ( ; s ; s=s->texturechain)
		R_RenderBrushPoly (s);
	cl.worldmodel->textures[mirrortexturenum]->texturechain = NULL;
	glDisable (GL_BLEND);
	GL_Color(1,1,1,1);
	
	//mirror fix - from QER
	GL_EnableState(GL_REPLACE);
	//mirror fix
	
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView (void)
{
	double	time1, time2;

	if (r_norefresh.value)
		return;

	if (!r_worldentity.model || !cl.worldmodel)
		Sys_Error ("R_RenderView: NULL worldmodel");

	if (r_speeds.value)
	{
		time1 = Sys_FloatTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	mirror = false;

	R_Clear ();

	// render normal view

	R_RenderScene ();
	R_DrawViewModel ();
	R_DrawWaterSurfaces ();

	// render mirror view
	R_Mirror ();

	R_PolyBlend ();

	if (r_speeds.value)
	{
		time2 = Sys_FloatTime ();
		Con_Printf ("%3i ms  %4i wpoly %4i epoly\n", (int)((time2-time1)*1000), c_brush_polys, c_alias_polys); 
	}
}
