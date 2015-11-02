#ifndef INCLUDED_KEYBOARDS_DANZEFF_H
#define INCLUDED_KEYBOARDS_DANZEFF_H

//danzeff is BSD licensed, if you do make a new renderer then please share it back and I can add it
//to the original distribution.

#include <psp2/ctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

//Initialization and de-init of the keyboard, provided as the keyboard uses alot of images, so if you aren't going to use it for a while, I'd recommend unloading it.
void danzeff_load();
void danzeff_free();

//returns true if the keyboard is initialized
/*bool*/ int danzeff_isinitialized();

/** Attempts to read a character from the controller
* If no character is pressed then we return 0
* Other special values: 1 = move left, 2 = move right, 3 = select, 4 = start
* Every other value should be a standard ascii value.
* An unsigned int is returned so in the future we can support unicode input
*/
unsigned int danzeff_readInput(SceCtrlData pspctrl);
#define DANZEFF_LEFT   1
#define DANZEFF_RIGHT  2
#define DANZEFF_SELECT 3
#define DANZEFF_START  4

//Move the area the keyboard is rendered at to here
void danzeff_moveTo(const int newX, const int newY);

//Returns true if the keyboard that would be rendered now is different to the last time
//that the keyboard was drawn, this is altered by readInput/render.
/*bool*/ int danzeff_dirty();

//draw the keyboard to the screen
void danzeff_render();

#ifdef __cplusplus
}
#endif

#endif //INCLUDED_KEYBOARDS_DANZEFF_H
