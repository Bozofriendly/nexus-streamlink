#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <mutex>

#include "arcdps_structs.h"

// Plugin version
#define PLUGIN_NAME "WvW Killstreak"
#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_SIG 0xB020F1 // Unique signature

// Output file path (relative to GW2 directory)
static const char* OUTPUT_FILE = "addons/arcdps/killstreak.txt";

// State
static std::atomic<uint32_t> g_killCount{0};
static std::atomic<bool> g_inWvW{false};
static std::mutex g_fileMutex;
static uintptr_t g_selfId = 0;
static uint16_t g_selfInstId = 0;

// ArcDPS exports
static arcdps_exports g_exports;

// Forward declarations
static void write_killcount_to_file();
static uintptr_t mod_combat(cbtevent* ev, ag* src, ag* dst, const char* skillname, uint64_t id, uint64_t revision);
static uintptr_t mod_wnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
            // Track self agent ID
            g_selfId = src->id;

            // Check if we're in WvW based on team ID
            if (is_wvw_team(src->team)) {
                if (!g_inWvW.load()) {
                    g_inWvW.store(true);
                    // Reset kill count when entering WvW
                    g_killCount.store(0);
                    write_killcount_to_file();
                }
            }
        }
        return 0;
    }

    // Only process in WvW
    if (!g_inWvW.load()) {
        return 0;
    }

    // Handle state changes
    if (ev->is_statechange) {
        switch (ev->is_statechange) {
            case CBTS_CHANGEDEAD:
                // Check if WE died (src is the one who died)
                if (src && src->self) {
                    // Player died - reset killstreak
                    g_killCount.store(0);
                    write_killcount_to_file();
                }
                break;

            case CBTS_TEAMCHANGE:
                // Track team changes to detect WvW entry/exit
                if (src && src->self) {
                    bool wasInWvW = g_inWvW.load();
                    bool nowInWvW = is_wvw_team(src->team);
                    g_inWvW.store(nowInWvW);

                    if (!wasInWvW && nowInWvW) {
                        // Just entered WvW - reset count
                        g_killCount.store(0);
                        write_killcount_to_file();
                    }
                }
                break;

            case CBTS_MAPID:
                // Map changed - check if it's a WvW map
                // WvW map IDs are in ranges: 38, 94-96, 1099, 1143, 968, 1206, 1323
                {
                    uint32_t mapId = static_cast<uint32_t>(ev->src_agent);
                    bool isWvWMap = (mapId == 38) ||                    // Eternal Battlegrounds
                                   (mapId >= 94 && mapId <= 96) ||     // Borderlands
                                   (mapId == 1099) ||                   // Obsidian Sanctum
                                   (mapId == 1143) ||                   // Edge of the Mists
                                   (mapId == 968) ||                    // Armistice Bastion
                                   (mapId == 1206) ||                   // Alpine Borderlands
                                   (mapId == 1323);                     // Desert Borderlands variant

                    bool wasInWvW = g_inWvW.load();
                    g_inWvW.store(isWvWMap);

                    if (!wasInWvW && isWvWMap) {
                        g_killCount.store(0);
                        write_killcount_to_file();
                    } else if (wasInWvW && !isWvWMap) {
                        // Left WvW - optionally keep the count or reset
                        // Currently keeping the count
                    }
                }
                break;
        }
        return 0;
    }

    // Handle killing blows
    // result == CBTR_KILLINGBLOW means this hit killed the target
    if (ev->result == CBTR_KILLINGBLOW) {
        // Check if WE dealt the killing blow
        if (src && src->self) {
            // Make sure target is a foe (enemy player)
            if (ev->iff == IFF_FOE && dst) {
                // Increment kill count
                g_killCount.fetch_add(1);
                write_killcount_to_file();
            }
        }
    }

    // Alternative: Track when dst_agent dies and we were the source
    // This catches kills from DoT effects etc
    if (ev->is_statechange == CBTS_CHANGEDEAD && dst) {
        // The destination agent died
        if (src && src->self && ev->iff == IFF_FOE) {
            // We killed an enemy - but only count if not already counted via KILLINGBLOW
            // To avoid double counting, we only use KILLINGBLOW above
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
    // Final write
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

    // Initialize file with 0
    g_killCount.store(0);
    write_killcount_to_file();

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
    // Could store arc functions here if needed
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
