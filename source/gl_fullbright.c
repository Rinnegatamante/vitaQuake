//gl_fullbright.c
#include "quakedef.h"
//extern qboolean mtexenabled;
int FindFullbrightTexture (byte *pixels, int num_pix)
{
	int i;
	for (i = 0; i < num_pix; i++)
	if (pixels[i] > 223)
	return 1;
	return 0;
}
void ConvertPixels (byte *pixels, int num_pixels)
{
	int i;
	for (i = 0; i < num_pixels; i++)
	if (pixels[i] < 224)
	pixels[i] = 255;
}
void DrawFullBrightTextures (msurface_t *first_surf, int num_surfs)
{
	// gl_texsort 1 version
	int i;
	msurface_t *fa;
	texture_t *t;
	if (r_fullbright.value)
	return;
	//if (mtexenabled)
	//return;
	//GL_DisableMultitexture ();
	for (fa = first_surf, i = 0; i < num_surfs; fa++, i++)
	{
		// find the correct texture
		t = R_TextureAnimation (fa->texinfo->texture);
		if (t->fullbright != -1 && fa->draw_this_frame)
		{
			glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable (GL_BLEND);
			GL_Bind (t->fullbright);
			DrawGLPoly (fa->polys);
			glDisable (GL_BLEND);
			glBlendFunc (GL_ZERO, GL_SRC_COLOR);
		}
		fa->draw_this_frame = 0;
	}
}