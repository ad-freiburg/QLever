// Copyright 2015, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)

#include "./ResultTable.h"
#include <cassert>

// _____________________________________________________________________________
ResultTable::ResultTable(ad_utility::AllocatorWithLimit<Id> allocator)
    : _sortedBy(),
      _data(std::move(allocator)),
      _resultTypes(),
      _localVocab(std::make_shared<std::vector<std::string>>()) {}

// _____________________________________________________________________________
void ResultTable::clear() {
  _localVocab = nullptr;
  _data.clear();
}

// _____________________________________________________________________________
ResultTable::~ResultTable() { clear(); }

// _____________________________________________________________________________
string ResultTable::asDebugString() const {
  std::ostringstream os;
  os << "First (up to) 5 rows of result with size:\n";
  for (size_t i = 0; i < std::min<size_t>(5, _data.size()); ++i) {
    for (size_t j = 0; j < _data.cols(); ++j) {
      os << _data(i, j) << '\t';
    }
    os << '\n';
  }
  return os.str();
}

// _____________________________________________________________________________
size_t ResultTable::size() const { return _data.size(); }
