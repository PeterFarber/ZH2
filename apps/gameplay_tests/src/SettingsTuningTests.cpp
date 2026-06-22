#include "Test.hpp"

#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Gameplay/Tuning/TuningSerializer.hpp"

using namespace Hockey;

void RunSettingsTuningTests() {
    HockeyTest::BeginSuite("SettingsTuningTests");

    Config config;
    GameplaySettings defaults = LoadGameplaySettings(config);
    HK_CHECK(defaults.enabled);
    HK_CHECK_EQ(defaults.targetPlayerCount, 8u);
    HK_CHECK_EQ(defaults.skatersPerTeam, 3u);
    HK_CHECK_EQ(defaults.goaliesPerTeam, 1u);
    HK_CHECK_NEAR(defaults.fixedDeltaSeconds, 1.0f / 60.0f, 0.0001f);
    HK_CHECK_NEAR(defaults.periodLengthSeconds, 180.0f, 0.0001f);
    HK_CHECK_NEAR(defaults.pregameCountdownSeconds, 10.0f, 0.0001f);
    HK_CHECK_NEAR(defaults.countdownBeepStartSeconds, 4.0f, 0.0001f);

    GameplaySettings source;
    source.enabled = false;
    source.targetPlayerCount = 6;
    source.periodLengthSeconds = 120.0f;
    source.pregameCountdownSeconds = 7.0f;
    source.countdownBeepStartSeconds = 3.0f;
    source.logGameplayEvents = true;
    SaveGameplaySettings(config, source);
    GameplaySettings loaded = LoadGameplaySettings(config);
    HK_CHECK(!loaded.enabled);
    HK_CHECK_EQ(loaded.targetPlayerCount, 6u);
    HK_CHECK_NEAR(loaded.periodLengthSeconds, 120.0f, 0.0001f);
    HK_CHECK_NEAR(loaded.pregameCountdownSeconds, 7.0f, 0.0001f);
    HK_CHECK_NEAR(loaded.countdownBeepStartSeconds, 3.0f, 0.0001f);
    HK_CHECK(loaded.logGameplayEvents);

    const Result<GameplayTuning> tuning = TuningSerializer::Load(Paths::DataFile("gameplay/tuning.default.yaml"));
    HK_CHECK_MSG(static_cast<bool>(tuning), tuning.error);
    if (tuning) {
        HK_CHECK_NEAR(tuning.value.skater.maxSpeed, 9.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skater.boostImpulse, 7.5f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skater.boostCooldownSeconds, 1.25f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skater.slideStopDamping, 0.35f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skater.doubleStopWindowSeconds, 0.30f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skater.stealRadius, 1.5f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.skater.stealCooldownSeconds, 0.35f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalie.maxSpeed, 6.5f, 0.0001f);
        HK_CHECK_EQ(tuning.value.goalie.boostCharges, 2u);
        HK_CHECK_NEAR(tuning.value.goalie.boostRechargeSeconds, 4.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalie.boostImpulse, 8.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalie.shieldRadius, 2.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalie.shieldDurationSeconds, 1.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalie.shieldCooldownSeconds, 5.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.goalie.shieldReflectImpulse, 22.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.puck.floorY, 0.05f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.puck.possessionOffset.z, 1.1f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.shot.maxPower, 32.0f, 0.0001f);
        HK_CHECK_NEAR(tuning.value.shot.selfCollisionGraceSeconds, 0.20f, 0.0001f);

        const std::string text = TuningSerializer::Serialize(tuning.value);
        GameplayTuning roundTrip;
        HK_CHECK(TuningSerializer::Deserialize(text, roundTrip));
        HK_CHECK_NEAR(roundTrip.skater.boostImpulse, 7.5f, 0.0001f);
        HK_CHECK_EQ(roundTrip.goalie.boostCharges, 2u);
        HK_CHECK_NEAR(roundTrip.goalie.shieldReflectImpulse, 22.0f, 0.0001f);
        HK_CHECK_NEAR(roundTrip.puck.floorY, 0.05f, 0.0001f);
        HK_CHECK_NEAR(roundTrip.shot.selfCollisionGraceSeconds, 0.20f, 0.0001f);
        HK_CHECK_NEAR(roundTrip.pass.maxAssistAngleDegrees, 25.0f, 0.0001f);
        HK_CHECK_NEAR(roundTrip.check.radius, 1.25f, 0.0001f);
    }
}
