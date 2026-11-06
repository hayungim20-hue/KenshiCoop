// Restore the real object when a destination refuses a transfer. The final
// backend operation must preserve it on the ground when storage is exhausted.
#ifndef KENSHICOOP_TRANSFER_RETURN_H
#define KENSHICOOP_TRANSFER_RETURN_H

namespace coop {
enum TransferReturnResult {
    TRANSFER_RETURN_FAILED = 0,
    TRANSFER_RETURN_EQUIPPED = 1,
    TRANSFER_RETURN_STORED = 2,
    TRANSFER_RETURN_GROUND = 3
};

template <class Backend>
TransferReturnResult returnTransferredObject(Backend& back, bool wasEquipped) {
    if (wasEquipped && back.equip()) return TRANSFER_RETURN_EQUIPPED;
    if (back.originalSection()) return TRANSFER_RETURN_STORED;
    if (back.store()) return TRANSFER_RETURN_STORED;
    return back.storeOrDrop() ? TRANSFER_RETURN_STORED : TRANSFER_RETURN_GROUND;
}
} // namespace coop
#endif
