///----------------------------------------------------------------------------------------------------
/// Unofficial Extras API structures for Nexus squad events
/// Based on Krappa322/arcdps_unofficial_extras_releases Definitions.h
///----------------------------------------------------------------------------------------------------

#ifndef UNOFFICIAL_EXTRAS_H
#define UNOFFICIAL_EXTRAS_H

#include <cstdint>
#include <ctime>

namespace UnofficialExtras
{
    enum class UserRole : uint8_t
    {
        SquadLeader = 0,
        Lieutenant = 1,
        Member = 2,
        Invited = 3,
        Applied = 4,
        None = 5,
        Invalid = 6
    };

    enum class ChannelType : uint8_t
    {
        Party = 0,
        Squad = 1,
        Reserved = 2,
        Invalid = 3
    };

    struct UserInfo
    {
        const char* AccountName;
        __time64_t JoinTime;
        UserRole Role;
        uint8_t Subgroup;
        bool ReadyStatus;
        ChannelType GroupType;
        uint32_t _Unused2;
    };
}

/// Event payload for EV_UNOFFICIAL_EXTRAS_SQUAD_UPDATE
struct EvSquadUpdate
{
    UnofficialExtras::UserInfo* UpdatedUsers;
    uint64_t UpdatedUsersCount;
};

#define EV_UNOFFICIAL_EXTRAS_SQUAD_UPDATE "EV_UNOFFICIAL_EXTRAS_SQUAD_UPDATE"

#endif
