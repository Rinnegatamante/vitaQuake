//gl_fullbright.h
#include "gl_model.h"
int FindFullbrightTexture (byte *pixels, int num_pix);
void DrawFullBrightTextures (msurface_t *first_surf, int num_surfs);
void ConvertPixels (byte *pixels, int num_pixels);