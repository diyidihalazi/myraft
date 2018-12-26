#ifndef MYRAFT_ENTRY_SLICE_H_
#define MYRAFT_ENTRY_SLICE_H_

#include "raftpb/raft.pb.h"

namespace myraft {

class EntrySlice {
 private:
  using Entries = ::google::protobuf::RepeatedPtrField<::raftpb::Entry>;

 public:
  EntrySlice(const Entries& entries, int first, int size)
      : entries_(entries), first_(first), size_(size) {}
  EntrySlice(const EntrySlice& entry_slice, int first, int size)
      : entries_(entry_slice.entries_),
        first_(entry_slice.first_ + first),
        size_(size) {}

  ~EntrySlice() = default;

  EntrySlice(const EntrySlice&) = default;
  EntrySlice& operator=(const EntrySlice&) = default;
  EntrySlice(EntrySlice&&) = default;
  EntrySlice& operator=(EntrySlice&&) = default;

  const raftpb::Entry& operator[](int index) const {
    return entries_[first_ + index];
  }

  int Size() const { return size_; }

 private:
  const Entries& entries_;
  int first_;
  int size_;
}; // class EntrySlice

} // namespace myraft

#endif // MYRAFT_ENTRY_SLICE_H_
