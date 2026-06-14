#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Hockey/Editor/Logging/EditorConsoleSink.hpp"
#include "Hockey/Editor/Panel.hpp"

namespace Hockey {

// Editor log/console output backed by EditorConsoleStore. Supports per-severity
// filtering, text search, clear, copy, auto-scroll and collapsing of repeated
// messages. Entries are cached and only re-copied from the store when its
// revision changes.
class ConsolePanel : public Panel {
public:
    ConsolePanel();
    void OnImGui(EditorContext& context) override;

private:
    void DrawToolbar();
    bool PassesFilter(const EditorLogEntry& entry) const;

    std::vector<EditorLogEntry> m_Cached;
    std::size_t m_CachedRevision = static_cast<std::size_t>(-1);

    bool m_ShowTrace = false;
    bool m_ShowInfo = true;
    bool m_ShowWarn = true;
    bool m_ShowError = true;
    bool m_AutoScroll = true;
    bool m_Collapse = false;
    char m_Search[128] = {};
    std::string m_SelectedLine;
};

} // namespace Hockey
