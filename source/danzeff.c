#include "danzeff.h"
#include "danzeff_img.h"

#include <malloc.h>
#include <vita2d.h>

#define false 0
#define true 1

int holding = false;     //user is holding a button
int dirty = true;        //keyboard needs redrawing
int shifted = false;     //user is holding shift
int mode = 0;             //charset selected. (0 - letters or 1 - numbers)
int initialized = false; //keyboard is initialized

//Position on the 3-3 grid the user has selected (range 0-2)
int selected_x = 1;
int selected_y = 1;

int moved_x = 0, moved_y = 0; // location that we are moved to

//Variable describing where each of the images is
#define guiStringsSize 12 /* size of guistrings array */

#define MODE_COUNT 2
//this is the layout of the keyboard
char modeChar[MODE_COUNT*2][3][3][5] =
{
   {   //standard letters
      { ",abc",  ".def","!ghi" },
      { "-jkl","\010m n", "?opq" },
      { "(rst",  ":uvw",")xyz" }
   },

   {   //capital letters
      { "^ABC",  "@DEF","*GHI" },
      { "_JKL","\010M N", "\"OPQ" },
      { "=RST",  ";UVW","/XYZ" }
   },

   {   //numbers
      { "\0\0\0001","\0\0\0002","\0\0\0003" },
      { "\0\0\0004",  "\010\0 5","\0\0\0006" },
      { "\0\0\0007","\0\0\0008", "\0\00009" }
   },

   {   //special characters
      { "'(.)",  "\"<'>","-[_]" },
      { "!{?}","\010\0 \0", "+\\=/" },
      { ":@;#",  "~$`%","*^|&" }
   }
};

int danzeff_isinitialized()
{
   return initialized;
}

int danzeff_dirty()
{
   return dirty;
}



unsigned int danzeff_readInput(SceCtrlData pspctrl)
{
   //Work out where the analog stick is selecting
   int x = 1;
   int y = 1;
   if (pspctrl.lx < 85)     x -= 1;
   else if (pspctrl.lx > 170) x += 1;

   if (pspctrl.ly < 85)     y -= 1;
   else if (pspctrl.ly > 170) y += 1;

   if (selected_x != x || selected_y != y) //If they've moved, update dirty
   {
      dirty = true;
      selected_x = x;
      selected_y = y;
   }
   //if they are changing shift then that makes it dirty too
   if ((!shifted && (pspctrl.buttons & SCE_CTRL_RTRIGGER)) || (shifted && !(pspctrl.buttons & SCE_CTRL_RTRIGGER)))
      dirty = true;

   unsigned int pressed = 0; //character they have entered, 0 as that means 'nothing'
   shifted = (pspctrl.buttons & SCE_CTRL_RTRIGGER)?true:false;

   if (!holding)
   {
      if (pspctrl.buttons& (SCE_CTRL_CROSS|SCE_CTRL_CIRCLE|SCE_CTRL_TRIANGLE|SCE_CTRL_SQUARE)) //pressing a char select button
      {
         int innerChoice = 0;
         if      (pspctrl.buttons & SCE_CTRL_TRIANGLE)
            innerChoice = 0;
         else if (pspctrl.buttons & SCE_CTRL_SQUARE)
            innerChoice = 1;
         else if (pspctrl.buttons & SCE_CTRL_CROSS)
            innerChoice = 2;
         else //if (pspctrl.Buttons & PSP_CTRL_CIRCLE)
            innerChoice = 3;

         //Now grab the value out of the array
         pressed = modeChar[ mode*2 + shifted][y][x][innerChoice];
      }
      else if (pspctrl.buttons & SCE_CTRL_LTRIGGER) //toggle mode
      {
         dirty = true;
         mode++;
         mode %= MODE_COUNT;
      }
      else if (pspctrl.buttons & SCE_CTRL_DOWN)
      {
         pressed = '\n';
      }
      else if (pspctrl.buttons & SCE_CTRL_UP)
      {
         pressed = DANZEFF_LEFT;
      }
      else if (pspctrl.buttons & SCE_CTRL_LEFT)
      {
         pressed = 8; //LEFT
      }
      else if (pspctrl.buttons & SCE_CTRL_RIGHT)
      {
         pressed = DANZEFF_RIGHT; //RIGHT
      }
      else if (pspctrl.buttons & SCE_CTRL_SELECT)
      {
         pressed = DANZEFF_SELECT; //SELECT
      }
      else if (pspctrl.buttons & SCE_CTRL_START)
      {
         pressed = DANZEFF_START; //START
      }
   }

   holding = pspctrl.buttons & ~SCE_CTRL_RTRIGGER; //RTRIGGER doesn't set holding

   return pressed;
}

vita2d_texture* keyTextures[guiStringsSize];

/* load all the guibits that make up the OSK */
void danzeff_load()
{
   if (initialized) return;

   keyTextures[0] = vita2d_load_PNG_buffer(keys_start);
   keyTextures[1] = vita2d_load_PNG_buffer(keys_t_start);
   keyTextures[2] = vita2d_load_PNG_buffer(keys_s_start);
   keyTextures[3] = vita2d_load_PNG_buffer(keys_c_start);
   keyTextures[4] = vita2d_load_PNG_buffer(keys_c_t_start);
   keyTextures[5] = vita2d_load_PNG_buffer(keys_s_c_start);
   keyTextures[6] = vita2d_load_PNG_buffer(nums_start);
   keyTextures[7] = vita2d_load_PNG_buffer(nums_t_start);
   keyTextures[8] = vita2d_load_PNG_buffer(nums_s_start);
   keyTextures[9] = vita2d_load_PNG_buffer(nums_c_start);
   keyTextures[10] = vita2d_load_PNG_buffer(nums_c_t_start);
   keyTextures[11] = vita2d_load_PNG_buffer(nums_s_c_start);

   initialized = true;
}

/* remove all the guibits from memory */
void danzeff_free()
{
   if (!initialized) return;

   int a;
   for (a = 0; a < guiStringsSize; a++)
   {
      vita2d_free_texture(keyTextures[a]);
      keyTextures[a] = NULL;
   }
   initialized = false;
}

/* blit the images to screen */
void danzeff_render()
{
   dirty = false;

   if (selected_x == 1 && selected_y == 1)
        {
		int width = vita2d_texture_get_width(keyTextures[6*mode + shifted*3]);
		int height = vita2d_texture_get_height(keyTextures[6*mode + shifted*3]);
		vita2d_draw_texture_part(keyTextures[6*mode + shifted*3], moved_x, moved_y, 0, 0, width, height);
        }
   else
        {
		int width = vita2d_texture_get_width(keyTextures[6*mode + shifted*3 + 1]);
		int height = vita2d_texture_get_height(keyTextures[6*mode + shifted*3 + 1]);
		vita2d_draw_texture_part(keyTextures[6*mode + shifted*3 + 1], moved_x, moved_y, 0, 0, width, height);
        }

       vita2d_draw_texture_part(keyTextures[6*mode + shifted*3 + 2], selected_x*43 + moved_x, selected_y*43 + moved_y, selected_x*64,selected_y*64, 64, 64);
}

void danzeff_moveTo(const int newX, const int newY)
{
   moved_x = newX;
   moved_y = newY;
}
