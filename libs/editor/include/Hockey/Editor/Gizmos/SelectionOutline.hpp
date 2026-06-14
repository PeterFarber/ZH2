#pragma once

namespace Hockey {

class DebugDraw;
class Scene;
class Selection;

// Draws an oriented unit-box highlight around each selected entity using the
// renderer's debug lines. The primary selection is drawn brighter. The box
// matches the unit cube used by viewport picking, so the highlight lines up with
// what is clickable.
namespace SelectionOutline {

void Submit(DebugDraw& debug, Scene& scene, const Selection& selection);

} // namespace SelectionOutline

} // namespace Hockey
