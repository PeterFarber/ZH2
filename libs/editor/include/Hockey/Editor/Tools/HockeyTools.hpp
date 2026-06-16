#pragma once

#include "Hockey/Editor/Tools/EditorTool.hpp"

namespace Hockey {

// Instant placement tools that create hockey scene-marker entities/components.
// They contain no gameplay logic; they only author scene data (markers, spawns,
// goals, etc.). Each is undoable via the SpawnEntities command.

class HockeyRinkTool : public EditorTool {
public:
    const char* Name() const override {
        return "Hockey Rink";
    }
    const char* Category() const override {
        return "Hockey";
    }
    bool IsInstant() const override {
        return true;
    }
    void OnSelected(EditorContext& context) override;
};

class HockeySpawnTool : public EditorTool {
public:
    const char* Name() const override {
        return "Hockey Spawns";
    }
    const char* Category() const override {
        return "Hockey";
    }
    bool IsInstant() const override {
        return true;
    }
    void OnSelected(EditorContext& context) override;
};

class HockeyPlayerTool : public EditorTool {
public:
    const char* Name() const override {
        return "Hockey Players";
    }
    const char* Category() const override {
        return "Hockey";
    }
    bool IsInstant() const override {
        return true;
    }
    void OnSelected(EditorContext& context) override;
};

class HockeyGoalTool : public EditorTool {
public:
    const char* Name() const override {
        return "Hockey Goals";
    }
    const char* Category() const override {
        return "Hockey";
    }
    bool IsInstant() const override {
        return true;
    }
    void OnSelected(EditorContext& context) override;
};

class HockeyPuckTool : public EditorTool {
public:
    const char* Name() const override {
        return "Hockey Puck";
    }
    const char* Category() const override {
        return "Hockey";
    }
    bool IsInstant() const override {
        return true;
    }
    void OnSelected(EditorContext& context) override;
};

class HockeyFaceoffTool : public EditorTool {
public:
    const char* Name() const override {
        return "Hockey Faceoff Spots";
    }
    const char* Category() const override {
        return "Hockey";
    }
    bool IsInstant() const override {
        return true;
    }
    void OnSelected(EditorContext& context) override;
};

class HockeyCameraRigTool : public EditorTool {
public:
    const char* Name() const override {
        return "Hockey Camera Rig";
    }
    const char* Category() const override {
        return "Hockey";
    }
    bool IsInstant() const override {
        return true;
    }
    void OnSelected(EditorContext& context) override;
};

} // namespace Hockey
