// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Johannes Kalmbach <johannes.kalmbach@gmail.com>
#pragma once

#include <string>
#include <sparsehash/sparse_hash_map>

#include "../global/Id.h"
#include "../global/Constants.h"

using std::string;
// _______________________________________________________________
void mergeVocabulary(const std::string& basename, size_t numFiles);

// __________________________________________________________________________________________
google::sparse_hash_map<string, Id> vocabMapFromPartialIndexedFile(const string& partialFile);
