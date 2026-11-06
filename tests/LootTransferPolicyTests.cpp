#include "../src/plugin/sync/LootTransferPolicy.h"
#include "../src/plugin/sync/TransferAck.h"
#include "../src/plugin/sync/TransferPlan.h"
#include <stdio.h>

#define CHECK(x) do { if (!(x)) { printf("FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)

int main() {
    // A recovered NPC must remain an authoritative container long enough for
    // a delayed drag/ACK, including when the millisecond timer wraps.
    CHECK(coop::retainBodyAuthority(31000, 1000, false));
    CHECK(!coop::retainBodyAuthority(31001, 1000, false));
    CHECK(coop::retainBodyAuthority(90000, 1000, true));
    CHECK(coop::retainBodyAuthority(5, 0xfffffff0UL, false));
    CHECK(!coop::transferSourceMayFabricate(true, true));
    CHECK(!coop::transferSourceMayFabricate(false, true));
    CHECK(!coop::transferSourceMayFabricate(true, false));
    CHECK(coop::transferSourceMayFabricate(false, false));
    CHECK(!coop::transferLatchExpired(true, 0, 100));
    CHECK(!coop::transferLatchExpired(true, 0, 1000000));
    CHECK(coop::transferLatchExpired(false, 10000, 10001));
    CHECK(!coop::transferLatchExpired(false, 10000, 10000));

    // Host-side census losses need to participate in drag matching too. Both
    // corpse -> host and corpse -> join captures pair with their recipient,
    // and unrelated unpaired item losses survive the exact-unit baseline step.
    typedef std::pair<int, int> Item;
    typedef coop::TransferMatch<int, Item> Match;
    std::map<int, std::map<Item, int> > delta, baseline;
    Item sword(11, 1), food(12, 2);
    delta[1][sword] = -1; delta[2][sword] = 1; delta[1][food] = -3;
    std::vector<Match> matches;
    coop::planTransferMatches(delta, matches);
    CHECK(matches.size() == 1);
    baseline[1][sword] = 1; baseline[1][food] = 3;
    coop::advanceTransferBaseline(baseline, matches[0]);
    CHECK(baseline[1][sword] == 0 && baseline[2][sword] == 1);
    CHECK(baseline[1][food] == 3);

    // Two optimistic looters compete for the same authoritative stack. Exercise
    // all stack/request sizes and both arrival orders: short sources refuse
    // rather than mint. A rejected looter returns exactly the refused units.
    int cases = 0;
    for (int stock = 0; stock <= 20; ++stock) {
        for (int a = 1; a <= 25; ++a) for (int b = 1; b <= 25; ++b) {
            for (int order = 0; order < 2; ++order) {
                int remaining = stock, won[2] = {0, 0};
                int request[2] = {a, b};
                for (int turn = 0; turn < 2; ++turn) {
                    int who = (turn + order) % 2;
                    int moved = remaining < request[who] ? remaining : request[who];
                    int shortage = request[who] - moved;
                    int fabricated = coop::transferSourceMayFabricate(true, true) ? shortage : 0;
                    won[who] = moved + fabricated; remaining -= moved;
                    coop::TransferAckAmounts ack = coop::transferAckAmounts(request[who], won[who]);
                    CHECK(request[who] - ack.refused == won[who]);
                    CHECK(ack.accepted == won[who]);
                    // While waiting for a verdict/remainder snapshot, even a
                    // very old full-body snapshot must remain suppressed.
                    CHECK(!coop::transferLatchExpired(true, 0, 20000));
                }
                CHECK(remaining + won[0] + won[1] == stock);
                CHECK(won[0] <= a && won[1] <= b);
                ++cases;
            }
        }
    }
    printf("LootTransferPolicyTests: %d competing-loot cases passed\n", cases);
    return 0;
}
