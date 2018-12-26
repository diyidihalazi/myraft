#ifndef MYRAFT_UNSTABLE_H_
#define MYRAFT_UNSTABLE_H_

namespace myraft {

class Unstable {
 private:
  using Entries = ::google::protobuf::RepeatedPtrField<::raftpb::Entry>;
  static const uint64_t kEntryBufferSize = 1000;

 public:
  Unstable(uint64_t first) : first_(first), last_(first_ - 1) {}
  ~Unstable() = default;

  Unstable(const Unstable&)            = delete;
  Unstable& operator=(const Unstable&) = delete;
  Unstable(Unstable&&)                 = default;
  Unstable& operator=(Unstable&&)      = default;

  bool MaybeFirstIndex(uint64_t* result) const;
  bool MaybeLastIndex(uint64_t* result) const;
  bool MaybeTerm(uint64_t index, uint64_t* result) const;
  bool Snapshot(raftpb::Snapshot* snapshot) const;

  void StableTo(uint64_t index, int64_t term);
  void StableSnapTo(uint64_t index);

  void Restore(const raftpb::Snapshot& snapshot);
  void TruncateAndAppend(const EntrySlice& entries);

  void Slice(uint64_t low, uint64_t high, Entries* entries) const;

  uint64_t First() const { return first_; }
  uint64_t Last()  const { return last_; }
  uint64_t Size()  const { return last_ + 1 - first_; }

 private:
  void Append(const EntrySlice& entries);

  void MustCheckOutofBounds(uint64_t low, uint64_t high) const;

  static uint64_t BaseIndex(uint64_t index) { return index / kEntryBufferSize * kEntryBufferSize; }

 private:
  std::unique_ptr<raftpb::Snapshot> snapshot_;

  std::map<uint64_t, std::unique_ptr<raftpb::Entry[]>> entries_;

  uint64_t first_;
  uint64_t last_;
}; // class Unstable

} // namespace myraft

#endif // MYRAFT_UNSTABLE_H_
