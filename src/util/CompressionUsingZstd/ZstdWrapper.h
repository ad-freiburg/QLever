// Copyright 2021, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Johannes Kalmbach <johannes.kalmbach@gmail.com>


#pragma once

#include <zstd.h>
#include <vector>
#include <thread>
#include <span>
#include "../Exception.h"
#include "../WorkQueue.h"


class ZstdWrapper {
 public:
  static constexpr auto noop = []([[maybe_unused]]auto&&... args) {};

  template <typename Callback>
  static void compress (void* src, size_t numBytes, Callback whatToDoWithResult, int compressionLevel = 3) {
    std::vector<char> result(ZSTD_compressBound(numBytes));
    auto compressedSize = ZSTD_compress(result.data(), result.size(), src, numBytes, compressionLevel);
    result.resize(compressedSize);
    whatToDoWithResult(std::move(result));
  }

  template <typename Callback>
  static void compressInChunks (char* src, size_t numBytes, Callback whatToDoWithResult, size_t  numThreads, int compressionLevel = 3, size_t frameSize = 1ul << 23) {
    // need at least one thread for compression, and one for writing
    AD_CHECK(numThreads >= 2);
    // TODO: Remove Magic Constants

    struct CompressedInput {
      std::vector<char> _compressedBytes;
      size_t index;
    };
    auto getIndex = [](const CompressedInput c) {return c.index;};
    ad_utility::WorkQueue<CompressedInput, true, decltype(getIndex)> writeQueue{100};
    std::jthread writeThread {[&writeQueue, &whatToDoWithResult] {
      while (auto nextOptionalCompressedFrame = writeQueue.pop()) {

        whatToDoWithResult(nextOptionalCompressedFrame->_compressedBytes);
    }}};

    struct ChunkAndIndex {
      std::span<char> chunk;
      size_t index;
    };
    ad_utility::WorkQueue<ChunkAndIndex> toCompressQueue{100};

    std::atomic<size_t> nextChunkNum = 0;
    std::atomic<size_t> nextChunkNumToPush = 0;
    std::vector<std::jthread> compressors;
    for (size_t i = 0; i < numThreads - 1; ++i){
      compressors.emplace_back(
      [&]{
            while (auto optionalSpan = toCompressQueue.pop()) {
          std::vector<char> compressedResult;
          auto writeToResult = [&](auto result) {compressedResult = std::move(result);};
          compress(optionalSpan->chunk.data(), optionalSpan->chunk.size(), writeToResult, compressionLevel);
          writeQueue.push(CompressedInput{std::move(compressedResult), optionalSpan->index});
        }
      }
    );
  }


    size_t numChunks = 0;
    while (numBytes > 0) {
      size_t actualNumBytes = std::min(numBytes, frameSize);
      toCompressQueue.push(ChunkAndIndex{{src, numChunks++}});
      numBytes -= actualNumBytes;
      src += actualNumBytes;
    }

    toCompressQueue.finish();
    for (auto& thread : compressors) {
      thread.join();
    }
    // The compression is complete, so all the indices should be in the writeQueu
    writeQueue.finish();

    // The destructor of the writing thread blocks here until everything has written.
  }



  template<typename T> requires (std::is_trivially_copyable_v<T>)
  static std::vector<T> decompress (void* src, size_t numBytes, size_t knownOriginalSize) {
    knownOriginalSize *= sizeof(T);
    std::vector<T> result(knownOriginalSize / sizeof(T));
    auto compressedSize = ZSTD_decompress(result.data(), knownOriginalSize, src, numBytes);
    AD_CHECK(compressedSize == knownOriginalSize);
    return result;
  }
};
