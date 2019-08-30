# Introduction
vitaQuake is a Quake engine source port for PSVITA.

An official channel to discuss the development of this source port can be found on [Vita Nuova discord server](https://discord.gg/PyCaBx9).

# Features
- Hardware accelerated GPU rendering
- Native 960x544 resolution
- MSAA 2x and MSAA 4x support
- Dual analogs support
- Native IME for inputing commands/text
- Sounds and Musics (CDTracks) support in OGG, MP3, WAV formats
- Gyroscope and touchscreen support for camera movement
- Custom arguments support and mods support
- Support for both official missionpacks
- Support for transparent surfaces (.alpha and .renderamt)
- Increased Quake Engine limits (max vertices, max entities, max static entities, etc...)
- LAN Multiplayer support (locale and online)
- AdHoc Multiplayer support
- ProQuake net protocol support
- Savegames fully working
- Support for colored lights with .lit files support
- Support for Half-Life BSP
- Smooth animations thanks to interpolation techniques
- Crosshair and custom crosshairs support
- Mirrors support
- Specular mode support
- Fog support
- Cel Shading support
- Bilinear filtering support
- Dynamic shadows support
- Several different improvements in the renderer quality
- Several different miscellaneous features (eg: transparent statusbar, benchmark feature, working gamma, etc...)

# CDAudio Support

vitaQuake supports all soundtrack packs for Quake and its two official mission packs, "Scourge of Armagon" and "Dissolution of Eternity." In order for the soundtrack to work, files must be placed in a folder named /cdtracks/ in each campaign folder (see chart below for directories). The normal campaign and the two official mission packs will have their own /cdtracks/ folder. 

By default, the music folder has tracks named as track02, track03, etc. For vitaQuake, add an extra "0" after "track" in order for them to be loaded properly and in order. **Ex.: track02 -> track002**

You can find the official soundtrack for the main campaign in .ogg format [here](https://www.quaddicted.com/files/music/quake_music.zip).

# Loading Expansions and Mods

vitaQuake supports the official Quake expansions, "Scourge of Armagon" and "Dissolution of Eternity." These were offical expansions, so they can be found usually wherever the full base game is sold (GOG, Steam). In order to get them to load properly, place them in the "ux0:/data/quake/" folder alongside "id1". 

Both official mission packs support their own soundtracks as long as they are placed properly in their respective "/cdtracks/" folder.

Mod compatibility is varied, but as a general rule of thumb, mods compatible with winQuake will be compatible with vitaQuake.

Here's a list of some popular mods and their actual working state:

Expansion/Mod | Link | Status
---|---|---|
Dissolution of Eternity | Official Expansion | Fully Working
dopa | [Free](https://twitter.com/machinegames/status/746363189768650752?lang=en) | ![#007f00](https://placehold.it/15/007f00/000000?text=+) `Fully Working`
Halo Revamped | [Free](https://wololo.net/downloads/index.php/download/1376) | ![#d0d000](https://placehold.it/15/d0d000/000000?text=+) `Boots, lots of bugs`
Kurok | [Free](http://www.bladebattles.com/kurok/) | ![#007f00](https://placehold.it/15/007f00/000000?text=+) `Playable with glitches`
Nazi Zombies Portable | [Free](https://www.moddb.com/games/nazi-zombies-portable/news/nazi-zombies-portable-ps-vita-info) | ![#ff0000](https://placehold.it/15/ff0000/000000?text=+) `Not Working`
Scourge of Armagon | Official Expansion | ![#007f00](https://placehold.it/15/007f00/000000?text=+) `Fully Working`
SUPERHOT Quake | [Free](https://www.moddb.com/mods/superhot-quake) | ![#007f00](https://placehold.it/15/007f00/000000?text=+) `Working without monochromatic graphics`
SUPERQOT | [Free](https://superhotgame.com/SUPERQOT/) | ![#ff0000](https://placehold.it/15/ff0000/000000?text=+) `Not Working`

# Credits
- idSoftware for winQuake sourcecode
- MasterFeizz for ctrQuake sourcecode i studied to understand how winQuake works
- xerpi for vita2dlib
- EasyRPG Team for the audio decoder used for CDAudio support
- Ch0wW for various improvements and code cleanup
- JPG for ProQuake and some various fixes.
