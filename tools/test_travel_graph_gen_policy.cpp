#include "../ai/playerbot/policy/travel/TravelGraphGenPolicy.h"

#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <vector>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

namespace
{
    // Minimal faithful mirror of TravelNode + TravelNodeMap::removeLowNodes /
    // removeNode to prove under ASan / _GLIBCXX_DEBUG that overlapping directed
    // reachability components (< 5 nodes) and link-only / path-only edges are
    // pruned without use-after-free or double delete.
    struct SimPath
    {
        std::vector<int> waypoints{ 1, 2, 3, 4 };
    };

    struct SimNode
    {
        int id = 0;
        bool overworld = true;
        bool important = true;
        std::unordered_map<SimNode*, SimPath> paths;
        std::unordered_map<SimNode*, SimPath*> links;
        std::unordered_map<SimNode*, bool> routes;

        void setPathTo(SimNode* target, bool isLink = true)
        {
            if (target == this)
                return;
            paths[target] = SimPath{};
            if (isLink)
                links[target] = &paths[target];
        }

        std::vector<SimNode*> getNodeMap(bool importantOnly = false)
        {
            std::vector<SimNode*> open{ this };
            std::vector<SimNode*> close;
            for (std::size_t i = 0; i < open.size(); ++i)
            {
                SimNode* cur = open[i];
                if (!importantOnly || cur->important)
                    close.push_back(cur);
                for (auto& kv : cur->links)
                {
                    SimNode* next = kv.first;
                    if (std::find(open.begin(), open.end(), next) == open.end())
                        open.push_back(next);
                }
            }
            return close;
        }
    };

    struct SimMap
    {
        std::vector<SimNode*> nodes;
        int deleteCount = 0;

        ~SimMap()
        {
            for (SimNode* n : nodes)
                delete n;
        }

        SimNode* add(int id)
        {
            SimNode* n = new SimNode();
            n->id = id;
            nodes.push_back(n);
            return n;
        }

        void removeLinkTo(SimNode* self, SimNode* target, bool removePaths)
        {
            if (target)
            {
                if (removePaths)
                    self->paths.erase(target);
                self->links.erase(target);
                self->routes.erase(target);
            }
            else
            {
                for (SimNode* other : nodes)
                {
                    if (other == self)
                        continue;
                    removeLinkTo(other, self, removePaths);
                }
                self->links.clear();
                self->paths.clear();
                self->routes.clear();
            }
        }

        void removeNode(SimNode* node)
        {
            if (!node || std::find(nodes.begin(), nodes.end(), node) == nodes.end())
                return;
            removeLinkTo(node, nullptr, true);
            for (SimNode*& tnode : nodes)
            {
                if (tnode == node)
                {
                    delete tnode;
                    tnode = nullptr;
                    ++deleteCount;
                }
            }
            nodes.erase(std::remove(nodes.begin(), nodes.end(), nullptr), nodes.end());
        }

        void removeLowNodes()
        {
            std::vector<SimNode*> goodNodes;
            std::vector<SimNode*> remNodes;
            for (SimNode* node : nodes)
            {
                if (!node->overworld)
                    continue;
                if (std::find(goodNodes.begin(), goodNodes.end(), node) != goodNodes.end())
                    continue;
                if (std::find(remNodes.begin(), remNodes.end(), node) != remNodes.end())
                    continue;
                std::vector<SimNode*> comp = node->getNodeMap(true);
                if (comp.size() < 5)
                    ai::TravelGenAppendUniqueRemovals(remNodes, comp);
                else
                    goodNodes.insert(goodNodes.end(), comp.begin(), comp.end());
            }
            for (SimNode* node : remNodes)
                removeNode(node);
        }
    };
}

int main()
{
    std::cout << "Starting TortoiseBots TravelGraphGenPolicy tests...\n";

    // Constants match the sparse first-boot density rules.
    CHECK(ai::kTravelGenMaxLinksPerNode == 8);
    CHECK(ai::kTravelGenLinkRadius == 400.0f);
    CHECK(ai::kTravelGenExistingRouteMaxRatio == 1.5f);

    // Nearest-poly snap ceiling: up to +3 yd above query Z is allowed, higher is rejected;
    // snapping below query Z is allowed within search extents.
    CHECK(ai::TravelGenPolySnapAllowed(10.0f, 10.0f));
    CHECK(ai::TravelGenPolySnapAllowed(10.0f, 13.0f));
    CHECK(!ai::TravelGenPolySnapAllowed(10.0f, 13.1f));
    CHECK(ai::TravelGenPolySnapAllowed(10.0f, 2.0f));

    // Existing route shortcut: < 1.5x direct distance skips Detour query.
    CHECK(ai::TravelGenExistingRouteSufficient(140.0f, 100.0f));
    CHECK(!ai::TravelGenExistingRouteSufficient(150.0f, 100.0f));
    CHECK(!ai::TravelGenExistingRouteSufficient(200.0f, 100.0f));
    CHECK(!ai::TravelGenExistingRouteSufficient(0.0f, 100.0f));

    // Two-point jump gate: 2 points over 5 yd apart is rejected; 3+ points or <= 5 yd passes.
    CHECK(ai::TravelGenIsTwoPointJump(2, 12.0f));
    CHECK(!ai::TravelGenIsTwoPointJump(2, 5.0f));
    CHECK(!ai::TravelGenIsTwoPointJump(3, 50.0f));

    // Endpoint cliff step rejection: vertical > 10 yd with v/h > 2 is rejected.
    CHECK(ai::TravelGenEndpointStepTooSteep(12.0f, 4.0f));
    CHECK(ai::TravelGenEndpointStepTooSteep(11.0f, 0.0f));
    CHECK(!ai::TravelGenEndpointStepTooSteep(12.0f, 8.0f));
    CHECK(!ai::TravelGenEndpointStepTooSteep(9.5f, 1.0f));

    // Taxi DBC leg guard: bounds pathId, requires legs, same overworld map, distinct nodes.
    CHECK(ai::TravelGenTaxiPathEligible(10, 500, 12, 0, 0, true, true, false));
    CHECK(!ai::TravelGenTaxiPathEligible(500, 500, 12, 0, 0, true, true, false));
    CHECK(!ai::TravelGenTaxiPathEligible(10, 500, 0, 0, 0, true, true, false));
    CHECK(!ai::TravelGenTaxiPathEligible(10, 500, 12, 0, 1, true, true, false));
    CHECK(!ai::TravelGenTaxiPathEligible(10, 500, 12, 36, 36, false, false, false));
    CHECK(!ai::TravelGenTaxiPathEligible(10, 500, 12, 0, 0, true, true, true));

    // Grid bounds and MMapManager::loadMap axis transpose (Stormwind mGridPair(48, 31) -> loadMap(31, 48)).
    CHECK(ai::TravelGenGridInBounds(0, 0, 64));
    CHECK(ai::TravelGenGridInBounds(48, 31, 64));
    CHECK(ai::TravelGenGridInBounds(63, 63, 64));
    CHECK(!ai::TravelGenGridInBounds(-1, 31, 64));
    CHECK(!ai::TravelGenGridInBounds(48, 64, 64));
    CHECK((ai::TravelGenMmapLoadPair(48, 31) == std::pair<int, int>{ 31, 48 }));

    // Directed low-node pruning: overlapping small components (A -> C and B -> C)
    // must not schedule C for deletion twice, and a surviving component (G1..G5)
    // that had a one-way link into C must have its link/path/route to C erased.
    {
        std::vector<int> rem;
        ai::TravelGenAppendUniqueRemovals(rem, { 1, 3 });
        ai::TravelGenAppendUniqueRemovals(rem, { 2, 3 });
        CHECK((rem == std::vector<int>{ 1, 3, 2 }));

        SimMap map;
        SimNode* a = map.add(1);
        SimNode* b = map.add(2);
        SimNode* c = map.add(3);
        // Directed links: A -> C, B -> C (no reverse link from C).
        // Without deduplication, A's GetNodeMap returns {A, C} (< 5) and B's
        // GetNodeMap returns {B, C} (< 5), pushing C into remNodes twice and
        // causing use-after-free + double-free on C.
        a->setPathTo(c, true);
        b->setPathTo(c, true);

        // A 5-node connected component that survives pruning, with a stale
        // route entry and link pointing to C.
        SimNode* g1 = map.add(10);
        SimNode* g2 = map.add(11);
        SimNode* g3 = map.add(12);
        SimNode* g4 = map.add(13);
        SimNode* g5 = map.add(14);
        g1->setPathTo(g2); g2->setPathTo(g3); g3->setPathTo(g4); g4->setPathTo(g5); g5->setPathTo(g1);
        g1->routes[c] = true;

        map.removeLowNodes();
        CHECK(map.deleteCount == 3);
        CHECK(map.nodes.size() == 5);
        CHECK(g1->routes.find(c) == g1->routes.end());
    }

    std::cout << "travel graph gen policy: OK\n";
    return 0;
}
