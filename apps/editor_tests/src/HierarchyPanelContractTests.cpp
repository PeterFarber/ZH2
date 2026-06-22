#include "Test.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include "Hockey/Core/Paths.hpp"

namespace {

std::string ReadProjectFile(const char* relativePath) {
    std::ifstream in(Hockey::Paths::Get().root / relativePath, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool Contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

} // namespace

void RunHierarchyPanelContractTests() {
    HockeyTest::BeginSuite("HierarchyPanelContractTests");

    const std::string header = ReadProjectFile("libs/editor/include/Hockey/Editor/Panels/HierarchyPanel.hpp");
    const std::string source = ReadProjectFile("libs/editor/src/Panels/HierarchyPanel.cpp");

    HK_CHECK_MSG(Contains(header, "MoveEntity"), "Hierarchy panel has a deferred move operation");
    HK_CHECK_MSG(Contains(header, "m_PendingSiblingIndex"), "Hierarchy panel stores the requested sibling index");
    HK_CHECK_MSG(Contains(source, "EditorCommands::MoveEntity"),
                 "Hierarchy drag/drop uses the undoable move command");
    HK_CHECK_MSG(Contains(source, "GetSiblingIndex"), "Hierarchy drag/drop computes sibling insertion indexes");
    HK_CHECK_MSG(Contains(source, "GetItemRectMin") && Contains(source, "GetItemRectMax"),
                 "Hierarchy drag/drop splits rows into before/inside/after drop bands");
    HK_CHECK_MSG(Contains(source, "DrawRootDropTarget(Scene& scene)"),
                 "Hierarchy root drop target can move entities to root order");
    HK_CHECK_MSG(Contains(source, "DrawHierarchyDropPreview"),
                 "Hierarchy drag/drop draws a custom visual preview");
    HK_CHECK_MSG(Contains(source, "ImGuiDragDropFlags_AcceptPeekOnly"),
                 "Hierarchy entity drops preview before delivery without ImGui's default target rectangle");
    HK_CHECK_MSG(Contains(source, "payload->IsDelivery()"),
                 "Hierarchy drag/drop only mutates the scene when the payload is delivered");
    HK_CHECK_MSG(Contains(source, "AddRectFilled") && Contains(source, "AddLine"),
                 "Hierarchy preview distinguishes inside-parent drops from sibling insertion lines");
}
