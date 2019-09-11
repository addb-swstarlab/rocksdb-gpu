//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once
#include <memory>
#include "db/range_tombstone_fragmenter.h"
#include "rocksdb/slice_transform.h"
#include "table/internal_iterator.h"

namespace rocksdb {

class Iterator;
struct ParsedInternalKey;
class Slice;
class Arena;
struct ReadOptions;
struct TableProperties;
class GetContext;

// A Table is a sorted map from strings to strings.  Tables are
// immutable and persistent.  A Table may be safely accessed from
// multiple threads without external synchronization.
class TableReader {
 public:
  virtual ~TableReader() {}

  // Returns a new iterator over the table contents.
  // The result of NewIterator() is initially invalid (caller must
  // call one of the Seek methods on the iterator before using it).
  // arena: If not null, the arena needs to be used to allocate the Iterator.
  //        When destroying the iterator, the caller will not call "delete"
  //        but Iterator::~Iterator() directly. The destructor needs to destroy
  //        all the states but those allocated in arena.
  // skip_filters: disables checking the bloom filters even if they exist. This
  //               option is effective only for block-based table format.
  virtual InternalIterator* NewIterator(const ReadOptions&,
                                        const SliceTransform* prefix_extractor,
                                        Arena* arena = nullptr,
                                        bool skip_filters = false,
                                        bool for_compaction = false) = 0;

  // RUDA
  // Returns all DataBlocks on SST file.
  virtual Status GetDataBlocks(const ReadOptions&,
                               std::vector<char>& /* data */,
                               std::vector<uint64_t>& /* seek_indices */) {
    return Status();
  };

  virtual Status GetDataBlocks(const ReadOptions&,
                               std::vector<char>& /* data */,
                               std::vector<uint64_t>& /* seek_indices */,
                               uint64_t /* seek_index_start_offset */) {
    return Status();
  };
  
  virtual size_t GetBlockSize() {
    return 0;
  };
  
  virtual Status GetBlockHandles(const ReadOptions&, std::vector<uint64_t> & /*handles*/) {
    return Status();
  };
  
  virtual FragmentedRangeTombstoneIterator* NewRangeTombstoneIterator(
      const ReadOptions& /*read_options*/) {
    return nullptr;
  }

  // Given a key, return an approximate byte offset in the file where
  // the data for that key begins (or would begin if the key were
  // present in the file).  The returned value is in terms of file
  // bytes, and so includes effects like compression of the underlying data.
  // E.g., the approximate offset of the last key in the table will
  // be close to the file length.
  virtual uint64_t ApproximateOffsetOf(const Slice& key) = 0;

  // Set up the table for Compaction. Might change some parameters with
  // posix_fadvise
  virtual void SetupForCompaction() = 0;

  virtual std::shared_ptr<const TableProperties> GetTableProperties() const = 0;

  // Prepare work that can be done before the real Get()
  virtual void Prepare(const Slice& /*target*/) {}

  // Report an approximation of how much memory has been used.
  virtual size_t ApproximateMemoryUsage() const = 0;

  // Calls get_context->SaveValue() repeatedly, starting with
  // the entry found after a call to Seek(key), until it returns false.
  // May not make such a call if filter policy says that key is not present.
  //
  // get_context->MarkKeyMayExist needs to be called when it is configured to be
  // memory only and the key is not found in the block cache.
  //
  // readOptions is the options for the read
  // key is the key to search for
  // skip_filters: disables checking the bloom filters even if they exist. This
  //               option is effective only for block-based table format.
  virtual Status Get(const ReadOptions& readOptions, const Slice& key,
                     GetContext* get_context,
                     const SliceTransform* prefix_extractor,
                     bool skip_filters = false) = 0;

  // virtual Status ValueFilter(const ReadOptions& /*readOptions*/, const Slice& /*key*/, const SlicewithSchema &/*schema*/,
  //                    GetContext* /*get_context*/,
  //                    const SliceTransform* /*prefix_extractor*/,
  //                    bool /*skip_filters = false*/) { return Status::OK(); }

  // Ruda
  virtual Status AvxFilter(const ReadOptions& /* readOptions */,
                           const Slice& /* key */,
                           const SlicewithSchema& /* schema_key */,
                           GetContext* /* get_context */,
                           const SliceTransform* /* prefix_extractor */,
                           bool /* skip_filters*/) {
    return Status();
  }

  virtual Status AvxFilterBlock(const ReadOptions& /* readOptions */,
                           const Slice& /* key */,
                           const SlicewithSchema& /* schema_key */,
                           GetContext* /* get_context */,
                           const SliceTransform* /* prefix_extractor */,
                           bool /* skip_filters*/) {
    return Status();
  }

  // Prefetch data corresponding to a give range of keys
  // Typically this functionality is required for table implementations that
  // persists the data on a non volatile storage medium like disk/SSD
  virtual Status Prefetch(const Slice* begin = nullptr,
                          const Slice* end = nullptr) {
    (void) begin;
    (void) end;
    // Default implementation is NOOP.
    // The child class should implement functionality when applicable
    return Status::OK();
  }

  // convert db file to a human readable form
  virtual Status DumpTable(WritableFile* /*out_file*/,
                           const SliceTransform* /*prefix_extractor*/) {
    return Status::NotSupported("DumpTable() not supported");
  }

  // check whether there is corruption in this db file
  virtual Status VerifyChecksum() {
    return Status::NotSupported("VerifyChecksum() not supported");
  }

  virtual void Close() {}
};

}  // namespace rocksdb
