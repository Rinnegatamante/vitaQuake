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
// cvar.h

/*

cvar_t variables are used to hold scalar or string variables that can be changed or displayed at the console or prog code as well as accessed directly
in C code.

it is sufficient to initialize a cvar_t with just the first two fields, or
you can add a CVAR_ARCHIVE flag for variables that you want saved to the configuration
file when the game is quit:

cvar_t	r_draworder = {"r_draworder","1"};
cvar_t	scr_screensize = {"screensize","1",CVAR_ARCHIVE};

Cvars must be registered before use, or they will have a 0 value instead of the float interpretation of the string.  Generally, all cvar_t declarations should be registered in the apropriate init function before any console commands are executed:
Cvar_RegisterVariable (&host_framerate);


C code usually just references a cvar in place:
if ( r_draworder.value )

It could optionally ask for the value to be looked up for a string name:
if (Cvar_VariableValue ("r_draworder"))

Interpreted prog code can access cvars with the cvar(name) or
cvar_set (name, value) internal functions:
teamplay = cvar("teamplay");
cvar_set ("registered", "1");

The user can access cvars from the console in two ways:
r_draworder			prints the current value
r_draworder 0		sets the current value to 0
Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.
*/

#define CVAR_NONE 		BIT(0) 	// No property (can be omitted)
#define CVAR_ARCHIVE 	BIT(1)	// CVAR saved on the CFG.
#define CVAR_SERVERINFO BIT(2)	// Informative CVAR from the server.
#define CVAR_ROM	 	BIT(3) 	// Only set by the engine.
#define CVAR_DEBUG		BIT(4)	// CVARs only enabled if the DEBUG flag is set.
#define	CVAR_CALLBACK	BIT(5)	// CVAR has a callback

// Specific platform CVARs. (ToDo)
#define CVAR_PSVITA		BIT(6)
#define CVAR_PSTV		BIT(7)

typedef void(*cvarcallback_t) (struct cvar_s *);

typedef struct cvar_s
{
	char	*name;
	char	*string;
	unsigned int flags;
	float	value;
	cvarcallback_t	callback;
	struct cvar_s *next;
} cvar_t;

#define CVAR(name, defaultvalue, flags) cvar_t name = {#name, #defaultvalue, flags};
#define STATIC_CVAR(name, defaultvalue, flags) static cvar_t name = {#name, #defaultvalue, flags};

void 	Cvar_RegisterVariable (cvar_t *variable);
// registers a cvar that already has the name, string, and optionally the
// archive elements set.

// equivelant to "<name> <variable>" typed at the console
void 	Cvar_Set (char *var_name, char *value);
void 	Cvar_ForceSet (char *var_name, char *value);

// set a callback function to the var
void	Cvar_SetCallback(cvar_t *var, cvarcallback_t func);

// expands value to a string and calls Cvar_Set
void	Cvar_SetValue (char *var_name, float value);

// Toggles a value
void	Cvar_ToggleValue(cvar_t *cvar);

// returns 0 if not defined or non numeric
float	Cvar_VariableValue (char *var_name);

// returns an empty string if not defined
char	*Cvar_VariableString (char *var_name);

// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits
char 	*Cvar_CompleteVariable (char *partial);

bool Cvar_Command (void);
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void 	Cvar_WriteVariables (FILE *f);
// Writes lines containing "set variable value" for all variables
// with the archive flag set to true.

cvar_t *Cvar_FindVar (char *var_name);

extern cvar_t	*cvar_vars;
