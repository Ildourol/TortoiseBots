#include "../runtime/ReplacedGear.h"

#include <cstdlib>
#include <iostream>

#define CHECK(x) do { \
    if (!(x)) { \
        std::cerr << "Assertion failed at line " << __LINE__ << ": " #x "\n"; \
        std::exit(1); \
    } \
} while (0)

using TortoiseBots::KeepsReplacedGear;

int main()
{
    std::cout << "Starting TortoiseBots replaced gear policy tests...\n";

    // A real player always keeps what they wore.
    CHECK(KeepsReplacedGear(false, true, false, false, false, false));
    CHECK(KeepsReplacedGear(false, false, false, false, false, false));

    // Only a masterless pool bot loses the piece.
    CHECK(!KeepsReplacedGear(true, true, false, false, false, false));
    std::cout << "  [PASS] only a masterless pool bot destroys replaced gear\n";

    // Player-owned bots keep it: own-account alt, a live player master,
    // hire ledger, claim registry, or sharing a group with a player.
    CHECK(KeepsReplacedGear(true, false, false, false, false, false)); // owned alt
    CHECK(KeepsReplacedGear(true, true, true, false, false, false));   // real player master
    CHECK(KeepsReplacedGear(true, true, false, true, false, false));   // hired
    CHECK(KeepsReplacedGear(true, true, false, false, true, false));   // claimed
    CHECK(KeepsReplacedGear(true, true, false, false, false, true));   // grouped with a player
    std::cout << "  [PASS] owned, hired, claimed and grouped bots keep it\n";

    std::cout << "All replaced gear policy checks PASSED!\n";
    return 0;
}
