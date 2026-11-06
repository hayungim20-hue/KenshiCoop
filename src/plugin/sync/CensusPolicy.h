// Census framing and absence evidence. No engine dependencies: a partial list
// proves that its listed bodies exist, never that an omitted body does not.
#ifndef KENSHICOOP_CENSUS_POLICY_H
#define KENSHICOOP_CENSUS_POLICY_H

#include "../../netproto/Wire.h"
#include <cstring>

namespace coop {
namespace sync {

inline u16 censusCountField(unsigned int count, bool complete) {
    if (count > NPC_CENSUS_MAX) { count = NPC_CENSUS_MAX; complete = false; }
    return (u16)(count | (complete ? 0u : NPC_CENSUS_INCOMPLETE));
}

struct CensusFrame {
    unsigned int count;
    bool complete;
};

// Used by the real receive path. Check the masked count BEFORE using it in
// byte lengths or offsets, including a zero-row incomplete enumeration.
inline bool readCensusFrame(const void* data, unsigned int length,
                            NpcCensusHeader& header, CensusFrame& frame) {
    if (!data || length < sizeof(NpcCensusHeader)) return false;
    std::memcpy(&header, data, sizeof(header));
    if (header.type != PKT_NPC_CENSUS) return false;
    unsigned int count = header.count & NPC_CENSUS_COUNT_MASK;
    if (count > NPC_CENSUS_MAX) return false;
    unsigned int need = (unsigned int)sizeof(NpcCensusHeader)
                      + count * (5u * sizeof(u32) + 3u * sizeof(float));
    if (length < need) return false;
    frame.count = count;
    frame.complete = (header.count & NPC_CENSUS_INCOMPLETE) == 0;
    return true;
}

inline bool censusAllowsAbsence(bool complete, u32 recvMs, u32 now,
                                u32 freshMs) {
    return complete && recvMs != 0 && (u32)(now - recvMs) <= freshMs;
}

} // namespace sync
} // namespace coop
#endif
