# Introduction
vitaQuake is a port of Quake 1 / ProQuake for PSVITA. It is currently based on winQuake.

# Features
- Software GPU rendering
- Native 960x544 resolution
- Sound support
- Analogs support for camera and movement
- Right analogs sensibility interchangeable in Options menu
- Savestates support
- Resolution rescaler in options menu
- CDAudio (Soundtracks) support
- Multiplayer support for both online and locale lobbies

#TODO List

Priority: HIGH:
- Switch to Hardware GPU rendering
- Add different servers type support (DarkPlaces, Cheatfree, etc)

Priority: LOW
- Add support to gyroscope.
- Enable multiplayer menu only if the user has WiFi on.
- Add "Performances" submenu.
- Make a more complete joystick configuration menu.
- ProQuake's fullpitch seems clamped at . [BUG]
- Rewrite CVAR support (partially done ATM)
- Add a CVAR to separate commands/menus, depending on the hardware.

#Credits
- idSoftware for winQuake sourcecode
- MasterFeizz for ctrQuake sourcecode i studied to understand how winQuake works
- xerpi for vita2dlib
- EasyRPG Team for the audio decoder used for CDAudio support
- Ch0wW for various improvements and code cleanup