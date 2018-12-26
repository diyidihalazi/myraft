#include "progress.h"

#include <limits>

#include <util/util.h>

namespace myraft {

Progress::Progress(uint64_t match,
                   uint64_t next,
                   uint64_t max_inflight,
                   bool is_learner)
    : match_(match),
      next_(next),
      state_(ProgressStateProbe),
      paused_(false),
      pending_snapshot_(0),
      recent_active_(false),
      inflights_(max_inflight),
      is_learner_(is_learner) {}

void Progress::BecomeProbe() {
  if (ProgressStateSnapshot == state_) {
    next_   = std::max(match_ + 1, pending_snapshot_ + 1);
    state_  = ProgressStateProbe;
    paused_ = false;
    pending_snapshot_ = 0;
    inflights_.Clear();
  } else {
    next_   = match_ + 1;
    state_  = ProgressStateProbe;
    paused_ = false;
    pending_snapshot_ = 0;
    inflights_.Clear();
  }
}

void Progress::BecomeReplicate() {
  next_   = match_ + 1;
  state_  = ProgressStateReplicate;
  paused_ = false;
  pending_snapshot_ = 0;
  inflights_.Clear();
}

void Progress::BecomeSnapshot(uint64_t snapshot_index) {
  state_  = ProgressStateSnapshot;
  paused_ = false;
  pending_snapshot_ = snapshot_index;
  inflights_.Clear();
}

bool Progress::MaybeUpdate(uint64_t index) {
  bool updated = false;

  if (match_ < index) {
    match_ = index;
    updated = true;
    Resume();
  }

  if (next_ <= index) {
    next_ = index + 1;
  }

  return updated;
}

bool Progress::MaybeDecrease(uint64_t rejected, uint64_t last) {
  if (ProgressStateReplicate == state_) {
    if (rejected <= match_) {
      return false;
    }

    next_ = match_ + 1;
    return true;
  }

  if (next_ - 1 != rejected) {
    return false;
  }

  next_ = std::min(rejected, last + 1);
  if (next_ < 1) {
    next_ = 1;
  }
  Resume();

  return true;
}

bool Progress::IsPaused() const {
  switch (state_) {
    case ProgressStateProbe:
      return paused_;
    case ProgressStateReplicate:
      return inflights_.Full();
    case ProgressStateSnapshot:
      return true;
    default:
      assert(false);
  }
}

std::string Progress::String() const {
  char buff[1024] = {0};

  std::string state;
  switch (state_) {
    case ProgressStateProbe:
      state = "ProgressStateProbe";
      break;
    case ProgressStateReplicate:
      state = "ProgressStateReplicate";
      break;
    case ProgressStateSnapshot:
      state = "ProgressStateSnapshot";
      break;
    default:
      assert(false);
  }

  sprintf(buff, "next = %lu, match = %lu, state = %s, waiting = %s, pending_snapshot = %lu",
          next_, match_, state.data(), myutil::String(IsPaused()).data(), pending_snapshot_);
  assert(strlen(buff) < 1024);
  return buff;
}

bool Progress::ProgressAppResp(const std::unique_ptr<const raftpb::Message>& m) {
  recent_active_ = true;

  if (m->reject()) {
    if (MaybeDecrease(m->index(), m->rejecthint())) {
      if (ProgressStateReplicate == state_) {
        BecomeProbe();
      }
      return true;
    }
  } else {
    if (MaybeUpdate(m->index())) {
      switch (state_) {
        case ProgressStateProbe:
          BecomeReplicate();
          break;
        case ProgressStateSnapshot:
          if (NeedSnapshotAbort()) {
            BecomeProbe();
          }
          break;
        case ProgressStateReplicate:
          inflights_.PopTo(m->index());
          break;
      }
      return true;
    }
  }

  return false;
}

void Progress::ProgressHeartbeatResp(const std::unique_ptr<const raftpb::Message>& m) {
  (void)m;

  recent_active_ = true;
  Resume();

  if (ProgressStateReplicate == state_ && inflights_.Full()) {
    inflights_.PopFirstOne();
  }
}

void Progress::ProgressSnapStatus(const std::unique_ptr<const raftpb::Message>& m) {
  if (ProgressStateSnapshot != state_) {
    return;
  }

  if (m->reject()) {
    SnapshotFailure();
    BecomeProbe();
  } else {
    BecomeProbe();
  }

  Pause();
}

void Progress::ProgressUnreachable(const std::unique_ptr<const raftpb::Message>& m) {
  (void)m;

  if (ProgressStateReplicate == state_) {
    BecomeProbe();
  }
}

void Progress::Inflights::Push(uint64_t inflight) {
  if (Full()) { /*Panicf*/ }
  queue_.push_back(inflight);
}

void Progress::Inflights::PopTo(uint64_t to) {
  while (!queue_.empty() && to >= queue_.front()) {
    queue_.pop_front();
  }
}

} // namespace myraft
