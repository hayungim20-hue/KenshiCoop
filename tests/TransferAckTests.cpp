#include "../src/plugin/sync/TransferAck.h"
#include <stdio.h>
#define CHECK(x) do { if (!(x)) { printf("FAIL line %d: %s\n", __LINE__, #x); return 1; } } while(0)
int main() {
    int checks = 0;
    for (int qty = 1; qty <= 100; ++qty) {
        for (int applied = 0; applied <= qty; ++applied) {
            coop::TransferAckAmounts a = coop::transferAckAmounts(qty, applied);
            // The accepted part remains protected when ACK precedes state;
            // only the refused units are rolled back to the original owner.
            int sourceLatch = -qty + a.refused;
            int destinationLatch = qty - a.refused;
            CHECK(sourceLatch == -applied);
            CHECK(destinationLatch == applied);
            CHECK(a.refused + a.accepted == qty);
            // Both directions conserve units across a partial/failed trade.
            int source = 100 - qty + a.refused;
            int destination = qty - a.refused;
            CHECK(source + destination == 100);
            CHECK(destination == applied);
            ++checks;
        }
    }
    CHECK(coop::transferAckAmounts(3, 100).accepted == 3);
    CHECK(coop::transferAckAmounts(3, -1).refused == 3);
    printf("TransferAckTests: %d cases passed\n", checks);
    return 0;
}
