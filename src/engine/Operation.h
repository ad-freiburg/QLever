// Copyright 2015, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)
#pragma once

#include <iomanip>
#include <iostream>
#include <memory>
#include <utility>

#include "../util/Exception.h"
#include "../util/Log.h"
#include "../util/Timer.h"
#include "QueryExecutionContext.h"
#include "ResultTable.h"
#include "RuntimeInformation.h"

using std::endl;
using std::pair;
using std::shared_ptr;

// forward declaration needed to break dependencies
class QueryExecutionTree;

class Operation {
 public:
  // Default Constructor.
  Operation() : _executionContext(nullptr), _hasComputedSortColumns(false) {}

  // Typical Constructor.
  explicit Operation(QueryExecutionContext* executionContext)
      : _executionContext(executionContext), _hasComputedSortColumns(false) {}

  // Destructor.
  virtual ~Operation() {
    // Do NOT delete _executionContext, since
    // there is no ownership.
  }

  /// get non-owning pointers to all the held subtrees to actually use the
  /// Execution Trees as trees
  virtual std::vector<QueryExecutionTree*> getChildren() = 0;

  /// get non-owning constant pointers to all the held subtrees to actually use
  /// the Execution Trees as trees
  std::vector<const QueryExecutionTree*> getChildren() const {
    vector<QueryExecutionTree*> interm{
        const_cast<Operation*>(this)->getChildren()};
    return {interm.begin(), interm.end()};
  }

  // recursively collect all Warnings generated by all descendants
  vector<string> collectWarnings() const;

  /**
   * Abort this Operation.  Removes the Operation's result from the cache so
   * that it can be retried. The result must be owned meaning only the
   * computing thread can abort an Operation. Retrying may succeed for example
   * when memory pressure was lowered in the meantime.  When print is true the
   * Operation is printed to the ERROR LOG
   */
  void abort(const shared_ptr<CacheValue>& cachedResult, bool print) {
    const std::string opString = asString();
    if (print) {
      LOG(ERROR) << "Aborted Operation:" << endl;
      LOG(ERROR) << opString << endl;
    }
    // Remove Operation from cache so we may retry it later. Anyone with a live
    // pointer will be waiting and register the abort.
    _executionContext->getQueryTreeCache().erase(opString);
    cachedResult->_resTable->abort();
  }

  /**
   * @return A list of columns on which the result of this operation is sorted.
   */
  const vector<size_t>& getResultSortedOn() {
    if (!_hasComputedSortColumns) {
      _hasComputedSortColumns = true;
      _resultSortedColumns = resultSortedOn();
    }
    return _resultSortedColumns;
  }

  const Index& getIndex() const { return _executionContext->getIndex(); }

  const Engine& getEngine() const { return _executionContext->getEngine(); }

  // Get a unique, not ambiguous string representation for a subtree.
  // This should possible act like an ID for each subtree.
  virtual string asString(size_t indent = 0) const = 0;

  // Gets a very short (one line without line ending) descriptor string for
  // this Operation.  This string is used in the RuntimeInformation
  virtual string getDescriptor() const = 0;
  virtual size_t getResultWidth() const = 0;
  virtual void setTextLimit(size_t limit) = 0;
  virtual size_t getCostEstimate() = 0;
  virtual size_t getSizeEstimate() = 0;
  virtual float getMultiplicity(size_t col) = 0;
  virtual bool knownEmptyResult() = 0;
  virtual ad_utility::HashMap<string, size_t> getVariableColumns() const = 0;

  RuntimeInformation& getRuntimeInfo() { return _runtimeInfo; }

  // Get the result for the subtree rooted at this element.
  // Use existing results if they are already available, otherwise
  // trigger computation.
  shared_ptr<const ResultTable> getResult(bool isRoot = false);

  // typedef for a synchronized and shared timeoutTimer
  using SyncTimer = ad_utility::TimeoutChecker;
  // set a global timeout timer for all child operations.
  // As soon as this runs out, the complete tree will fail.
  void recursivelySetTimeoutTimer(std::shared_ptr<SyncTimer> timer);

 protected:
  QueryExecutionContext* getExecutionContext() const {
    return _executionContext;
  }

  // The QueryExecutionContext for this particular element.
  // No ownership.
  QueryExecutionContext* _executionContext;

  /**
   * @brief Allows for updating of the sorted columns of an operation. This
   *        has to be used by an operation if it's sort columns change during
   *        the operations lifetime.
   */
  void setResultSortedOn(const vector<size_t>& sortedColumns) {
    _resultSortedColumns = sortedColumns;
  }

  /**
   * @brief Compute and return the columns on which the result will be sorted
   * @return The columns on which the result will be sorted.
   */
  [[nodiscard]] virtual vector<size_t> resultSortedOn() const = 0;

  /// interface to the generated warnings of this operation
  std::vector<std::string>& getWarnings() { return _warnings; }
  [[nodiscard]] const std::vector<std::string>& getWarnings() const {
    return _warnings;
  }

  // check if we still have time left on the clock.
  // if not, throw a TimeoutException
  void checkTimeout() const;

  // handles the timeout of this operation
  std::shared_ptr<SyncTimer> _timeoutTimer =
      std::make_shared<SyncTimer>(ad_utility::TimeoutTimer::unlimited());

 private:
  //! Compute the result of the query-subtree rooted at this element..
  //! Computes both, an EntityList and a HitList.
  virtual void computeResult(ResultTable* result) = 0;

  vector<size_t> _resultSortedColumns;
  RuntimeInformation _runtimeInfo;

  bool _hasComputedSortColumns;

  /// collect all the warnings that were created during the creation or
  /// execution of this operation
  std::vector<std::string> _warnings;

  // recursively call a function on all children
  template <typename F>
  void forAllDescendants(F f);

  // recursively call a function on all children
  template <typename F>
  void forAllDescendants(F f) const;
};
