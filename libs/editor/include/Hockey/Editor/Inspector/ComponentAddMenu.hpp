#pragma once

namespace Hockey {

class EditorContext;
class Entity;

// "Add Component" button + searchable popup generated from the ComponentRegistry.
// Lists addable components not already present on the entity, filtered by a
// search box; adding one marks the scene dirty.
class ComponentAddMenu {
public:
    void Draw(EditorContext& context, Entity& entity);

private:
    char m_Search[128] = {};
};

} // namespace Hockey
