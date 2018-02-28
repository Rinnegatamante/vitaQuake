# Introduction
vitaQuake is a port of Quake 1 / ProQuake for PSVITA. It is currently based on winQuake.

# Features
- Hardware accelerated GPU rendering
- Native 960x544 resolution
- Native IME for inputing commands/text
- Sound support
- Analogs support for camera and movement
- Right analogs sensibility interchangeable in Options menu
- Savestates support
- CDAudio (Soundtracks) support
- Multiplayer support for both online and locale lobbies

# CDAudio Support

vitaQuake supports all soundtrack packs for Quake and its two official mission packs, "Scourge of Armagon" and "Dissolution of Eternity." In order for the soundtrack to work, files must be placed in a folder named /cdtracks/ in each campaign folder (see chart below for directories). The normal campaign and the two official mission packs will have their own /cdtracks/ folder. 

By default, the music folder has tracks named as track02, track03, etc. For vitaQuake, add an extra "0" after "track" in order for them to be loaded properly and in order. **Ex.: track02 -> track002**

You can find the official soundtrack for the main campaign in .ogg format [here](https://www.quaddicted.com/files/music/quake_music.zip).

# Loading Expansions and Mods

vitaQuake supports the official Quake expansions, "Scourge of Armagon" and "Dissolution of Eternity." These were offical expansions, so they can be found usually wherever the full base game is sold (GOG, Steam). In order to get them to load properly, place them in the "ux0:/data/quake/" folder alongside "id1". 

Both official mission packs support their own soundtracks as long as they are placed properly in their respective "/cdtracks/" folder.

Mod compatibility is varied, but as a general rule of thumb, mods compatible with winQuake will be compatible with vitaQuake. Some confirmed working mod packs include:

Expansion/Mod | Status/Link | Developer | CD Audio Folder
---|---|---|---|
dopa | [Free](https://twitter.com/machinegames/status/746363189768650752?lang=en) | machinegames | N/A
Scourge of Armagon | Official Expansion | Hipnotic Software | `ux0:/data/Quake/hipnotic/cdtracks`
Dissolution of Eternity | Official Expansion | Rogue Entertainment | `ux0:/data/Quake/rogue/cdtracks`

# TODO List

Priority: HIGH:
- Increase engine limits (FitzQuake's protocol)

Priority: LOW
- Enable multiplayer menu only if the user has WiFi on.
- Add "Performances" submenu.
- Customize joystick's X/Y sensitivity
- Improve CVAR system
- Separate menus according to your hardware (PSVita/PSTV)

# Credits
- idSoftware for winQuake sourcecode
- MasterFeizz for ctrQuake sourcecode i studied to understand how winQuake works
- xerpi for vita2dlib
- EasyRPG Team for the audio decoder used for CDAudio support
- Ch0wW for various improvements and code cleanup
- JPG for ProQuake and some various fixes.
