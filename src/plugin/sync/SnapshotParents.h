// C++03 helpers for inventory snapshot parent references. Entry::parentIdx
// is a one-based index in the SAME snapshot, never an index in another capture.
#ifndef KENSHICOOP_SNAPSHOT_PARENTS_H
#define KENSHICOOP_SNAPSHOT_PARENTS_H

#include <cstring>
#include <vector>

namespace coop {

template <class Entry>
unsigned int snapshotParentForLocal(const std::vector<Entry>& target,
                                    const Entry* local, unsigned int localCount,
                                    unsigned int localParent) {
    if (localParent == 0 || localParent > localCount) return 0;
    const Entry& parent = local[localParent - 1];
    unsigned int rank = 0;
    for (unsigned int i = 0; i + 1 < localParent; ++i)
        if (local[i].parentIdx == 0 && local[i].itemType == parent.itemType &&
            std::strcmp(local[i].stringID, parent.stringID) == 0) ++rank;
    for (unsigned int i = 0; i < target.size(); ++i) {
        if (target[i].parentIdx != 0 || target[i].quantity == 0 ||
            target[i].itemType != parent.itemType ||
            std::strcmp(target[i].stringID, parent.stringID) != 0) continue;
        if (rank == 0) return i + 1;
        --rank;
    }
    // No corresponding parent in this older snapshot: hold the real item's
    // quantity at the character's top level until the authoritative bag arrives.
    return 0;
}

template <class Entry>
void compactSnapshotParents(std::vector<Entry>& items) {
    std::vector<unsigned int> remap(items.size(), 0);
    std::vector<Entry> kept;
    for (unsigned int i = 0; i < items.size(); ++i) {
        if (items[i].quantity == 0) continue;
        unsigned int parent = items[i].parentIdx;
        if (parent != 0 && (parent > items.size() ||
            items[parent - 1].quantity == 0 || items[parent - 1].parentIdx != 0)) continue;
        remap[i] = (unsigned int)kept.size() + 1;
        kept.push_back(items[i]);
    }
    for (unsigned int i = 0; i < kept.size(); ++i) {
        unsigned int parent = kept[i].parentIdx;
        if (parent != 0) kept[i].parentIdx = (unsigned char)remap[parent - 1];
    }
    items.swap(kept);
}

} // namespace coop

#endif
