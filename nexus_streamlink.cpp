///----------------------------------------------------------------------------------------------------
/// WvW Killstreak Tracker - Nexus Addon
///
/// Tracks personal kills in WvW and writes the killstreak count to a file for OBS integration.
///----------------------------------------------------------------------------------------------------

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <mutex>
#include <string>
#include <unordered_set>

#include "Nexus.h"
#include "ArcDPS.h"
#include "UnofficialExtras.h"

// Note: ImGui UI disabled - configure via settings file at:
// <GW2>/addons/streamlink/settings.txt

// Plugin info
#define ADDON_NAME "Nexus Streamlink"
// Negative signature for non-Raidcore hosted addons (cast to uint32_t)
#define ADDON_SIGNATURE static_cast<uint32_t>(-0xB020F1)

// State
static std::atomic<uint32_t> g_killCount{0};
static std::atomic<bool> g_inWvW{false};
static std::atomic<bool> g_inSquad{false};
static std::mutex g_fileMutex;
static std::mutex g_squadMutex;
static uintptr_t g_selfId = 0;
static std::unordered_set<std::string> g_squadMembers;

// Nexus API
static AddonAPI* g_api = nullptr;
static HMODULE g_hModule = nullptr;
static AddonDefinition g_addonDef = {};

// Settings
static char g_outputPath[512] = "addons/streamlink/killstreak.txt";
static char g_squadOutputPath[512] = "addons/streamlink/squad.txt";
static char g_settingsPath[512] = "";

// Forward declarations
static void AddonLoad(AddonAPI* aAPI);
static void AddonUnload();
static void OnCombatEvent(void* eventArgs);
static void OnSquadUpdate(void* eventArgs);
static void WriteKillcountToFile();
static void WriteSquadStatusToFile();
static void LoadSettings();
static std::string GetFullOutputPath();
static std::string GetSquadOutputPath();

///----------------------------------------------------------------------------------------------------
/// DllMain
///----------------------------------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    (void)lpReserved;  // Unused parameter
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            g_hModule = hModule;
            DisableThreadLibraryCalls(hModule);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

///----------------------------------------------------------------------------------------------------
/// GetAddonDef - Nexus entry point
///----------------------------------------------------------------------------------------------------
extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef()
{
    g_addonDef.Signature = ADDON_SIGNATURE;
    g_addonDef.APIVersion = NEXUS_API_VERSION;
    g_addonDef.Name = ADDON_NAME;
    g_addonDef.Version.Major = 2;
    g_addonDef.Version.Minor = 4;
    g_addonDef.Version.Build = 0;
    g_addonDef.Version.Revision = 0;
    g_addonDef.Author = "Bozo";
    g_addonDef.Description = "Tracks WvW killstreaks and writes to file for OBS integration.";
    g_addonDef.Load = AddonLoad;
    g_addonDef.Unload = AddonUnload;
    g_addonDef.Flags = EAddonFlags_None;
    g_addonDef.Provider = EUpdateProvider_GitHub;
    g_addonDef.UpdateLink = "https://github.com/Bozofriendly/nexus-streamlink";

    return &g_addonDef;
}

///----------------------------------------------------------------------------------------------------
/// GetFullOutputPath - Returns the full path to the output file
///----------------------------------------------------------------------------------------------------
static std::string GetFullOutputPath()
{
    if (!g_api) return g_outputPath;

    const char* gameDir = g_api->Paths_GetGameDirectory();
    if (!gameDir) return g_outputPath;

    std::string fullPath = gameDir;
    if (!fullPath.empty() && fullPath.back() != '\\' && fullPath.back() != '/')
    {
        fullPath += "\\";
    }
    fullPath += g_outputPath;
    return fullPath;
}

///----------------------------------------------------------------------------------------------------
/// GetSquadOutputPath - Returns the full path to the squad status file
///----------------------------------------------------------------------------------------------------
static std::string GetSquadOutputPath()
{
    if (!g_api) return g_squadOutputPath;

    const char* gameDir = g_api->Paths_GetGameDirectory();
    if (!gameDir) return g_squadOutputPath;

    std::string fullPath = gameDir;
    if (!fullPath.empty() && fullPath.back() != '\\' && fullPath.back() != '/')
    {
        fullPath += "\\";
    }
    fullPath += g_squadOutputPath;
    return fullPath;
}

///----------------------------------------------------------------------------------------------------
/// GetSettingsPath - Returns the path to the settings file
///----------------------------------------------------------------------------------------------------
static std::string GetSettingsPath()
{
    if (!g_api) return "";

    const char* addonDir = g_api->Paths_GetAddonDirectory("streamlink");
    if (!addonDir) return "";

    std::string path = addonDir;
    if (!path.empty() && path.back() != '\\' && path.back() != '/')
    {
        path += "\\";
    }
    path += "settings.txt";
    return path;
}

///----------------------------------------------------------------------------------------------------
/// LoadSettings - Load settings from file
///----------------------------------------------------------------------------------------------------
static void LoadSettings()
{
    std::string path = GetSettingsPath();
    if (path.empty()) return;

    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "r") == 0 && f)
    {
        char buffer[512];
        if (fgets(buffer, sizeof(buffer), f))
        {
            // Remove newline
            size_t len = strlen(buffer);
            if (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r'))
                buffer[len-1] = '\0';
            if (len > 1 && (buffer[len-2] == '\n' || buffer[len-2] == '\r'))
                buffer[len-2] = '\0';

            strncpy_s(g_outputPath, buffer, sizeof(g_outputPath) - 1);
        }
        fclose(f);
    }
}

///----------------------------------------------------------------------------------------------------
/// WriteKillcountToFile - Write current killstreak to output file
///----------------------------------------------------------------------------------------------------
static void WriteKillcountToFile()
{
    std::lock_guard<std::mutex> lock(g_fileMutex);

    std::string fullPath = GetFullOutputPath();

    // Ensure directory exists
    std::string dirPath = fullPath.substr(0, fullPath.find_last_of("\\/"));
    CreateDirectoryA(dirPath.c_str(), nullptr);

    FILE* f = nullptr;
    if (fopen_s(&f, fullPath.c_str(), "w") == 0 && f)
    {
        fprintf(f, "%u", g_killCount.load());
        fclose(f);
    }
}

///----------------------------------------------------------------------------------------------------
/// WriteSquadStatusToFile - Write current squad status (0 or 1) to output file
///----------------------------------------------------------------------------------------------------
static void WriteSquadStatusToFile()
{
    std::lock_guard<std::mutex> lock(g_fileMutex);

    std::string fullPath = GetSquadOutputPath();

    // Ensure directory exists
    std::string dirPath = fullPath.substr(0, fullPath.find_last_of("\\/"));
    CreateDirectoryA(dirPath.c_str(), nullptr);

    FILE* f = nullptr;
    if (fopen_s(&f, fullPath.c_str(), "w") == 0 && f)
    {
        fprintf(f, "%u", g_inSquad.load() ? 1 : 0);
        fclose(f);
    }
}

///----------------------------------------------------------------------------------------------------
/// OnSquadUpdate - Handle Unofficial Extras squad update events via Nexus
///----------------------------------------------------------------------------------------------------
static void OnSquadUpdate(void* eventArgs)
{
    if (!eventArgs) return;

    EvSquadUpdate* data = static_cast<EvSquadUpdate*>(eventArgs);
    if (!data->UpdatedUsers || data->UpdatedUsersCount == 0) return;

    std::lock_guard<std::mutex> lock(g_squadMutex);

    for (uint64_t i = 0; i < data->UpdatedUsersCount; i++)
    {
        const UnofficialExtras::UserInfo& user = data->UpdatedUsers[i];
        if (!user.AccountName) continue;

        std::string accountName(user.AccountName);

        if (user.Role != UnofficialExtras::UserRole::None &&
            user.Role != UnofficialExtras::UserRole::Invalid)
        {
            g_squadMembers.insert(accountName);
        }
        else
        {
            g_squadMembers.erase(accountName);
        }
    }

    bool wasInSquad = g_inSquad.load();
    bool nowInSquad = !g_squadMembers.empty();

    if (wasInSquad != nowInSquad)
    {
        g_inSquad.store(nowInSquad);
        WriteSquadStatusToFile();
    }
}

///----------------------------------------------------------------------------------------------------
/// OnCombatEvent - Handle ArcDPS combat events via Nexus
///----------------------------------------------------------------------------------------------------
static void OnCombatEvent(void* eventArgs)
{
    if (!eventArgs) return;

    EvCombatData* data = static_cast<EvCombatData*>(eventArgs);
    ArcDPS::CombatEvent* ev = data->ev;
    ArcDPS::AgentShort* src = data->src;
    ArcDPS::AgentShort* dst = data->dst;

    // Handle null event (agent tracking)
    if (!ev)
    {
        if (src && src->IsSelf)
        {
            g_selfId = src->ID;
        }
        return;
    }

    // Handle state changes
    if (ev->IsStatechange)
    {
        switch (ev->IsStatechange)
        {
            case ArcDPS::CBTS_ENTERCOMBAT:
                break;

            case ArcDPS::CBTS_CHANGEDEAD:
                // Check if WE died (WvW only - don't let PvE deaths reset streak)
                if (g_inWvW.load() && src && (src->IsSelf || (g_selfId != 0 && src->ID == g_selfId)))
                {
                    g_killCount.store(0);
                    WriteKillcountToFile();
                }
                break;

            case ArcDPS::CBTS_MAPID:
                {
                    uint32_t mapId = static_cast<uint32_t>(ev->SourceAgent);
                    bool isWvWMap = (mapId == 38) ||                    // Eternal Battlegrounds
                                   (mapId >= 94 && mapId <= 96) ||     // Borderlands
                                   (mapId == 1099) ||                   // Obsidian Sanctum
                                   (mapId == 1143) ||                   // Edge of the Mists
                                   (mapId == 968) ||                    // Armistice Bastion
                                   (mapId == 1206) ||                   // Alpine Borderlands
                                   (mapId == 1323);                     // Desert Borderlands variant

                    bool wasInWvW = g_inWvW.load();
                    g_inWvW.store(isWvWMap);

                    if (!wasInWvW && isWvWMap)
                    {
                        g_killCount.store(0);
                        WriteKillcountToFile();
                    }
                    else if (wasInWvW && !isWvWMap)
                    {
                        g_killCount.store(0);
                        WriteKillcountToFile();
                    }
                }
                break;
        }
        return;
    }

    // Check for killing blow (WvW only)
    if (ev->Result == ArcDPS::CBTR_KILLINGBLOW && g_inWvW.load())
    {
        // Check if WE dealt the killing blow
        bool isSelfKill = false;
        if (src)
        {
            if (src->IsSelf)
            {
                isSelfKill = true;
            }
            else if (g_selfId != 0 && src->ID == g_selfId)
            {
                isSelfKill = true;
            }
        }

        if (isSelfKill)
        {
            uint32_t newCount = g_killCount.fetch_add(1) + 1;
            WriteKillcountToFile();

            // Send alert for milestones
            if (g_api && (newCount == 5 || newCount == 10 || newCount == 25 || newCount == 50 || newCount == 100))
            {
                char alertMsg[64];
                snprintf(alertMsg, sizeof(alertMsg), "Killstreak: %u!", newCount);
                g_api->GUI_SendAlert(alertMsg);
            }
        }

        // Check if WE were killed (we are the target of a killing blow)
        // Note: Stomp deaths don't trigger KILLINGBLOW, only direct deaths do
        bool isSelfDeath = false;
        if (dst)
        {
            if (dst->IsSelf)
            {
                isSelfDeath = true;
            }
            else if (g_selfId != 0 && dst->ID == g_selfId)
            {
                isSelfDeath = true;
            }
        }

        if (isSelfDeath)
        {
            g_killCount.store(0);
            WriteKillcountToFile();
        }
    }
}

///----------------------------------------------------------------------------------------------------
/// AddonLoad - Called when addon is loaded
///----------------------------------------------------------------------------------------------------
static void AddonLoad(AddonAPI* aAPI)
{
    g_api = aAPI;

    // Load settings
    LoadSettings();

    // Subscribe to ArcDPS combat events
    aAPI->Events_Subscribe(EV_ARCDPS_COMBATEVENT_LOCAL_RAW, OnCombatEvent);

    // Subscribe to Unofficial Extras squad events (requires ArcdpsIntegration addon)
    aAPI->Events_Subscribe(EV_UNOFFICIAL_EXTRAS_SQUAD_UPDATE, OnSquadUpdate);

    // Initialize output files
    g_killCount.store(0);
    g_inSquad.store(false);
    WriteKillcountToFile();
    WriteSquadStatusToFile();

    aAPI->Log(ELogLevel_INFO, ADDON_NAME, "Addon loaded successfully.");
}

///----------------------------------------------------------------------------------------------------
/// AddonUnload - Called when addon is unloaded
///----------------------------------------------------------------------------------------------------
static void AddonUnload()
{
    if (g_api)
    {
        // Unsubscribe from events
        g_api->Events_Unsubscribe(EV_ARCDPS_COMBATEVENT_LOCAL_RAW, OnCombatEvent);
        g_api->Events_Unsubscribe(EV_UNOFFICIAL_EXTRAS_SQUAD_UPDATE, OnSquadUpdate);

        g_api->Log(ELogLevel_INFO, ADDON_NAME, "Addon unloaded.");
    }

    // Final file writes
    WriteKillcountToFile();
    WriteSquadStatusToFile();

    // Clear squad members
    {
        std::lock_guard<std::mutex> lock(g_squadMutex);
        g_squadMembers.clear();
    }

    g_api = nullptr;
}
