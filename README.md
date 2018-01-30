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

vitaQuake supports all official soundtrack packs for Quake and it's two official mission packs, Hipnotic and Rogue. In order for the soundtrack to work, files must be placed in a folder named /cdtracks/ in each campaign foler **(ux0:/data/quake/id1/cdtracks/, ux0:/data/quake/rogue/cdtracks/)**. The normal campaign, Hipnotic and Rogue will each have their own /cdtracks/ folder. You can find the official soundtrack for the main campaign [here](https://www.quaddicted.com/files/music/quake_music.zip).



Each soundfile will need to be renamed. By default, the "/quake/music" folder has tracks named as track02, track03, etc. For vitaQuake, add an extra "0" after "track" in order for them to be loaded properly and in order. **Ex.: track02 -> track002



# Loading Mods

vitaQuake has support for the official mission packs "rogue" and "hipnotic". These can be found online. In order to get them to load properly, place them in the "ux0:/data/quake/" folder alongside "id1". 

Both official mission packs support their own OSTs so long as they are placed properly in their "/cdtracks/" folder. More information about OST loading above.

Mod compatibility is varied, but as a general rule of thumb, mods compatable with winQuake will be compatible with vitaQuake. Some confirmed working mod packs include:

- DOPA (by machinegames)
- Scourge of Armaggon (Hipnotic)
- Dissolution of Eternity (Rogue)


# TODO List

Priority: HIGH:
- Increase engine limits (FitzQuake's protocol)

Priority: LOW
- Add support to gyroscope.
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
