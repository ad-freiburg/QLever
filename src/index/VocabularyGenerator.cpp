#include "./VocabularyGenerator.h"
#include <unordered_set>
#include <vector>
#include <queue>
#include <string>
#include <utility>
#include <fstream>
#include <iostream>

#include "../util/Log.h"

class PairCompare {
 public:
  bool operator()(const std::pair<std::string, size_t>& p1, const std::pair<std::string, size_t>& p2) {
    return p1.first > p2.first;}
};
// ___________________________________________________________________
void mergeVocabulary(const std::string& basename, size_t numFiles) {
  size_t bufferSize = 200000;
  std::vector<std::fstream> infiles;
  std::vector<std::vector<std::string>> vecs(numFiles);
  std::ofstream outfile(basename + ".vocabulary");
  std::ofstream outfileExternal(basename + "externalTextFile");
  std::vector<bool> endOfFile(numFiles, false);
  std::vector<std::streampos> posNextWord;
  std::vector<std::vector<std::string>::iterator> iterators;
  for (size_t i = 0; i < numFiles; i++) {
    infiles.emplace_back(basename + "partialVocabulary" + std::to_string(i), std::ios_base::in | std::ios_base::out);
    endOfFile[i] = true;
    size_t numRead = 0;
    vecs[i].reserve(bufferSize);

    unsigned int len;
    if (infiles[i].read((char*)&len, sizeof(len))) {
      vecs[i].emplace_back();
      vecs[i].back().resize(len);
      infiles[i].read(&(vecs[i].back()[0]), len);
      endOfFile[i] = false;
    
    }
  }


  using pair_T = std::pair<std::string, size_t>;
  std::priority_queue<pair_T, std::vector<pair_T>, PairCompare> queue;
  for (size_t i = 0; i < numFiles; i++) {
    if (!vecs[i].empty()) {
      queue.push(std::make_pair(vecs[i].front(), i));
      //std::cout << vecs[i].front() << i << std::endl;;
    }
    iterators.push_back(vecs[i].begin());
  }

  std::string lastWritten = "";
  size_t totalWritten = 0;

  while (! queue.empty()) {
    auto top = queue.top();
    queue.pop();


    // avoid duplicates
    if (top.first != lastWritten) {
      lastWritten = top.first;
      if (top.first < string({EXTERNALIZED_LITERALS_PREFIX})) {
        outfile << top.first << std::endl;
      } else {
        outfileExternal << top.first << std::endl;
      }
      infiles[top.second].write((char*)&totalWritten, sizeof(totalWritten));
      totalWritten++;
      //std::cout << "outputting " << top.first << " from " << top.second << std::endl;
    } else {
      // always write Index (also in case of duplicates)
      // we already have increased total written, so for the duplicate
      // we have to subtract one again
      infiles[top.second].write((char*)&totalWritten, sizeof(totalWritten));

    }

    // refill with top element from current vector
    iterators[top.second]++;
    if (iterators[top.second] == vecs[top.second].end()) {
      if (!endOfFile[top.second]) {
        // if above condition is false, we have no more words for file i, just
        // skip

        // refill vector from file
        vecs[top.second].clear();
        auto i = top.second;
        size_t numRead = 0;
        std::string word;
        endOfFile[top.second] = true;
        unsigned int len;
        if (infiles[i].read((char*)&len, sizeof(len))) {
          vecs[i].emplace_back();
          vecs[i].back().resize(len);
          infiles[i].read(&(vecs[i].back()[0]), len);
          endOfFile[i] = false;
        }
        iterators[i] = vecs[i].begin();
        if (vecs[i].begin() != vecs[i].end()) {
          queue.push(std::make_pair(vecs[i].front(), i));
        }
      }
    } else {
      queue.push(std::make_pair(*(iterators[top.second]), top.second));
    }
  }
}

// ____________________________________________________________________________________________
google::sparse_hash_map<string, Id> vocabMapFromPartialIndexedFile(const string& partialFile) {
  std::ifstream file(partialFile, std::ios_base::binary);
  google::sparse_hash_map<string, Id> vocabMap;
  unsigned int len;
  while (file.read((char*)&len, sizeof(len))) {
    std::string word;
    word.resize(len);
    file.read(&(word[0]), len);
    size_t idx;
    file.read((char*)&idx, sizeof(idx));
    vocabMap[word] = idx;
  }
  return vocabMap;
}

