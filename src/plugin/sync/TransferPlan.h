// Pure C++03 cross-container transfer planning. The caller supplies only
// settled, locally observed deltas; ownership and networking stay with it.
#ifndef KENSHICOOP_TRANSFER_PLAN_H
#define KENSHICOOP_TRANSFER_PLAN_H

#include <map>
#include <utility>
#include <vector>

namespace coop {

template <class ContainerKey, class ItemKey>
struct TransferMatch {
    ContainerKey src;
    ContainerKey dst;
    ItemKey key;
    int qty;
};

// Consume quantities, not entire containers: several item kinds can move in
// one drag burst, and a stack can fan out to several recipients in one scan.
// Input is copied so the caller retains unmatched observations and timestamps.
template <class ContainerKey, class ItemKey>
void planTransferMatches(
        const std::map<ContainerKey, std::map<ItemKey, int> >& deltas,
        std::vector<TransferMatch<ContainerKey, ItemKey> >& matches) {
    typedef std::map<ItemKey, int> ItemDeltas;
    typedef std::map<ContainerKey, ItemDeltas> ContainerDeltas;
    ContainerDeltas remaining(deltas);
    matches.clear();
    for (typename ContainerDeltas::iterator src = remaining.begin();
         src != remaining.end(); ++src) {
        for (typename ItemDeltas::iterator loss = src->second.begin();
             loss != src->second.end(); ++loss) {
            if (loss->second >= 0) continue;
            for (typename ContainerDeltas::iterator dst = remaining.begin();
                 dst != remaining.end() && loss->second < 0; ++dst) {
                if (src == dst) continue;
                typename ItemDeltas::iterator gain = dst->second.find(loss->first);
                if (gain == dst->second.end() || gain->second <= 0) continue;
                int qty = -loss->second;
                if (gain->second < qty) qty = gain->second;
                TransferMatch<ContainerKey, ItemKey> m;
                m.src = src->first; m.dst = dst->first;
                m.key = loss->first; m.qty = qty;
                matches.push_back(m);
                loss->second += qty;
                gain->second -= qty;
            }
        }
    }
}

// Advance only the exact paired units. Rebasing a whole container here would
// silently absorb other item kinds, unpaired losses/gains, and stack remainders.
template <class ContainerKey, class ItemKey>
void advanceTransferBaseline(
        std::map<ContainerKey, std::map<ItemKey, int> >& baseline,
        const TransferMatch<ContainerKey, ItemKey>& m) {
    std::map<ItemKey, int>& source = baseline[m.src];
    int& from = source[m.key];
    from -= m.qty;
    if (from == 0) source.erase(m.key);
    baseline[m.dst][m.key] += m.qty;
}

// A relocated bag carries its children as real objects. Those observed child
// deltas still advance the baseline but must not generate a second move/fab.
template <class ContainerKey, class ItemKey>
void transferNetworkQuantities(
        const std::vector<TransferMatch<ContainerKey, ItemKey> >& matches,
        const std::map<std::pair<ContainerKey, ContainerKey>, std::map<ItemKey, int> >& carried,
        std::vector<int>& quantities) {
    typedef std::pair<ContainerKey, ContainerKey> Ends;
    typedef std::map<ItemKey, int> ItemCounts;
    typedef std::map<Ends, ItemCounts> CarriedCounts;
    CarriedCounts remaining(carried);
    quantities.clear();
    for (unsigned int i = 0; i < matches.size(); ++i) {
        const TransferMatch<ContainerKey, ItemKey>& m = matches[i];
        int qty = m.qty;
        typename CarriedCounts::iterator ends = remaining.find(Ends(m.src, m.dst));
        if (ends != remaining.end()) {
            typename ItemCounts::iterator child = ends->second.find(m.key);
            if (child != ends->second.end() && child->second > 0) {
                int covered = child->second < qty ? child->second : qty;
                qty -= covered; child->second -= covered;
            }
        }
        quantities.push_back(qty);
    }
}

} // namespace coop

#endif
