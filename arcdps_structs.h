#pragma once

#include <cstdint>

// ArcDPS combat state change types
enum cbtstatechange {
    CBTS_NONE,              // 0 - not used
    CBTS_ENTERCOMBAT,       // 1 - src_agent entered combat
    CBTS_EXITCOMBAT,        // 2 - src_agent left combat
    CBTS_CHANGEUP,          // 3 - src_agent is now alive
    CBTS_CHANGEDEAD,        // 4 - src_agent is now dead
    CBTS_CHANGEDOWN,        // 5 - src_agent is now downed
    CBTS_SPAWN,             // 6 - src_agent is now in tracking range
    CBTS_DESPAWN,           // 7 - src_agent is no longer tracked
    CBTS_HEALTHUPDATE,      // 8 - health update
    CBTS_LOGSTART,          // 9 - log start
    CBTS_LOGEND,            // 10 - log end
    CBTS_WEAPSWAP,          // 11 - weapon swap
    CBTS_MAXHEALTHUPDATE,   // 12 - max health changed
    CBTS_POINTOFVIEW,       // 13 - recording player
    CBTS_LANGUAGE,          // 14 - text language
    CBTS_GWBUILD,           // 15 - game build
    CBTS_SHARDID,           // 16 - server shard id
    CBTS_REWARD,            // 17 - reward
    CBTS_BUFFINITIAL,       // 18 - buff on logging start
    CBTS_POSITION,          // 19 - position
    CBTS_VELOCITY,          // 20 - velocity
    CBTS_FACING,            // 21 - facing
    CBTS_TEAMCHANGE,        // 22 - team change
    CBTS_ATTACKTARGET,      // 23 - attack target
    CBTS_TARGETABLE,        // 24 - targetable state
    CBTS_MAPID,             // 25 - map id
    CBTS_REPLINFO,          // 26 - internal
    CBTS_STACKACTIVE,       // 27 - stack active
    CBTS_STACKRESET,        // 28 - stack reset
    CBTS_GUILD,             // 29 - guild
    CBTS_BUFFINFO,          // 30 - buff info
    CBTS_BUFFFORMULA,       // 31 - buff formula
    CBTS_SKILLINFO,         // 32 - skill info
    CBTS_SKILLTIMING,       // 33 - skill timing
    CBTS_BREAKBARSTATE,     // 34 - breakbar state
    CBTS_BREAKBARPERCENT,   // 35 - breakbar percent
    CBTS_ERROR,             // 36 - error
    CBTS_TAG,               // 37 - tag (commander)
    CBTS_BARRIERUPDATE,     // 38 - barrier update
    CBTS_STATRESET,         // 39 - stat reset
    CBTS_EXTENSION,         // 40 - extension event
    CBTS_APIDELAYED,        // 41 - api delayed
    CBTS_INSTANCESTART,     // 42 - instance start
    CBTS_TICKRATE,          // 43 - tick rate
    CBTS_LAST90BEFOREDOWN,  // 44 - last 90 before down
    CBTS_EFFECT,            // 45 - effect
    CBTS_IDTOGUID,          // 46 - id to guid
    CBTS_LOGNPCUPDATE,      // 47 - log npc update
    CBTS_IDLESTATE,         // 48 - idle state
    CBTS_EXTENSIONCOMBAT,   // 49 - extension combat
    CBTS_FRACTALSCALE,      // 50 - fractal scale
    CBTS_EFFECT2,           // 51 - effect 2
    CBTS_RULESET,           // 52 - ruleset
    CBTS_SQUADMARKER,       // 53 - squad marker
    CBTS_ARCDPSBUILD,       // 54 - arcdps build
    CBTS_GLIDER,            // 55 - glider
    CBTS_STUNBAR,           // 56 - stun bar
    CBTS_UNKNOWN
};

// IFF (Identify Friend or Foe)
enum iff {
    IFF_FRIEND,
    IFF_FOE,
    IFF_UNKNOWN
};

// Combat result types
enum cbtresult {
    CBTR_NORMAL,        // 0
    CBTR_CRIT,          // 1
    CBTR_GLANCE,        // 2
    CBTR_BLOCK,         // 3
    CBTR_EVADE,         // 4
    CBTR_INTERRUPT,     // 5
    CBTR_ABSORB,        // 6
    CBTR_BLIND,         // 7
    CBTR_KILLINGBLOW,   // 8
    CBTR_DOWNED,        // 9
    CBTR_BREAKBAR,      // 10
    CBTR_ACTIVATION,    // 11
    CBTR_UNKNOWN
};

// Combat activation types
enum cbtactivation {
    ACTV_NONE,
    ACTV_START,
    ACTV_QUICKNESS_UNUSED,
    ACTV_CANCEL_FIRE,
    ACTV_CANCEL_CANCEL,
    ACTV_RESET,
    ACTV_UNKNOWN
};

// Combat buff remove types
enum cbtbuffremove {
    CBTB_NONE,
    CBTB_ALL,
    CBTB_SINGLE,
    CBTB_MANUAL,
    CBTB_UNKNOWN
};

// Combat event structure
#pragma pack(push, 1)
typedef struct cbtevent {
    uint64_t time;              // timeGetTime() at time of event
    uint64_t src_agent;         // unique identifier
    uint64_t dst_agent;         // unique identifier
    int32_t value;              // event-specific value
    int32_t buff_dmg;           // estimated buff damage
    uint32_t overstack_value;   // overstack or duration
    uint32_t skillid;           // skill id
    uint16_t src_instid;        // source agent instance id
    uint16_t dst_instid;        // destination agent instance id
    uint16_t src_master_instid; // source master instance id (minions)
    uint16_t dst_master_instid; // destination master instance id
    uint8_t iff;                // friend/foe
    uint8_t buff;               // buff applied
    uint8_t result;             // combat result
    uint8_t is_activation;      // activation type
    uint8_t is_buffremove;      // buff remove type
    uint8_t is_ninety;          // src at > 90% health
    uint8_t is_fifty;           // dst at < 50% health
    uint8_t is_moving;          // moving
    uint8_t is_statechange;     // state change type
    uint8_t is_flanking;        // flanking
    uint8_t is_shields;         // shields
    uint8_t is_offcycle;        // offcycle
    uint8_t pad61;
    uint8_t pad62;
    uint8_t pad63;
    uint8_t pad64;
} cbtevent;
#pragma pack(pop)

// Agent structure
typedef struct ag {
    const char* name;   // agent name (UTF-8)
    uintptr_t id;       // unique id
    uint32_t prof;      // profession id
    uint32_t elite;     // elite spec id
    uint32_t self;      // 1 if self, 0 otherwise
    uint16_t team;      // team id
} ag;

// ArcDPS exports structure
typedef struct arcdps_exports {
    uint64_t size;              // sizeof(arcdps_exports)
    uint32_t sig;               // unique signature
    uint32_t imguivers;         // imgui version
    const char* out_name;       // display name
    const char* out_build;      // version string
    void* wnd_nofilter;         // window callback (unfiltered)
    void* combat;               // combat callback
    void* imgui;                // imgui callback
    void* options_end;          // options callback
    void* combat_local;         // local combat callback
    void* wnd_filter;           // window callback (filtered)
    void* options_windows;      // options windows callback
} arcdps_exports;

// WvW team IDs
constexpr uint16_t WVW_TEAM_RED = 705;
constexpr uint16_t WVW_TEAM_BLUE = 706;
constexpr uint16_t WVW_TEAM_GREEN = 707;

// Helper to check if team ID indicates WvW
inline bool is_wvw_team(uint16_t team) {
    return team == WVW_TEAM_RED || team == WVW_TEAM_BLUE || team == WVW_TEAM_GREEN;
}
