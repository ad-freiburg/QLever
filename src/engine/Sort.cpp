// Copyright 2015, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)

#include "./Sort.h"
#include <sstream>
#include "CallFixedSize.h"
#include "QueryExecutionTree.h"

using std::string;

// _____________________________________________________________________________
size_t Sort::getResultWidth() const { return _subtree->getResultWidth(); }

// _____________________________________________________________________________
Sort::Sort(QueryExecutionContext* qec,
           std::shared_ptr<QueryExecutionTree> subtree, size_t sortCol)
    : Operation(qec), _subtree(std::move(subtree)), _sortCol(sortCol) {}

// _____________________________________________________________________________
string Sort::asString(size_t indent) const {
  std::ostringstream os;
  for (size_t i = 0; i < indent; ++i) {
    os << " ";
  }

  std::stringstream columns;
  // TODO<joka921> This produces exactly the same format as ORDER BY operations
  // which is crucial for caching. Please refactor those classes to one class
  // (this is only an optimization for sorts on a single column);
  columns << "asc(" << _sortCol << ") ";
  os << "SORT / ORDER BY on columns:" << columns.str() << "\n"
     << _subtree->asString(indent);
  return os.str();
}

// _____________________________________________________________________________
string Sort::getDescriptor() const {
  std::string orderByVars;
  for (const auto& p : _subtree->getVariableColumns()) {
    if (p.second == _sortCol) {
      orderByVars = "ASC(" + p.first + ") ";
      break;
    }
  }

  return "Sort on (OrderBy) on " + orderByVars;
}

// _____________________________________________________________________________
void Sort::computeResult(ResultTable* result) {
  LOG(DEBUG) << "Getting sub-result for Sort result computation..." << endl;
  shared_ptr<const ResultTable> subRes = _subtree->getResult();

  // TODO<joka921> proper timeout for sorting operations
  double remainingSecs =
      static_cast<double>(_timeoutTimer->wlock()->remainingMicroseconds()) /
      1'000'000;
  if (getExecutionContext()
          ->getSortPerformanceEstimator()
          .estimatedSortTimeInSeconds(subRes->size(), subRes->width()) >
      remainingSecs * RuntimeParameters().get<"SORT_ESTIMATE_CANCELLATION_FACTOR">()) {
    // The estimated time for this sort is much larger than the actually
    // remaining time, cancel this operation
    throw ad_utility::TimeoutException(
        "Sort operation was canceled, because time estimate exceeded "
        "remaining time by a factor of " +
        std::to_string(SORT_ESTIMATE_CANCELLATION_FACTOR));
  }

  RuntimeInformation& runtimeInfo = getRuntimeInfo();
  runtimeInfo.addChild(_subtree->getRootOperation()->getRuntimeInfo());

  LOG(DEBUG) << "Sort result computation..." << endl;
  result->_data.setCols(subRes->_data.cols());
  result->_resultTypes.insert(result->_resultTypes.end(),
                              subRes->_resultTypes.begin(),
                              subRes->_resultTypes.end());
  result->_localVocab = subRes->_localVocab;
  result->_data.insert(result->_data.end(), subRes->_data.begin(),
                       subRes->_data.end());
  int width = result->_data.cols();
  CALL_FIXED_SIZE_1(width, Engine::sort, &result->_data, _sortCol);
  result->_sortedBy = resultSortedOn();

  LOG(DEBUG) << "Sort result computation done." << endl;
}
