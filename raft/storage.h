#ifndef MYRAFT_STORAGE_H_
#define MYRAFT_STORAGE_H_

#include <stdint.h>

#include <string>

#include "raftpb/raft.pb.h"

namespace myraft {

class Storage {
 private:
  using Entries = ::google::protobuf::RepeatedPtrField<::raftpb::Entry>;

 public:
  enum Error {
    OK,
    ErrCompacted,
    ErrSnapOutOfDate,
    ErrUnavailable,
    ErrSnapshotTemporarilyUnavailable,
  }; // enum Error

  static std::string ErrorString(Error error) {
    static const char* kErrorStrings[] = {
      "OK",
      "requested index is unavailable due to compaction",
      "requested index is older than the existing snapshot",
      "requested entry at index is unavailable",
      "snapshot is temporarily unavailable",
    };

    return kErrorStrings[error];
  }

 public:
  Storage()          = default;
  virtual ~Storage() = default;

  Storage(const Storage&)            = default;
  Storage& operator=(const Storage&) = default;
  Storage(Storage&&)                 = default;
  Storage& operator=(Storage&&)      = default;

  virtual Error InitialState(raftpb::HardState* hard_state, raftpb::ConfState* conf_state) const;
  virtual Error GetEntries(uint64_t low, uint64_t high, Entries* entries) const;
  //append 语义
  virtual Error Term(uint64_t index, uint64_t* result) const;
  virtual Error LastIndex(uint64_t* result) const;
  virtual Error FirstIndex(uint64_t* result) const;
  virtual Error Snapshot(raftpb::Snapshot* snapshot) const;
}; // class Storage

} // namespace myraft

#endif // MYRAFT_STORAGE_H_
