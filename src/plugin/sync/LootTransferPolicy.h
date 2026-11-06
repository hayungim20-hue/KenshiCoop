#ifndef KENSHICOOP_LOOT_TRANSFER_POLICY_H
#define KENSHICOOP_LOOT_TRANSFER_POLICY_H

namespace coop {
// Shared body stock has a single authority. A short source is not permission
// to fabricate: another looter may already have claimed those same units.
inline bool transferSourceMayFabricate(bool localAuthority, bool sharedBody) {
    return !localAuthority && !sharedBody;
}
// Zero is an authority-held body latch, not a wall-clock expiry. A delayed
// host snapshot must never resurrect loot just because ten seconds elapsed.
inline bool transferLatchExpired(bool sharedBody, unsigned long deadline,
                                 unsigned long now) {
    return !(sharedBody && deadline == 0) && now > deadline;
}
inline bool retainBodyAuthority(unsigned long now, unsigned long lastSeen,
                                bool pendingTransfer) {
    return pendingTransfer || now - lastSeen <= 30000;
}
} // namespace coop
#endif
