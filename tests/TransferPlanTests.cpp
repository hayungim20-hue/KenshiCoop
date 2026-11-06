#include "../src/plugin/sync/TransferPlan.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>

typedef std::pair<std::string, unsigned int> Item;
typedef std::map<Item, int> Counts;
typedef std::map<int, Counts> State;
typedef coop::TransferMatch<int, Item> Move;
typedef std::vector<Move> Moves;

static unsigned int checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { \
    std::printf("FAIL line %d: %s\n", __LINE__, #x); std::exit(1); } } while (0)

static Item item(const char* sid, unsigned int kind = 2) {
    return Item(std::string(sid), kind);
}

static int quantity(const State& s, int c, const Item& i) {
    State::const_iterator container = s.find(c);
    if (container == s.end()) return 0;
    Counts::const_iterator entry = container->second.find(i);
    return entry == container->second.end() ? 0 : entry->second;
}

static void apply(State& baseline, const Moves& moves) {
    for (unsigned int n = 0; n < moves.size(); ++n)
        coop::advanceTransferBaseline(baseline, moves[n]);
}

static void multipleKindsOneBurst() {
    const Item a = item("iron"), b = item("meat"), untouched = item("book");
    State delta, base;
    delta[0][a] = -2; delta[0][b] = -3;
    delta[1][a] = 2; delta[1][b] = 3;
    base[0][a] = 5; base[0][b] = 8; base[0][untouched] = 7;
    Moves m; coop::planTransferMatches(delta, m);
    CHECK(m.size() == 2);
    apply(base, m);
    CHECK(quantity(base, 0, a) == 3 && quantity(base, 1, a) == 2);
    CHECK(quantity(base, 0, b) == 5 && quantity(base, 1, b) == 3);
    CHECK(quantity(base, 0, untouched) == 7);
    CHECK(quantity(delta, 0, a) == -2); // planning must not erase observations
}

static void oneToManyAndRemainder() {
    const Item a = item("iron");
    State delta, base;
    delta[0][a] = -10; delta[1][a] = 3; delta[2][a] = 5;
    base[0][a] = 12;
    Moves m; coop::planTransferMatches(delta, m);
    CHECK(m.size() == 2);
    CHECK(m[0].qty == 3 && m[1].qty == 5);
    apply(base, m);
    CHECK(quantity(base, 0, a) == 4); // current=2; unmatched loss of 2 stays visible
    CHECK(quantity(base, 1, a) == 3 && quantity(base, 2, a) == 5);
}

static void manyToOneAndRemainder() {
    const Item a = item("iron");
    State delta, base;
    delta[0][a] = -2; delta[1][a] = -4; delta[2][a] = 9;
    base[0][a] = 2; base[1][a] = 4;
    Moves m; coop::planTransferMatches(delta, m);
    CHECK(m.size() == 2);
    apply(base, m);
    CHECK(quantity(base, 0, a) == 0 && quantity(base, 1, a) == 0);
    CHECK(quantity(base, 2, a) == 6); // current=9; unmatched gain of 3 stays visible
}

static void unpairedAndTypeIsolation() {
    const Item a = item("same", 0), b = item("same", 1), c = item("food");
    State delta, base;
    delta[0][a] = -1; delta[1][b] = 1;
    delta[0][c] = -1;
    base[0][a] = 2; base[0][c] = 3;
    const State old = base;
    Moves m; coop::planTransferMatches(delta, m);
    CHECK(m.empty());
    apply(base, m);
    CHECK(base == old);
}

static void bidirectionalKindsAndPartialBaseline() {
    const Item a = item("iron"), b = item("food"), c = item("unpaired");
    State delta, base;
    delta[0][a] = -5; delta[1][a] = 2;
    delta[0][b] = 4; delta[1][b] = -4;
    delta[0][c] = -7;
    base[0][a] = 8; base[1][b] = 4; base[0][c] = 9;
    Moves m; coop::planTransferMatches(delta, m);
    CHECK(m.size() == 2);
    apply(base, m);
    CHECK(quantity(base, 0, a) == 6 && quantity(base, 1, a) == 2);
    CHECK(quantity(base, 0, b) == 4 && quantity(base, 1, b) == 0);
    CHECK(quantity(base, 0, c) == 9);
    CHECK(base[1].find(b) == base[1].end()); // canonical zero-free source map
}

static void boundedConservation() {
    const Item a = item("iron"), b = item("food");
    for (unsigned int run = 0; run < 300; ++run) {
        State d, base;
        for (int c = 0; c < 5; ++c) {
            d[c][a] = (int)((run * 19 + c * 7) % 17) - 8;
            d[c][b] = (int)((run * 13 + c * 11) % 19) - 9;
            base[c][a] = 20; base[c][b] = 20;
        }
        Moves m; coop::planTransferMatches(d, m);
        State used;
        for (unsigned int n = 0; n < m.size(); ++n) {
            const Move& v = m[n];
            CHECK(v.qty > 0 && v.src != v.dst);
            CHECK(quantity(d, v.src, v.key) < 0 && quantity(d, v.dst, v.key) > 0);
            used[v.src][v.key] -= v.qty;
            used[v.dst][v.key] += v.qty;
        }
        for (State::const_iterator c = d.begin(); c != d.end(); ++c)
            for (Counts::const_iterator i = c->second.begin(); i != c->second.end(); ++i) {
                const int u = quantity(used, c->first, i->first);
                CHECK(i->second < 0 ? u >= i->second && u <= 0 : u <= i->second && u >= 0);
            }
        apply(base, m);
        int totalA = 0, totalB = 0;
        for (int c = 0; c < 5; ++c) {
            totalA += quantity(base, c, a); totalB += quantity(base, c, b);
            CHECK(quantity(base, c, a) == 20 + quantity(used, c, a));
            CHECK(quantity(base, c, b) == 20 + quantity(used, c, b));
        }
        CHECK(totalA == 100 && totalB == 100);
    }
}

static void carriedChildrenAreNotMovedTwice() {
    const Item bag = item("bag", 9), iron = item("iron"), food = item("food");
    State delta, base;
    delta[0][bag] = -1; delta[1][bag] = 1;
    delta[0][iron] = -5; delta[1][iron] = 5;
    delta[0][food] = -3; delta[1][food] = 3;
    base[0][bag] = 1; base[0][iron] = 8; base[0][food] = 3;
    Moves m; coop::planTransferMatches(delta, m);
    std::map<std::pair<int, int>, Counts> carried;
    carried[std::make_pair(0, 1)][iron] = 2;
    carried[std::make_pair(0, 1)][food] = 3;
    std::vector<int> wire;
    coop::transferNetworkQuantities(m, carried, wire);
    CHECK(m.size() == 3 && wire.size() == 3);
    for (unsigned int i = 0; i < m.size(); ++i) {
        if (m[i].key == bag) CHECK(wire[i] == 1);
        if (m[i].key == iron) CHECK(wire[i] == 3); // 2 bagged + 3 independent units
        if (m[i].key == food) CHECK(wire[i] == 0);
    }
    apply(base, m); // Baselines absorb ALL observed units, including carried ones.
    CHECK(quantity(base, 0, iron) == 3 && quantity(base, 1, iron) == 5);
    CHECK(quantity(base, 0, food) == 0 && quantity(base, 1, food) == 3);

    // Contents attributed to a different route must not suppress this move.
    carried.clear(); carried[std::make_pair(1, 0)][iron] = 99;
    coop::transferNetworkQuantities(m, carried, wire);
    for (unsigned int i = 0; i < m.size(); ++i) CHECK(wire[i] == m[i].qty);
}

int main() {
    multipleKindsOneBurst(); oneToManyAndRemainder(); manyToOneAndRemainder();
    unpairedAndTypeIsolation(); bidirectionalKindsAndPartialBaseline(); boundedConservation();
    carriedChildrenAreNotMovedTwice();
    std::printf("TransferPlanTests passed: %u checks\n", checks);
    return 0;
}
