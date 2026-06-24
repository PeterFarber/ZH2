#include "Test.hpp"

#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Selection.hpp"

using namespace Hockey;

void RunSelectionTests() {
    HockeyTest::BeginSuite("SelectionTests");

    const UUID a(1001);
    const UUID b(2002);
    const UUID c(3003);
    const UUID d(4004);
    const UUID e(5005);

    // --- select entity ------------------------------------------------------
    {
        Selection selection;
        selection.Select(a);
        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(1));
        HK_CHECK_MSG(selection.IsSelected(a), "selected entity reported selected");
        HK_CHECK_MSG(selection.Primary() == a, "single select sets primary");
    }

    // --- add entity ---------------------------------------------------------
    {
        Selection selection;
        selection.Add(a);
        selection.Add(b);
        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(2));
        HK_CHECK_MSG(selection.IsSelected(a) && selection.IsSelected(b), "both entities selected");
        HK_CHECK_MSG(selection.Primary() == b, "add makes the new entity primary");
        // adding a duplicate does not grow the selection
        selection.Add(a);
        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(2));
    }

    // --- remove entity ------------------------------------------------------
    {
        Selection selection;
        selection.Add(a);
        selection.Add(b);
        selection.Remove(b);
        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(1));
        HK_CHECK_MSG(!selection.IsSelected(b), "removed entity not selected");
        HK_CHECK_MSG(selection.Primary() == a, "removing primary repairs primary");
    }

    // --- toggle entity ------------------------------------------------------
    {
        Selection selection;
        selection.Toggle(a);
        HK_CHECK_MSG(selection.IsSelected(a), "toggle selects when absent");
        selection.Toggle(a);
        HK_CHECK_MSG(!selection.IsSelected(a), "toggle deselects when present");
        HK_CHECK_MSG(selection.Empty(), "selection empty after toggling off");
    }

    // --- clear selection ----------------------------------------------------
    {
        Selection selection;
        selection.Add(a);
        selection.Add(b);
        selection.Clear();
        HK_CHECK_MSG(selection.Empty(), "clear empties selection");
        HK_CHECK_MSG(!selection.Primary().IsValid(), "clear invalidates primary");
    }

    // --- primary selection / single-select replaces --------------------------
    {
        Selection selection;
        selection.Add(a);
        selection.Add(b);
        selection.Select(c); // single-select replaces the whole selection
        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(1));
        HK_CHECK_MSG(selection.Primary() == c, "select replaces and sets primary");
        // invalid id clears
        selection.Select(UUID(0));
        HK_CHECK_MSG(selection.Empty(), "selecting an invalid id clears selection");
    }

    // --- invalid selection removed after scene deletion ---------------------
    {
        Scene scene("SelectionTestScene");
        Entity e1 = scene.CreateEntity("E1");
        Entity e2 = scene.CreateEntity("E2");
        const UUID id1 = e1.GetUUID();
        const UUID id2 = e2.GetUUID();

        Selection selection;
        selection.Add(id1);
        selection.Add(id2);
        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(2));

        scene.DestroyEntity(e2);
        selection.Validate(scene);

        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(1));
        HK_CHECK_MSG(selection.IsSelected(id1), "valid entity stays selected");
        HK_CHECK_MSG(!selection.IsSelected(id2), "deleted entity dropped from selection");
        HK_CHECK_MSG(selection.Primary() == id1, "primary repaired to a valid entity");
    }

    // --- select all entities in a scene -------------------------------------
    {
        Scene scene("SelectAllScene");
        Entity e1 = scene.CreateEntity("E1");
        Entity e2 = scene.CreateEntity("E2");
        Entity e3 = scene.CreateEntity("E3");

        Selection selection;
        selection.Add(e1.GetUUID()); // existing selection is replaced
        selection.SelectAll(scene);

        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(3));
        HK_CHECK_MSG(selection.IsSelected(e1.GetUUID()) && selection.IsSelected(e2.GetUUID()) &&
                         selection.IsSelected(e3.GetUUID()),
                     "select all selects every entity");
        HK_CHECK_MSG(selection.Primary().IsValid() && scene.ContainsUUID(selection.Primary()),
                     "select all leaves a valid primary");
    }

    // --- select all on an empty scene clears --------------------------------
    {
        Scene scene("EmptySelectAllScene");
        Selection selection;
        selection.Add(a);
        selection.SelectAll(scene);
        HK_CHECK_MSG(selection.Empty(), "select all on empty scene clears selection");
        HK_CHECK_MSG(!selection.Primary().IsValid(), "empty select all has no primary");
    }

    // --- range selection uses the current hierarchy row order ---------------
    {
        Selection selection;
        const std::vector<UUID> visibleRows{a, b, c, d, e};

        selection.Select(b);
        selection.SelectRange(visibleRows, d, false);

        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(3));
        HK_CHECK_MSG(selection.IsSelected(b) && selection.IsSelected(c) && selection.IsSelected(d),
                     "range selection includes every visible row between anchor and target");
        HK_CHECK_MSG(selection.Primary() == d, "range selection makes the clicked target primary");
        HK_CHECK_MSG(selection.RangeAnchor() == b, "range selection keeps the original anchor");

        selection.SelectRange(visibleRows, a, false);
        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(2));
        HK_CHECK_MSG(selection.IsSelected(a) && selection.IsSelected(b), "range selection supports reverse ranges");
        HK_CHECK_MSG(!selection.IsSelected(c), "non-additive range replaces the previous range");
    }

    // --- additive range selection preserves existing selected rows ----------
    {
        Selection selection;
        const std::vector<UUID> visibleRows{a, b, c, d, e};

        selection.Select(c);
        selection.Add(e);
        selection.SelectRange(visibleRows, a, true);

        HK_CHECK_MSG(selection.IsSelected(a) && selection.IsSelected(b) && selection.IsSelected(c),
                     "additive range adds the rows between anchor and target");
        HK_CHECK_MSG(selection.IsSelected(e), "additive range keeps an existing out-of-range selection");
        HK_CHECK_MSG(selection.Primary() == a, "additive range still updates primary to the target");
    }

    // --- range selection without a valid anchor falls back to target ---------
    {
        Selection selection;
        const std::vector<UUID> visibleRows{a, b, c};

        selection.SelectRange(visibleRows, c, false);

        HK_CHECK_EQ(selection.Count(), static_cast<std::size_t>(1));
        HK_CHECK_MSG(selection.IsSelected(c), "range selection without an anchor selects the target row");
        HK_CHECK_MSG(selection.RangeAnchor() == c, "target becomes anchor when no valid anchor exists");
    }
}
