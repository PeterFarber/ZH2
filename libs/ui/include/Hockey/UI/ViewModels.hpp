#pragma once

#include <string>
#include <vector>

namespace Hockey {

struct MainMenuViewModel {
    bool networkingAvailable = false;
    std::string networkingStatus = "Networking unavailable";
};

struct LobbyViewModel {
    struct Player {
        std::string name;
        std::string team;
        std::string role;
        bool ready = false;
    };

    std::vector<Player> players;
    bool canReady = false;
};

struct TeamSelectViewModel {
    std::string selectedTeam = "Home";
    std::string selectedRole = "Skater";
};

struct HudViewModel {
    int period = 1;
    int homeScore = 0;
    int awayScore = 0;
    std::string clockText = "20:00";
    std::string phaseLabel = "Faceoff";
    std::string localPlayerLabel;
    std::string possessionLabel = "Loose puck";
    float shotChargeRatio = 0.0f;
    std::string notificationText;
};

struct PauseMenuViewModel {
    bool canResume = true;
};

struct ScoreboardViewModel {
    int homeScore = 0;
    int awayScore = 0;
    std::string periodLabel = "P1";
};

struct NetworkStatsViewModel {
    bool available = false;
    float pingMs = 0.0f;
    float packetLossPercent = 0.0f;
    float incomingKbps = 0.0f;
    float outgoingKbps = 0.0f;
};

std::string FormatClockText(float secondsRemaining);
std::string FormatShotCharge(float ratio);

} // namespace Hockey
