#include "read_only.h"

#include <utility>

namespace myraft {

ReadOnly::ReadOnly(ReadOnlyOption read_option)
    : kReadOption(read_option) {}

void ReadOnly::AddRequest(uint64_t index, std::unique_ptr<raftpb::Message> msg) {
  std::string ctx = msg->entries(0).data();
  if (pending_readindex_.end() != pending_readindex_.find(ctx)) {
    return ;
  }

  auto& rs = pending_readindex_[ctx];
  rs->request = std::move(msg);
  rs->index = index;

  readindex_queue_.push_back(std::move(ctx));
}

uint32_t ReadOnly::RecvAck(const std::unique_ptr<const raftpb::Message>& msg) {
  auto rs = pending_readindex_.find(msg->context());
  if (pending_readindex_.end() == rs) {
    return 0;
  }

  rs->second->acks.insert(msg->from());
  return rs->second->acks.size() + 1;
}

std::vector<std::unique_ptr<ReadOnly::ReadIndexStatus>> ReadOnly::Advance(
    const std::unique_ptr<const raftpb::Message>& msg) {
  const std::string ctx = msg->context();

  int count = 0;
  bool found = false;
  for (auto iter = readindex_queue_.begin(); readindex_queue_.end() != iter; iter++) {
    count++;
    if (pending_readindex_.end() == pending_readindex_.find(*iter)) {
      //panic
    }

    if (ctx == *iter) {
      found = true;
      break;
    }
  }

  std::vector<std::unique_ptr<ReadIndexStatus>> rss;
  if (found) {
    for (int i = 0; i < count; i++) {
      rss.push_back(std::move(pending_readindex_[readindex_queue_.front()]));
      pending_readindex_.erase(readindex_queue_.front());
      readindex_queue_.pop_front();
    }
  }

  return std::move(rss);
}

std::string ReadOnly::LastPendingRequestCtx() {
  if (0 == readindex_queue_.size()) {
    return "";
  }

  return readindex_queue_.back();
}

}; // namspace myraft
