#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <mutex>
#include <ctime>
#include <cstdarg>

#include "arcdps_structs.h"

// Plugin version
#define PLUGIN_NAME "WvW Killstreak"
#define PLUGIN_VERSION "1.0.1"
#define PLUGIN_SIG 0xB020F1 // Unique signature

// Debug mode - set to 1 to enable logging
#define DEBUG_MODE 1

// Output file paths (relative to GW2 directory)
static const char* OUTPUT_FILE = "addons/arcdps/killstreak.txt";
static const char* DEBUG_FILE = "addons/arcdps/killstreak_debug.txt";

// State
static std::atomic<uint32_t> g_killCount{0};
static std::atomic<bool> g_inWvW{false};
static std::mutex g_fileMutex;
static std::mutex g_debugMutex;
static uintptr_t g_selfId = 0;

// ArcDPS exports
static arcdps_exports g_exports;

// Forward declarations
static void write_killcount_to_file();
static void debug_log(const char* fmt, ...);
static uintptr_t mod_combat(cbtevent* ev, ag* src, ag* dst, const char* skillname, uint64_t id, uint64_t revision);
static uintptr_t mod_wnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Debug logging
static void debug_log(const char* fmt, ...) {
#if DEBUG_MODE
    std::lock_guard<std::mutex> lock(g_debugMutex);

    FILE* f = nullptr;
    if (fopen_s(&f, DEBUG_FILE, "a") == 0 && f) {
        // Timestamp
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);
        fprintf(f, "[%02d:%02d:%02d] ", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);

        fprintf(f, "\n");
        fclose(f);
    }
#endif
}

// Write the current kill count to file
static void write_killcount_to_file() {
    std::lock_guard<std::mutex> lock(g_fileMutex);

    FILE* f = nullptr;
    if (fopen_s(&f, OUTPUT_FILE, "w") == 0 && f) {
        fprintf(f, "%u", g_killCount.load());
        fclose(f);
    }
}

// Combat callback - handles kill tracking
static uintptr_t mod_combat(cbtevent* ev, ag* src, ag* dst, const char* skillname, uint64_t id, uint64_t revision) {
    // Handle null event (agent tracking)
    if (!ev) {
        if (src && src->self) {
            g_selfId = src->id;
            debug_log("Self agent detected: id=%llu, name=%s, team=%u",
                (unsigned long long)src->id, src->name ? src->name : "null", src->team);

            // Detect WvW from team ID (teams 9-2000+ are WvW teams)
            // Red=9-299, Blue=300-599, Green=600-899, etc.
            if (src->team >= 9 && !g_inWvW.load()) {
                debug_log("WvW detected from team ID %u - enabling WvW mode", src->team);
                g_inWvW.store(true);
            }
        }
        return 0;
    }

    // Handle state changes first
    if (ev->is_statechange) {
        switch (ev->is_statechange) {
            case CBTS_ENTERCOMBAT:
                if (src && src->self) {
                    debug_log("Entered combat - self, inWvW=%d, team=%u", g_inWvW.load(), src->team);
                    // Also check team on combat enter
                    if (src->team >= 9 && !g_inWvW.load()) {
                        debug_log("WvW detected from combat team ID %u", src->team);
                        g_inWvW.store(true);
                    }
                }
                break;

            case CBTS_CHANGEDEAD:
                debug_log("CHANGEDEAD: src=%s (self=%d, prof=%u, elite=%u, team=%u)",
                    src && src->name ? src->name : "null",
                    src ? src->self : 0,
                    src ? src->prof : 0,
                    src ? src->elite : 0,
                    src ? src->team : 0);

                // Check if WE died
                if (src && src->self) {
                    debug_log("Player died - resetting killstreak from %u", g_killCount.load());
                    g_killCount.store(0);
                    write_killcount_to_file();
                }
                // Check if an enemy player died (prof > 0 means it's a player, not NPC)
                // In WvW, enemy team IDs differ from ours
                else if (src && src->prof > 0 && src->prof <= 9 && g_inWvW.load()) {
                    debug_log("Enemy player died: %s (team=%u, our_id=%llu)",
                        src->name ? src->name : "unknown",
                        src->team,
                        (unsigned long long)g_selfId);
                }
                break;

            case CBTS_MAPID:
                {
                    uint32_t mapId = static_cast<uint32_t>(ev->src_agent);
                    bool isWvWMap = (mapId == 38) ||                    // Eternal Battlegrounds
                                   (mapId >= 94 && mapId <= 96) ||     // Borderlands
                                   (mapId == 1099) ||                   // Obsidian Sanctum
                                   (mapId == 1143) ||                   // Edge of the Mists
                                   (mapId == 968) ||                    // Armistice Bastion
                                   (mapId == 1206) ||                   // Alpine Borderlands
                                   (mapId == 1323);                     // Desert Borderlands variant

                    debug_log("MAPID change: mapId=%u, isWvWMap=%d, wasInWvW=%d",
                        mapId, isWvWMap, g_inWvW.load());

                    bool wasInWvW = g_inWvW.load();
                    g_inWvW.store(isWvWMap);

                    if (!wasInWvW && isWvWMap) {
                        debug_log("Entered WvW - resetting kill count");
                        g_killCount.store(0);
                        write_killcount_to_file();
                    }
                }
                break;
        }
        return 0;
    }

    // Log ALL killing blows for debugging (regardless of WvW state)
    if (ev->result == CBTR_KILLINGBLOW) {
        debug_log("KILLINGBLOW: src=%s (self=%d), dst=%s, iff=%d, inWvW=%d, skill=%s",
            src && src->name ? src->name : "null",
            src ? src->self : 0,
            dst && dst->name ? dst->name : "null",
            ev->iff,
            g_inWvW.load(),
            skillname ? skillname : "null");

        // Check if WE dealt the killing blow to a FOE
        if (src && src->self && dst && ev->iff == IFF_FOE) {
            uint32_t newCount = g_killCount.fetch_add(1) + 1;
            debug_log("KILL COUNTED via KILLINGBLOW! New killstreak: %u", newCount);
            write_killcount_to_file();
        }
    }

    // Also log physical hits to foes (to see if we're hitting enemies at all)
    if (ev->result <= CBTR_BLIND && src && src->self && ev->iff == IFF_FOE) {
        // Only log occasionally to avoid spam - log when dealing significant damage
        if (ev->value > 1000 || ev->buff_dmg > 1000) {
            debug_log("HIT on foe: dst=%s, dmg=%d, buff_dmg=%d, result=%d",
                dst && dst->name ? dst->name : "null",
                ev->value, ev->buff_dmg, ev->result);
        }
    }

    return 0;
}

// Window callback (not used but required)
static uintptr_t mod_wnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return uMsg;
}

// Module release
static uintptr_t mod_release() {
    debug_log("Plugin unloading, final kill count: %u", g_killCount.load());
    write_killcount_to_file();
    return 0;
}

// Module init - returns exports structure
static arcdps_exports* mod_init() {
    memset(&g_exports, 0, sizeof(g_exports));

    g_exports.size = sizeof(arcdps_exports);
    g_exports.sig = PLUGIN_SIG;
    g_exports.imguivers = 18000; // IMGUI_VERSION_NUM
    g_exports.out_name = PLUGIN_NAME;
    g_exports.out_build = PLUGIN_VERSION;
    g_exports.combat = reinterpret_cast<void*>(mod_combat);
    g_exports.wnd_nofilter = reinterpret_cast<void*>(mod_wnd);

    // Initialize
    g_killCount.store(0);
    write_killcount_to_file();

    // Clear debug log and write init message
#if DEBUG_MODE
    {
        FILE* f = nullptr;
        if (fopen_s(&f, DEBUG_FILE, "w") == 0 && f) {
            fclose(f);
        }
    }
#endif
    debug_log("Plugin initialized - version %s", PLUGIN_VERSION);

    return &g_exports;
}

// DLL exports required by ArcDPS

extern "C" __declspec(dllexport) void* get_init_addr(
    char* arcversion,
    void* imguictx,
    void* id3dptr,
    HANDLE arcdll,
    void* mallocfn,
    void* freefn,
    uint32_t d3dversion
) {
    return reinterpret_cast<void*>(mod_init);
}

extern "C" __declspec(dllexport) void* get_release_addr() {
    return reinterpret_cast<void*>(mod_release);
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
