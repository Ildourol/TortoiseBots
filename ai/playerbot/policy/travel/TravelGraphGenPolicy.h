#pragma once

// Pure decision rules for first-boot travel graph generation (TravelNode.cpp,
// WorldPosition.cpp). Kept in a standalone header so unit tests can verify
// sparse walk-link gating, detour endpoint slope/jump checks, taxi DBC leg
// guards, and per-tile navmesh coordinate matching without a server binary.
//
// Tested in tools/test_travel_graph_gen_policy.cpp.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace ai
{
    // First-boot density limits: each node links to at most 8 nearest
    // neighbours within 400 yd, and skips a Detour query when an existing
    // graph route is already within 1.5x of the direct distance.
    constexpr std::uint32_t kTravelGenMaxLinksPerNode = 8;
    constexpr float kTravelGenLinkRadius = 400.0f;
    constexpr float kTravelGenExistingRouteMaxRatio = 1.5f;

    // Detour FindWalkPoly ceiling: reject nearest-poly snaps more than 3 yd
    // above the query point (prevents snapping onto bridges/roofs above).
    constexpr float kTravelGenMaxPolySnapAboveYd = 3.0f;

    inline bool TravelGenPolySnapAllowed(float queryZ, float polyZ)
    {
        return polyZ <= queryZ + kTravelGenMaxPolySnapAboveYd;
    }

    // True when an existing route between two nodes is short enough that
    // probing a direct walk link is unnecessary.
    inline bool TravelGenExistingRouteSufficient(float routeDistance, float directDistance)
    {
        return routeDistance > 0.0f && directDistance > 0.0f &&
            routeDistance < directDistance * kTravelGenExistingRouteMaxRatio;
    }

    // True when a 2-point walk path is an unpathed jump (> 5 yd straight hop
    // with no intermediate navmesh vertices) and must be rejected.
    inline bool TravelGenIsTwoPointJump(std::size_t pathPoints, float startEndDistance)
    {
        return pathPoints == 2 && startEndDistance > 5.0f;
    }

    // True when the first or last leg of a multi-point walk path climbs or
    // drops more than 10 yd vertically at a vertical-to-horizontal ratio > 2.
    inline bool TravelGenEndpointStepTooSteep(float verticalDelta, float horizontalDist)
    {
        return verticalDelta > 10.0f && (horizontalDist == 0.0f || verticalDelta / horizontalDist > 2.0f);
    }

    // True when a taxi DBC path between two endpoints is eligible for the
    // travel graph: valid path index, non-empty node list, same overworld map
    // (0, 1, or 609), and distinct start/end nodes.
    inline bool TravelGenTaxiPathEligible(std::uint32_t pathId, std::size_t tableSize,
        std::size_t legCount, std::uint32_t startMapId, std::uint32_t endMapId,
        bool startOverworld, bool endOverworld, bool sameNode)
    {
        if (pathId >= tableSize || legCount == 0)
            return false;
        if (startMapId != endMapId)
            return false;
        if (!startOverworld || !endOverworld)
            return false;
        if (sameNode)
            return false;
        return true;
    }

    // Bounds check for 64x64 WoW grid indices before probing or loading tiles.
    inline bool TravelGenGridInBounds(int gridX, int gridY, int maxGrids)
    {
        return gridX >= 0 && gridX < maxGrids && gridY >= 0 && gridY < maxGrids;
    }

    // MMapManager::loadMap(mapId, gx, gy) expects core GridMap's axis order
    // (32 - worldY/grid, 32 - worldX/grid), which is the transpose of
    // WorldPosition::getmGridPair() (32 - worldX/grid, 32 - worldY/grid).
    inline std::pair<int, int> TravelGenMmapLoadPair(int mGridX, int mGridY)
    {
        return { mGridY, mGridX };
    }

    // Directed reachability in removeLowNodes: when node A reaches a small
    // cluster {A, C} (< 5 nodes) and node B -> C reaches the same {C} (< 5
    // nodes), C appears in both GetNodeMap(true) lists. Deduplicating the
    // removal list prevents removeNode(C) from running twice (use-after-free
    // and double delete on C).
    template <typename NodePtr>
    inline void TravelGenAppendUniqueRemovals(std::vector<NodePtr>& remNodes,
        std::vector<NodePtr> const& component)
    {
        for (NodePtr const& reached : component)
            if (std::find(remNodes.begin(), remNodes.end(), reached) == remNodes.end())
                remNodes.push_back(reached);
    }
}
