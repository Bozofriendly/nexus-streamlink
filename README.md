# ArcDPS WvW Killstreak Tracker

An ArcDPS plugin for Guild Wars 2 that tracks your personal killstreak in World vs. World (WvW). The kill count is written to a file that can be read by OBS or other streaming software.

## Features

- Tracks personal kills in WvW game mode
- Writes kill count to `addons/arcdps/killstreak.txt` in real-time
- Automatically resets count to 0 when you die
- Resets count when entering WvW

## Installation

1. Build the plugin (see Building section) or download a pre-built release
2. Copy `arcdps_killstreak.dll` to your Guild Wars 2 `bin64` folder (same location as `d3d11.dll` from ArcDPS)
3. Launch Guild Wars 2

## Output File

The plugin writes your current kill count to:
```
<GW2 Install>/addons/arcdps/killstreak.txt
```

This file contains a single number representing your current killstreak. You can use this with OBS Text (GDI+) source to display your killstreak on stream.

### OBS Setup

1. Add a "Text (GDI+)" source
2. Check "Read from file"
3. Browse to `<GW2 Install>/addons/arcdps/killstreak.txt`
4. Style as desired

## Building

### Requirements

- Windows 10/11
- Visual Studio 2022 (or 2019) with C++ Desktop Development workload
- CMake 3.15+

### Build Steps

**Using the batch file:**
```batch
build.bat
```

**Manual CMake:**
```batch
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

The output `arcdps_killstreak.dll` will be in the `build/Release` folder.

### Cross-compiling from Linux (MinGW)

```bash
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../mingw-toolchain.cmake ..
make
```

## How It Works

1. **Kill Detection**: Uses the `CBTR_KILLINGBLOW` combat result to detect when you personally kill an enemy player
2. **Death Detection**: Monitors `CBTS_CHANGEDEAD` state changes on your character to reset the counter
3. **WvW Detection**: Checks team IDs (705/706/707 for Red/Blue/Green) and map IDs to determine if you're in WvW

## API Reference

Based on the [ArcDPS API](https://www.deltaconnected.com/arcdps/api/).

## License

MIT License - Use as you wish.

## Disclaimer

This plugin only reads combat data provided by ArcDPS. It does not modify game behavior or provide any gameplay advantage beyond displaying information already visible in the game's combat log.
