# Introduction
vitaQuake is a porting of Quake Engine for PSVITA based on winQuake.

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
- Fullpitch doesn't seem complete with the joystick (?)
- Rewrite CVAR support (partially done ATM)

#Credits
- idSoftware for winQuake sourcecode
- MasterFeizz for ctrQuake sourcecode i studied to understand how winQuake works
- xerpi for vita2dlib
- EasyRPG Team for the audio decoder used for CDAudio support
- Ch0wW for various improvements and code cleanup