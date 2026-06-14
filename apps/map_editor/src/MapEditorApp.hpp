#pragma once
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/WindowedApplication.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorApp.hpp"
#include "Hockey/Renderer/Renderer.hpp"

// Thin host shell for the Unity-style map editor. It owns the OS/window, the
// renderer, the active scene and config, then hands references to EditorApp
// (libs/editor), which owns all editor UI/state. This app links hockey_editor;
// the game client and dedicated server must not.
class MapEditorApp final : public Hockey::WindowedApplication {
public:
    using WindowedApplication::WindowedApplication;

protected:
    bool OnInit() override;
    void OnShutdown() override;
    void OnUpdate(float deltaTime) override;

private:
    void LoadStartupScene();

    Hockey::Config m_Config;
    Hockey::Scene m_Scene{"Untitled Scene"};

    Hockey::Renderer m_Renderer;
    Hockey::AssetManager m_AssetManager;
    Hockey::EditorApp m_Editor;
    bool m_Ready = false;
    bool m_PhysicsReady = false;

    // Optional headless-ish smoke mode: --frames N renders N frames then quits.
    int m_FrameLimit = 0;
    int m_FrameCount = 0;
};
