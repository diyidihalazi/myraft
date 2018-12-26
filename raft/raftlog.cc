#include "raftlog.h"

#include <stdio.h>

#include <util/make_unique.h>

namespace myraft {

RaftLog::RaftLog(const std::shared_ptr<Storage>& storage,
                 uint64_t first_index, uint64_t last_index)
    : storage_(storage),
      unstable_(last_index + 1),
      committed_(first_index - 1),
      applied_(first_index - 1) {}

bool RaftLog::MaybeAppend(uint64_t index, uint64_t term, uint64_t committed,
                          const EntrySlice& entries, uint64_t* new_last_index) {
  if (MatchTerm(index, term)) {
    *new_last_index = index + static_cast<uint64_t>(entries.Size());

    uint64_t conflict = FindConflict(entries);
    if (0 == conflict) {

    } else if (conflict <= committed_) {
      //Panic
    } else {
      uint64_t offset = index + 1;
      uint64_t first = conflict - offset;
      Append(EntrySlice(entries, first, entries.Size() - first));
    }

    CommitTo(std::min(committed, *new_last_index));
    return true;
  }

  return false;
}

void RaftLog::Restore(const raftpb::Snapshot& snapshot) {
  //Infof
  committed_ = snapshot.metadata().index();
  unstable_.Restore(snapshot);
}

bool RaftLog::MaybeCommit(uint64_t index, uint64_t term) {
  uint64_t gterm = 0;
  auto error = Term(index, &gterm);
  if (index > committed_ && ZeroTermOnErrCompacted(gterm, error) == term) {
    CommitTo(index);
    return true;
  }

  return false;
}

void RaftLog::CommitTo(uint64_t committed) {
  if (committed_ < committed) {
    if (LastIndex() < committed) {
      //Panicf
    }
    committed_ = committed;
  }
}

void RaftLog::ApplyTo(uint64_t applied) {
  if (0 == applied) {
    return ;
  }
  if (committed_ < applied || applied < applied_) {
    //Panicf
  }
  applied_ = applied;
}

void RaftLog::StableTo(uint64_t index, uint64_t term) {
  unstable_.StableTo(index, term);
}

void RaftLog::StableSnapTo(uint64_t index) {
  unstable_.StableSnapTo(index);
}

bool RaftLog::IsUpToData(uint64_t index, uint64_t term) {
  uint64_t last_term = LastTerm();
  return (term > last_term || (term == last_term && index >= LastIndex()));
}

bool RaftLog::MatchTerm(uint64_t index, uint64_t term) {
  uint64_t gterm = 0;
  auto error = Term(index, &gterm);
  if (Storage::OK != error) {
    return false;
  }

  return gterm == term;
}

Storage::Error RaftLog::Snapshot(raftpb::Snapshot* snapshot) const {
  if (unstable_.Snapshot(snapshot)) {
    return Storage::OK;
  }

  return storage_->Snapshot(snapshot);
}

Storage::Error RaftLog::GetEntries(uint64_t index, uint64_t max, Entries* entries) {
  uint64_t last_index = LastIndex();
  if (index > last_index) {
    entries->Clear();
    return Storage::OK;
  }

  return Slice(index, std::min(index + max, last_index + 1), entries);
}

void RaftLog::UnstableEntries(Entries* entries) {
  entries->Clear();

  if (0 == unstable_.Size()) {
    return ;
  }

  unstable_.Slice(unstable_.First(), unstable_.Last() + 1, entries);
  return ;
}

void RaftLog::NextEntries(Entries* entries) {
  uint64_t offset = std::max(applied_ + 1, FirstIndex());
  if (committed_ + 1 > offset) {
    auto error = Slice(offset, committed_ + 1, entries);
    if (Storage::OK != error) {
      //Panicf
    }
  }
}

bool RaftLog::HasNextEntries() const {
  uint64_t offset = std::max(applied_ + 1, FirstIndex());
  return committed_ + 1 > offset;
}

uint64_t RaftLog::FirstIndex() const {
  uint64_t first_index = 0;
  if (unstable_.MaybeFirstIndex(&first_index)) {
    return first_index;
  }

  auto error = storage_->FirstIndex(&first_index);
  if (Storage::OK != error) {
    //Panicf
  }
  return first_index;
}

uint64_t RaftLog::LastIndex() const {
  uint64_t last_index = 0;
  if (unstable_.MaybeLastIndex(&last_index)) {
    return last_index;
  }

  auto error = storage_->LastIndex(&last_index);
  if (Storage::OK != error) {
    //Panicf
  }
  return last_index;
}

uint64_t RaftLog::LastTerm() {
  uint64_t last_term = 0;
  auto error = Term(LastIndex(), &last_term);
  if (Storage::OK != error) {
    //Panicf
  }
  return last_term;
}

Storage::Error RaftLog::Term(uint64_t index, uint64_t* result) {
  uint64_t dummy_index = FirstIndex() - 1;
  if (index < dummy_index || index > LastIndex()) {
    *result = 0;
    return Storage::OK;
  }

  if (unstable_.MaybeTerm(index, result)) {
    return Storage::OK;
  }

  auto error = storage_->Term(index, result);
  if (Storage::OK == error) {
    return Storage::OK;
  }
  if (Storage::ErrCompacted == error || Storage::ErrUnavailable == error) {
    *result = 0;
    return error;
  }

  //Panicf
  return error;
}

std::string RaftLog::String() const {
  char buffer[1024] = {0};
  snprintf(buffer, 1024,
           "committed=%lu, applied=%lu, unstable.first=%lu, unstable.Size=%lu",
           committed_, applied_, unstable_.First(), unstable_.Size());
  return buffer;
}

uint64_t RaftLog::ZeroTermOnErrCompacted(uint64_t term, Storage::Error error) {
  if (Storage::OK == error) {
    return term;
  }

  if (Storage::ErrCompacted == error) {
    return 0;
  }

  //Panicf
  return 0;
}

uint64_t RaftLog::Append(const EntrySlice& entries) {
  if (0 == entries.Size()) {
    return LastIndex();
  }

  uint64_t after = entries[0].index() - 1;
  if (after < committed_) {
    //Panicf
  }

  unstable_.TruncateAndAppend(entries);
  return LastIndex();
}

uint64_t RaftLog::FindConflict(const EntrySlice& entries) {
  uint64_t last_index = LastIndex();
  for (int i = 0, entries_size = entries.Size(); i < entries_size; i++) {
    const raftpb::Entry& entry = entries[i];
    if (!MatchTerm(entry.index(), entry.term())) {
      if (entry.index() <= last_index) {
        //Infof
      }
      return entry.index();
    }
  }

  return 0;
}

Storage::Error RaftLog::Slice(uint64_t low, uint64_t high, Entries* entries) {
  entries->Clear();

  auto error = MustCheckOutOfBounds(low, high);
  if (Storage::OK != error) {
    return error;
  }

  if (low == high) {
    return Storage::OK;
  }

  if (low < unstable_.First()) {
    auto error = storage_->GetEntries(low, std::min(high, unstable_.First()), entries);
    if (Storage::ErrCompacted == error) {
      return error;
    } else if (Storage::ErrUnavailable == error) {
      //Panicf
    } else if (Storage::OK != error) {
      //Panicf
    }

    if (static_cast<uint64_t>(entries->size()) < std::min(high, unstable_.First()) - low) {
      return Storage::OK;
    }
  }

  if (high > unstable_.First()) {
    unstable_.Slice(std::max(low, unstable_.First()), high, entries);
  }

  return Storage::OK;
}

Storage::Error RaftLog::MustCheckOutOfBounds(uint64_t low, uint64_t high) const {
  if (low > high) {
    //Panicf
  }

  uint64_t first_index = FirstIndex();
  if (low < first_index) {
    return Storage::ErrCompacted;
  }

  uint64_t last_index = LastIndex();
  if (low < first_index || high > last_index + 1) {
    //Panicf
  }

  return Storage::OK;
}

std::unique_ptr<RaftLog> NewRaftLog(const std::shared_ptr<Storage>& storage) {
  if (nullptr == storage.get()) {
    //Panic
  }

  uint64_t first_index = 0;
  auto error = storage->FirstIndex(&first_index);
  if (Storage::OK != error) {
    //Panic
  }

  uint64_t last_index = 0;
  error = storage->LastIndex(&last_index);
  if (Storage::OK != error) {
    //Panic
  }

  return myutil::make_unique<RaftLog>(storage, first_index, last_index);
}

} // namespace myraft
