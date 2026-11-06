#include "../src/plugin/sync/SnapshotParents.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct Entry {
    char stringID[48];
    unsigned int itemType;
    unsigned short quantity;
    unsigned char parentIdx;
};
static unsigned int checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { \
    std::printf("FAIL line %d: %s\n", __LINE__, #x); std::exit(1); } } while (0)

static Entry entry(const char* sid, unsigned short qty, unsigned char parent = 0) {
    Entry e; std::memset(&e, 0, sizeof(e));
    std::strncpy(e.stringID, sid, sizeof(e.stringID) - 1);
    e.itemType = 2; e.quantity = qty; e.parentIdx = parent;
    return e;
}

static void compactRemovedRows() {
    std::vector<Entry> v;
    v.push_back(entry("removed item", 0));
    v.push_back(entry("bag", 1));
    v.push_back(entry("iron", 2, 2));
    coop::compactSnapshotParents(v);
    CHECK(v.size() == 2);
    CHECK(std::strcmp(v[0].stringID, "bag") == 0);
    CHECK(v[1].parentIdx == 1 && v[1].quantity == 2);
}

static void removedParentRemovesChildren() {
    std::vector<Entry> v;
    v.push_back(entry("bag", 0));
    v.push_back(entry("iron", 3, 1));
    v.push_back(entry("food", 1));
    coop::compactSnapshotParents(v);
    CHECK(v.size() == 1);
    CHECK(std::strcmp(v[0].stringID, "food") == 0 && v[0].parentIdx == 0);
}

static void mapCaptureOrderAndBagRank() {
    Entry local[4];
    local[0] = entry("bag", 1);
    local[1] = entry("bag", 1);
    local[2] = entry("iron", 2, 2);
    local[3] = entry("food", 3, 1);
    std::vector<Entry> target;
    target.push_back(entry("sword", 1));
    target.push_back(entry("bag", 1));
    target.push_back(entry("bag", 1));
    CHECK(coop::snapshotParentForLocal(target, local, 4, local[2].parentIdx) == 3);
    CHECK(coop::snapshotParentForLocal(target, local, 4, local[3].parentIdx) == 2);
    CHECK(coop::snapshotParentForLocal(target, local, 4, 0) == 0);
    CHECK(coop::snapshotParentForLocal(target, local, 4, 5) == 0);
    target[2].quantity = 0;
    CHECK(coop::snapshotParentForLocal(target, local, 4, 2) == 0);
}

static void mappedAppendSurvivesCompaction() {
    Entry local[2]; local[0] = entry("bag", 1); local[1] = entry("iron", 2, 1);
    std::vector<Entry> v;
    v.push_back(entry("gone", 0));
    v.push_back(entry("bag", 1));
    Entry held = local[1];
    held.parentIdx = (unsigned char)coop::snapshotParentForLocal(v, local, 2, held.parentIdx);
    v.push_back(held);
    CHECK(v[2].parentIdx == 2);
    coop::compactSnapshotParents(v);
    CHECK(v.size() == 2 && v[1].parentIdx == 1);
    coop::compactSnapshotParents(v);
    CHECK(v.size() == 2 && v[1].parentIdx == 1); // idempotent
}

static void invalidParentAndForwardReference() {
    std::vector<Entry> v;
    v.push_back(entry("child before parent", 1, 2));
    v.push_back(entry("bag", 1));
    v.push_back(entry("orphan", 1, 17));
    v.push_back(entry("nested beyond one level", 1, 1));
    coop::compactSnapshotParents(v);
    CHECK(v.size() == 2 && v[0].parentIdx == 2);
}

int main() {
    compactRemovedRows(); removedParentRemovesChildren(); mapCaptureOrderAndBagRank();
    mappedAppendSurvivesCompaction(); invalidParentAndForwardReference();
    std::printf("SnapshotParentsTests passed: %u checks\n", checks);
    return 0;
}
