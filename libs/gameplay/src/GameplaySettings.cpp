#include "Hockey/Gameplay/GameplaySettings.hpp"

namespace Hockey {

GameplaySettings LoadGameplaySettings(const Config& config) {
    GameplaySettings settings;
    settings.enabled = config.GetBool("gameplay.enabled", settings.enabled);
    settings.targetPlayerCount =
        static_cast<uint32_t>(config.GetInt("gameplay.target_player_count", static_cast<int>(settings.targetPlayerCount)));
    settings.skatersPerTeam =
        static_cast<uint32_t>(config.GetInt("gameplay.skaters_per_team", static_cast<int>(settings.skatersPerTeam)));
    settings.goaliesPerTeam =
        static_cast<uint32_t>(config.GetInt("gameplay.goalies_per_team", static_cast<int>(settings.goaliesPerTeam)));
    settings.fixedDeltaSeconds = static_cast<float>(config.GetDouble("gameplay.fixed_delta_seconds", settings.fixedDeltaSeconds));
    settings.periodLengthSeconds =
        static_cast<float>(config.GetDouble("gameplay.period_length_seconds", settings.periodLengthSeconds));
    settings.periodCount = static_cast<uint32_t>(config.GetInt("gameplay.period_count", static_cast<int>(settings.periodCount)));
    settings.pregameCountdownSeconds =
        static_cast<float>(config.GetDouble("gameplay.pregame_countdown_seconds", settings.pregameCountdownSeconds));
    settings.countdownBeepStartSeconds =
        static_cast<float>(config.GetDouble("gameplay.countdown_beep_start_seconds", settings.countdownBeepStartSeconds));
    settings.stopClockAfterGoal = config.GetBool("gameplay.stop_clock_after_goal", settings.stopClockAfterGoal);
    settings.autoFaceoffAfterGoal = config.GetBool("gameplay.auto_faceoff_after_goal", settings.autoFaceoffAfterGoal);
    settings.postGoalDelaySeconds =
        static_cast<float>(config.GetDouble("gameplay.post_goal_delay_seconds", settings.postGoalDelaySeconds));
    settings.allowBodyChecking = config.GetBool("gameplay.allow_body_checking", settings.allowBodyChecking);
    settings.allowManualGoalie = config.GetBool("gameplay.allow_manual_goalie", settings.allowManualGoalie);
    settings.allowOutOfPlay = config.GetBool("gameplay.allow_out_of_play", settings.allowOutOfPlay);
    settings.debugDrawGameplay = config.GetBool("gameplay.debug_draw_gameplay", settings.debugDrawGameplay);
    settings.logGameplayEvents = config.GetBool("gameplay.log_gameplay_events", settings.logGameplayEvents);
    return settings;
}

void SaveGameplaySettings(Config& config, const GameplaySettings& settings) {
    config.SetBool("gameplay.enabled", settings.enabled);
    config.SetInt("gameplay.target_player_count", static_cast<int>(settings.targetPlayerCount));
    config.SetInt("gameplay.skaters_per_team", static_cast<int>(settings.skatersPerTeam));
    config.SetInt("gameplay.goalies_per_team", static_cast<int>(settings.goaliesPerTeam));
    config.SetDouble("gameplay.fixed_delta_seconds", settings.fixedDeltaSeconds);
    config.SetDouble("gameplay.period_length_seconds", settings.periodLengthSeconds);
    config.SetInt("gameplay.period_count", static_cast<int>(settings.periodCount));
    config.SetDouble("gameplay.pregame_countdown_seconds", settings.pregameCountdownSeconds);
    config.SetDouble("gameplay.countdown_beep_start_seconds", settings.countdownBeepStartSeconds);
    config.SetBool("gameplay.stop_clock_after_goal", settings.stopClockAfterGoal);
    config.SetBool("gameplay.auto_faceoff_after_goal", settings.autoFaceoffAfterGoal);
    config.SetDouble("gameplay.post_goal_delay_seconds", settings.postGoalDelaySeconds);
    config.SetBool("gameplay.allow_body_checking", settings.allowBodyChecking);
    config.SetBool("gameplay.allow_manual_goalie", settings.allowManualGoalie);
    config.SetBool("gameplay.allow_out_of_play", settings.allowOutOfPlay);
    config.SetBool("gameplay.debug_draw_gameplay", settings.debugDrawGameplay);
    config.SetBool("gameplay.log_gameplay_events", settings.logGameplayEvents);
}

}
