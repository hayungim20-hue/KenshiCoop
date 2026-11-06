#ifndef KENSHICOOP_TRANSFER_ACK_H
#define KENSHICOOP_TRANSFER_ACK_H

namespace coop {
// An ACK confirms application, not delivery of the owner's post-transfer
// snapshot. Keep the accepted part protected until that snapshot catches up.
struct TransferAckAmounts {
    int accepted;
    int refused;
};
inline TransferAckAmounts transferAckAmounts(int requested, int applied) {
    TransferAckAmounts out;
    if (requested < 0) requested = 0;
    if (applied < 0) applied = 0;
    if (applied > requested) applied = requested;
    out.accepted = applied;
    out.refused = requested - applied;
    return out;
}
}
#endif
