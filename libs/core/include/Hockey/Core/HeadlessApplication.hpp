#pragma once
#include "Hockey/Core/Application.hpp"
#include "Hockey/Core/FixedTimestep.hpp"
namespace Hockey { class HeadlessApplication : public Application { public: using Application::Application; int Run() override; protected: void SetTickRate(double tickRate); virtual void OnFixedUpdate(double fixedDeltaSeconds)=0; private: FixedTimestep m_Timestep{60.0}; }; }
