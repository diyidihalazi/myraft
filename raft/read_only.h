#ifndef MYRAFT_READ_ONLY_H_
#define MYRAFT_READ_ONLY_H_

#include <stdint.h>

#include <string>
#include <memory>
#include <vector>
#include <deque>
#include <set>
#include <unordered_map>

#include "raftpb/raft.pb.h"

namespace myraft {

class ReadOnly {
 public:
  enum ReadOnlyOption {
    ReadOnlySafe,
    ReadOnlyLeaseBased,
  }; // enum ReadOnlyOption

  struct ReadIndexStatus {
    std::unique_ptr<raftpb::Message> request;
    uint64_t                    index;
    std::set<uint64_t>          acks;
  }; // struct ReadIndexStatus

 public:
  ReadOnly(ReadOnlyOption read_option);
  ~ReadOnly() = default;

  ReadOnly(const ReadOnly&)            = delete;
  ReadOnly& operator=(const ReadOnly&) = delete;
  ReadOnly(ReadOnly&&)                 = default;
  ReadOnly& operator=(ReadOnly&&)      = default;

  void AddRequest(uint64_t index, std::unique_ptr<raftpb::Message> msg);
  uint32_t RecvAck(const std::unique_ptr<const raftpb::Message>& msg);
  std::vector<std::unique_ptr<ReadIndexStatus>> Advance(const std::unique_ptr<const raftpb::Message>& msg);
  std::string LastPendingRequestCtx();

 private:
  const ReadOnlyOption    kReadOption;
  std::deque<std::string> readindex_queue_;
  std::unordered_map<std::string, std::unique_ptr<ReadIndexStatus>> pending_readindex_;
}; // class ReadOnly

} // namespace myraft

#endif // MYRAFT_READ_ONLY_H_
