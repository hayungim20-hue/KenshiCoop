#ifndef KENSHICOOP_COMBAT_POLICY_H
#define KENSHICOOP_COMBAT_POLICY_H
#include "../../netproto/Wire.h"
#include <map>
namespace coop { namespace sync {
inline bool combatDamageValid(float flesh, float blood) {
    // Comparisons reject NaN and infinity without platform-specific intrinsics.
    return flesh >= 0.0f && flesh <= 1000000.0f &&
           blood >= 0.0f && blood <= 1000000.0f && (flesh > 0.0f || blood > 0.0f);
}
// Protocol 68: a report may carry damage, a knockout, or both. Knockouts are
// bounded (Kenshi's own KO timers stay well under ten minutes).
inline bool combatReportValid(float flesh, float blood, float ko) {
    bool dmgOk = flesh >= 0.0f && flesh <= 1000000.0f && blood >= 0.0f && blood <= 1000000.0f;
    bool koOk  = ko >= 0.0f && ko <= 600.0f;
    return dmgOk && koOk && (flesh > 0.0f || blood > 0.0f || ko > 0.0f);
}
inline bool combatAddressed(u32 sender, u32 target, u32 me) {
    return sender < MAX_PLAYERS && target < MAX_PLAYERS && sender != target && target == me;
}
inline bool combatMayApply(u32 target, u32 me, u32 currentOwner, bool playerBody) {
    return target == me && currentOwner == me && !playerBody;
}
struct CombatReplay {
    std::map<u32, u32> seen;
    bool accept(u32 sender, u32 id) {
        if(sender >= MAX_PLAYERS || id == 0) return false;
        std::map<u32,u32>::iterator it=seen.find(sender);
        if(it!=seen.end()) {
            if((int)(id-it->second)<=0) return false;
            it->second=id;
        } else seen[sender]=id;
        return true;
    }
    void clear() { seen.clear(); }
    void forget(u32 sender) { seen.erase(sender); }
};
} }
#endif
