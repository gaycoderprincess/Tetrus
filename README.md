# Tetrus

NES Tetris clone with unlocked FPS and local & online multiplayer written in C++ with DirectX 11, ImGui, BASS and ENet.

The goal of this project is to be accurate to the NES version where it counts, i.e. timings, speed, rotations, spawn positions, etc. while also feeling more responsive and modern, along with some new additions to freshen up gameplay.

![ingame preview](https://i.imgur.com/QLph2y4.png)
![menu preview](https://i.imgur.com/WvAIWQ1.png)

Special thanks to my lovely wifey teddyator for the art assets.

## Features

- Couch multiplayer with up to 4 players, with co-op mode (by sharing a larger board) and versus mode
- Online multiplayer versus mode with up to 256 players
- Starting level changeable between 00, 09, 18, 19, and 29
- Options to set alternate keybinds for the same action in order to be able to play faster without an NES controller
- Option to choose between the NES randomizer, the modern bag randomizer, and a truly unbiased randomizer
- Option to show a ghost piece like the newer games
- Scoreboards of the top 10 plays for each game type, along with saved replays so they can be rewatched
- Customizable color scheme based on the NES palette table
- Sound effects from the NES version
- Bugs kept from the NES version to a reasonable extent (level counter and line counter behavior, color palette breaking at level 138, etc.)

## Building

Building is done on an Arch Linux system with CLion and vcpkg being used for the build process. 

Before you begin, clone nya-common to a folder next to this one, so it can be found.

Required packages: `mingw-w64-gcc vcpkg`

To install all dependencies, use:
```console
vcpkg install enet:x64-mingw-static
```

Once installed, copy files from `~/.vcpkg/vcpkg/installed/x64-mingw-static/`:

- `include` dir to `nya-common/3rdparty`
- `lib` dir to `nya-common/lib64`

You should be able to build the project now in CLion.

After building, make sure to also get BASS from https://www.un4seen.com/bass.html
