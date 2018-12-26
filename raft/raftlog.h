#ifndef MYRAFT_RAFTLOG_H_
#define MYRAFT_RAFTLOG_H_

#include <stdint.h>

#include <memory>

#include "storage.h"
#include "entry_slice.h"
#include "unstable.h"
#include "raftpb/raft.pb.h"

namespace myraft {

class RaftLog {
 private:
  using Entries = ::google::protobuf::RepeatedPtrField<::raftpb::Entry>;

 public:
  RaftLog(const std::shared_ptr<Storage>& storage, uint64_t first_index, uint64_t last_index);
  ~RaftLog() = default;

  RaftLog(const RaftLog&)            = delete;
  RaftLog& operator=(const RaftLog&) = delete;
  RaftLog(RaftLog&&)                 = default;
  RaftLog& operator=(RaftLog&&)      = default;

  bool MaybeAppend(uint64_t index, uint64_t term, uint64_t committed,
                   const EntrySlice& entries, uint64_t* new_last_index);
  void Restore(const raftpb::Snapshot& snapshot);

  bool MaybeCommit(uint64_t index, uint64_t term);
  void CommitTo(uint64_t committed);
  void ApplyTo(uint64_t applied);
  void StableTo(uint64_t index, uint64_t term);
  void StableSnapTo(uint64_t index);

  bool IsUpToData(uint64_t index, uint64_t term);
  bool MatchTerm(uint64_t index, uint64_t term);

  Storage::Error Snapshot(raftpb::Snapshot* snapshot) const;
  Storage::Error GetEntries(uint64_t index, uint64_t max, Entries* entries);
  void UnstableEntries(Entries* entries);
  void NextEntries(Entries* entries);
  bool HasNextEntries() const;

  uint64_t FirstIndex() const;
  uint64_t LastIndex() const;
  uint64_t LastTerm();
  Storage::Error Term(uint64_t index, uint64_t* result);

  std::string String() const;

  static uint64_t ZeroTermOnErrCompacted(uint64_t term, Storage::Error error);

 private:
  uint64_t Append(const EntrySlice& entries);
  uint64_t FindConflict(const EntrySlice& entries);

  Storage::Error Slice(uint64_t low, uint64_t high, Entries* entries);
  Storage::Error MustCheckOutOfBounds(uint64_t low, uint64_t high) const;

 private:
  std::shared_ptr<Storage> storage_;
  Unstable unstable_;
  uint64_t committed_;
  uint64_t applied_;
}; // class RaftLog

std::unique_ptr<RaftLog> NewRaftLog(const std::shared_ptr<Storage>& storage);

} // namespace myraft

#endif // MYRAFT_RAFTLOG_H_
