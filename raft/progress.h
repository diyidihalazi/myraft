#ifndef MYRAFT_PROGRESS_H_
#define MYRAFT_PROGRESS_H_

#include <stdint.h>

#include <deque>
#include <memory>

#include "raftpb/raft.pb.h"

namespace myraft {

class Progress {
 public:
  enum ProgressState {
    ProgressStateProbe,
    ProgressStateReplicate,
    ProgressStateSnapshot,
  }; // enum ProgressState

 public:
  Progress(uint64_t match, uint64_t next, uint64_t max_inflight, bool is_learner);
  ~Progress() = default;

  Progress(const Progress&)            = default;
  Progress& operator=(const Progress&) = default;
  Progress(Progress&&)                 = default;
  Progress& operator=(Progress&&)      = default;

  void BecomeProbe();
  void BecomeReplicate();
  void BecomeSnapshot(uint64_t snapshot_index);

  bool MaybeUpdate(uint64_t index);
  bool MaybeDecrease(uint64_t rejected, uint64_t last);
  void OptimisticUpdate(uint64_t index) { next_ = index + 1; }

  void Pause()  { paused_ = true; }
  void Resume() { paused_ = false; }
  bool IsPaused() const;

  void SnapshotFailure() { pending_snapshot_ = 0; }
  bool NeedSnapshotAbort() const
  { return ProgressStateSnapshot == state_ && match_ >= pending_snapshot_; }

  bool ProgressAppResp(const std::unique_ptr<const raftpb::Message>& m);
  void ProgressHeartbeatResp(const std::unique_ptr<const raftpb::Message>& m);
  void ProgressSnapStatus(const std::unique_ptr<const raftpb::Message>& m);
  void ProgressUnreachable(const std::unique_ptr<const raftpb::Message>& m);

  std::string String() const;

 private:
  class Inflights {
   public:
    Inflights(uint64_t max_size) : max_size_(max_size) {}
    ~Inflights() = default;

    Inflights(const Inflights&)            = default;
    Inflights& operator=(const Inflights&) = default;
    Inflights(Inflights&&)                 = default;
    Inflights& operator=(Inflights&&)      = default;

    void Push(uint64_t inflight);
    void PopTo(uint64_t to);
    void PopFirstOne() { queue_.pop_front(); }
    void Clear()       { queue_.clear(); }

    bool Full() const  { return queue_.size() == max_size_; }

   private:
    std::deque<uint64_t> queue_;
    const uint64_t       max_size_;
  }; // class Inflights

 private:
  uint64_t      match_;
  uint64_t      next_;
  ProgressState state_;
  bool          paused_;
  uint64_t      pending_snapshot_;
  bool          recent_active_;
  Inflights     inflights_;
  bool          is_learner_;
}; // class Progress

} // namespace myraft

#endif // MYRAFT_PROGRESS_H_
