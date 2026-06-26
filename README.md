# Nexus Streamlink

A Guild Wars 2 Nexus addon that tracks your personal WvW killstreak, squad status, and player alive/downed/dead state. Data is written to text files for use with OBS or other streaming software.

## Features

- Tracks personal kills in WvW (player kills only, not NPCs)
  - Writes kill count to a configurable file in real-time
  - Automatically resets kill count to 0 when you die in WvW
- Tracks squad membership status
- Tracks player alive/downed/dead state (works in all game modes)

## Installation

1. Install [Nexus](https://raidcore.gg/Nexus) if you haven't already
2. Install [ArcDPS Integration](https://raidcore.gg/Addons?search=arcdps+integration) from Nexus addon library
3. Download `nexus_streamlink.dll` from the [Releases](../../releases) page
4. Place the DLL in your `<GW2 Install>/addons/` folder
5. Launch Guild Wars 2 with Nexus
6. (Optional) Edit `<GW2>/addons/streamlink/settings.txt` to change the killstreak output path

## Output Files

All files are located in `<GW2 Install>/addons/streamlink/` by default.

| File | Content | Description |
|------|---------|-------------|
| `killstreak.txt` | `0`, `1`, `2`, ... | Current WvW killstreak count. Resets to `0` on death. |
| `squad.txt` | `0` or `1` | `1` if you are in a squad, `0` if not. |
| `playerstatus.txt` | `alive`, `downed`, or `dead` | Your character's current alive state. Works in all game modes. |

## OBS Setup

1. Add a "Text (GDI+)" source
2. Check "Read from file"
3. Browse to the desired file in `<GW2 Install>/addons/streamlink/`
4. Style as desired

Repeat for each file you want to display on stream.

## How It Works

- **Kill Detection**: Uses the `KILLINGBLOW` combat result from ArcDPS local events to detect when you personally kill an enemy player
- **Death/Downed/Alive Detection**: Monitors `CHANGEUP`, `CHANGEDOWN`, and `CHANGEDEAD` state changes from ArcDPS squad events
- **WvW Detection**: Uses MumbleLink shared memory to check map type and determine if you're in WvW
- **Squad Detection**: Uses Unofficial Extras squad update events to track squad membership

## API References

- [Nexus API Documentation](https://christopher-trent.com/api-docs/)
- [ArcDPS API](https://www.deltaconnected.com/arcdps/api/)

## License

MIT License - Use as you wish.

## Disclaimer

This addon only reads combat data. It does not modify game behavior or provide any gameplay advantage beyond displaying information already visible in the game's combat log.
