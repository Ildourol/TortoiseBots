#include "../ai/playerbot/policy/travel/TravelGraphGenPolicy.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

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

    // Per-tile Detour coordinate swap: tx matches gridY, ty matches gridX.
    CHECK(ai::TravelGenTileCoordsMatch(42, 31, 31, 42));
    CHECK(!ai::TravelGenTileCoordsMatch(31, 42, 31, 42));

    std::cout << "travel graph gen policy: OK\n";
    return 0;
}
