#pragma once

namespace Hockey {

class Scene;
class Entity;

// Free hierarchy helpers. Scene's hierarchy methods are implemented in
// Hierarchy.cpp and delegate to these where it keeps the logic reusable.
namespace Hierarchy {

// True if 'child' lives somewhere in the subtree rooted at 'possibleParent'.
bool IsDescendantOf(const Scene& scene, Entity child, Entity possibleParent);

} // namespace Hierarchy

} // namespace Hockey
