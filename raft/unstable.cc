#include "unstable.h"

namespace myraft {

bool Unstable::MaybeFirstIndex(uint64_t* result) const {
  if (nullptr != snapshot_.get()) {
    *result = snapshot_->metadata().index() + 1;
    return true;
  }

  return false;
}

bool Unstable::MaybeLastIndex(uint64_t* result) const {
  if (0 != (last_ - first_ + 1)) {
    *result = last_;
    return true;
  }

  if (nullptr != snapshot_.get()) {
    *result = snapshot_->metadata().index();
    return true;
  }

  return false;
}

bool Unstable::MaybeTerm(uint64_t index, uint64_t* result) {
  if (nullptr != snapshot_.get() && snapshot_->metadata().index() == index) {
    *result = snapshot_->metadata().term();
    return true;
  }

  if (index < first_ || index > last_) {
    return false;
  }

  uint64_t base = BaseIndex(index);
  *result = entries_[base][index - base].term();
  return true;
}

bool Unstable::Snapshot(raftpb::Snapshot* snapshot) const {
  if (nullptr != snapshot_.get()) {
    *snapshot = *snapshot_;
    return true;
  }

  return false;
}

void Unstable::StableTo(uint64_t index, uint64_t term) {
  uint64_t gterm = 0;
  if (!MaybeTerm(index, &gterm)) {
    return ;
  }

  if (gterm == term && index >= first_) {
    for (uint64_t base = BaseIndex(first_), limit = BaseIndex(index + 1);
         base < limit; base += kEntryBufferSize) {
      entries_.erase(base);
    }
    first_ = index + 1;
  }
}

void Unstable::StableSnapTo(uint64_t index) {
  if (nullptr != snapshot_.get() && snapshot_->metadata().index() == index) {
    snapshot_.reset(nullptr);
  }
}

void Unstable::Restore(const raftpb::Snapshot& snapshot) {
  snapshot_.reset(new raftpb::Snapshot(snapshot));
  entries_.clear();
  first_ = snapshot_->metadata().index() + 1;
  last_ = first_ - 1;
}

void Unstable::TruncateAndAppend(const EntrySlice& entries) {
  uint64_t after = entries[0].index();
  if (after == last_ + 1) {
    Append(entries);
  } else if (after <= first_) {
    entries_.clear();
    first_ = after;
    last_ = after - 1;
    Append(entries);
  } else {
    last_ = after - 1;
    Append(entries);
  }
}

void Unstable::Slice(uint64_t low, uint64_t high, Entries* entries) {
  MustCheckOutofBounds(low, high);
  for (uint64_t i = low; i < high; ) {
    uint64_t base = BaseIndex(i);
    std::unique_ptr<raftpb::Entry[]>& buffer = entries_[base];

    for (uint64_t j = i - base; j < kEntryBufferSize && i < high; j++, i++) {
      *(entries->Add()) = buffer[j];
    }
  }
}

void Unstable::Append(const EntrySlice& entries) {
  for (uint64_t i = 0, size = entries.Size(); i < size; ) {
    uint64_t base = BaseIndex(entries[i].index());

    std::unique_ptr<raftpb::Entry[]>& buffer = entries_[base];
    if (nullptr == buffer.get()) {
      buffer.reset(new raftpb::Entry[kEntryBufferSize]);
    }

    for (uint64_t j = entries[i].index() - base;
         j < kEntryBufferSize && i < size; j++, i++) {
      buffer[j] = entries[i];
    }
  }

  last_ = last_ + entries.Size();
}

void Unstable::MustCheckOutofBounds(uint64_t low, uint64_t high) const {
  if (low > high) {
    //Panic
  }

  if (low < first_ || high > last_ + 1) {
    //Panic
  }
}

} // namespace myraft
