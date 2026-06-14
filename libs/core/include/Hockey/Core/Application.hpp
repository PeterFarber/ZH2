#pragma once
#include "Hockey/Core/CommandLine.hpp"
namespace Hockey { class Application { public: explicit Application(CommandLine commandLine); virtual ~Application()=default; virtual int Run(); void RequestQuit(); bool IsRunning() const; protected: virtual bool OnInit()=0; virtual void OnShutdown()=0; CommandLine& GetCommandLine(); private: CommandLine m_CommandLine; bool m_Running=true; }; }
