// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Florian Kramer (florian.kramer@mail.uni-freiburg.de)

#include <gtest/gtest.h>
#include <algorithm>
#include <cstdio>
#include "../src/engine/CallFixedSize.h"
#include "../src/engine/CountAvailablePredicates.h"
#include "../src/engine/HasPredicateScan.h"

// used to test HasRelationScan with a subtree
class DummyOperation : public Operation {
 public:
  DummyOperation(QueryExecutionContext* ctx) : Operation(ctx) {}
  virtual void computeResult(ResultTable* result) override {
    result->_resultTypes.push_back(ResultTable::ResultType::KB);
    result->_resultTypes.push_back(ResultTable::ResultType::KB);
    result->_data.setCols(2);
    for (size_t i = 0; i < 10; i++) {
      result->_data.push_back({10 - i, 2 * i});
    }
    result->finish();
  }

  string asString(size_t indent = 0) const override {
    (void)indent;
    return "dummy";
  }

  string getDescriptor() const override { return "dummy"; }

  virtual size_t getResultWidth() const override { return 2; }

  virtual vector<size_t> resultSortedOn() const override { return {1}; }

  virtual void setTextLimit(size_t limit) override { (void)limit; }

  virtual size_t getCostEstimate() override { return 10; }

  virtual size_t getSizeEstimate() override { return 10; }

  virtual float getMultiplicity(size_t col) override {
    (void)col;
    return 1;
  }

  virtual bool knownEmptyResult() override { return false; }

  virtual ad_utility::HashMap<string, size_t> getVariableColumns()
      const override {
    return {{"?a", 0}, {"?b", 1}};
  }
};

TEST(HasPredicateScan, freeS) {
  // Used to store the result.
  ResultTable resultTable;
  resultTable._data.setCols(1);
  // Maps entities to their patterns. If an entity id is higher than the lists
  // length the hasRelation relation is used instead.
  vector<PatternID> hasPattern = {0, NO_PATTERN, NO_PATTERN, 1, 0};
  // The has relation relation, which is used when an entity does not have a
  // pattern
  vector<vector<Id>> hasRelationSrc = {{},     {0, 3}, {0},    {}, {},
                                       {0, 3}, {3, 4}, {2, 4}, {3}};
  // Maps pattern ids to patterns
  vector<vector<Id>> patternsSrc = {{0, 2, 3}, {1, 3, 4, 2, 0}};

  // These are used to store the relations and patterns in contiguous blocks
  // of memory.
  CompactStringVector<Id, Id> hasRelation(hasRelationSrc);
  CompactStringVector<size_t, Id> patterns(patternsSrc);

  // Find all entities that are in a triple with predicate 3
  HasPredicateScan::computeFreeS(&resultTable, 3, hasPattern, hasRelation,
                                 patterns);
  IdTable& result = resultTable._data;

  // the result set does not guarantee any sorting so we have to sort manually
  std::sort(result.begin(), result.end(),
            [](const auto& a, const auto& b) { return a[0] < b[0]; });

  // three entties with a pattern and four entities without one are in the
  // relation
  ASSERT_EQ(7u, result.size());
  ASSERT_EQ(0u, result[0][0]);
  ASSERT_EQ(1u, result[1][0]);
  ASSERT_EQ(3u, result[2][0]);
  ASSERT_EQ(4u, result[3][0]);
  ASSERT_EQ(5u, result[4][0]);
  ASSERT_EQ(6u, result[5][0]);
  ASSERT_EQ(8u, result[6][0]);
}

TEST(HasPredicateScan, freeO) {
  // Used to store the result.
  ResultTable resultTable;
  resultTable._data.setCols(1);
  // Maps entities to their patterns. If an entity id is higher than the lists
  // length the hasRelation relation is used instead.
  vector<PatternID> hasPattern = {0, NO_PATTERN, NO_PATTERN, 1, 0};
  // The has relation relation, which is used when an entity does not have a
  // pattern
  vector<vector<Id>> hasRelationSrc = {{},     {0, 3}, {0},    {}, {},
                                       {0, 3}, {3, 4}, {2, 4}, {3}};
  // Maps pattern ids to patterns
  vector<vector<Id>> patternsSrc = {{0, 2, 3}, {1, 3, 4, 2, 0}};

  // These are used to store the relations and patterns in contiguous blocks
  // of memory.
  CompactStringVector<Id, Id> hasRelation(hasRelationSrc);
  CompactStringVector<size_t, Id> patterns(patternsSrc);

  // Find all predicates for entity 3 (pattern 1)
  HasPredicateScan::computeFreeO(&resultTable, 3, hasPattern, hasRelation,
                                 patterns);
  IdTable& result = resultTable._data;

  ASSERT_EQ(5u, result.size());
  ASSERT_EQ(1u, result[0][0]);
  ASSERT_EQ(3u, result[1][0]);
  ASSERT_EQ(4u, result[2][0]);
  ASSERT_EQ(2u, result[3][0]);
  ASSERT_EQ(0u, result[4][0]);

  resultTable._data.clear();

  // Find all predicates for entity 6 (has-relation entry 6)
  HasPredicateScan::computeFreeO(&resultTable, 6, hasPattern, hasRelation,
                                 patterns);

  ASSERT_EQ(2u, result.size());
  ASSERT_EQ(3u, result[0][0]);
  ASSERT_EQ(4u, result[1][0]);
}

TEST(HasPredicateScan, fullScan) {
  // Used to store the result.
  ResultTable resultTable;
  resultTable._data.setCols(2);
  // Maps entities to their patterns. If an entity id is higher than the lists
  // length the hasRelation relation is used instead.
  vector<PatternID> hasPattern = {0, NO_PATTERN, NO_PATTERN, 1, 0};
  // The has relation relation, which is used when an entity does not have a
  // pattern
  vector<vector<Id>> hasRelationSrc = {{}, {0, 3}, {0}, {}, {}, {0, 3}};
  // Maps pattern ids to patterns
  vector<vector<Id>> patternsSrc = {{0, 2, 3}, {1, 3, 4, 2, 0}};

  // These are used to store the relations and patterns in contiguous blocks
  // of memory.
  CompactStringVector<Id, Id> hasRelation(hasRelationSrc);
  CompactStringVector<size_t, Id> patterns(patternsSrc);

  // Query for all relations
  HasPredicateScan::computeFullScan(&resultTable, hasPattern, hasRelation,
                                    patterns, 16);
  IdTable& result = resultTable._data;

  ASSERT_EQ(16u, result.size());

  // check the entity ids
  ASSERT_EQ(0u, result[0][0]);
  ASSERT_EQ(0u, result[1][0]);
  ASSERT_EQ(0u, result[2][0]);
  ASSERT_EQ(1u, result[3][0]);
  ASSERT_EQ(1u, result[4][0]);
  ASSERT_EQ(2u, result[5][0]);
  ASSERT_EQ(3u, result[6][0]);
  ASSERT_EQ(3u, result[7][0]);
  ASSERT_EQ(3u, result[8][0]);
  ASSERT_EQ(3u, result[9][0]);
  ASSERT_EQ(3u, result[10][0]);
  ASSERT_EQ(4u, result[11][0]);
  ASSERT_EQ(4u, result[12][0]);
  ASSERT_EQ(4u, result[13][0]);
  ASSERT_EQ(5u, result[14][0]);
  ASSERT_EQ(5u, result[15][0]);

  // check the predicate ids
  ASSERT_EQ(0u, result[0][1]);
  ASSERT_EQ(2u, result[1][1]);
  ASSERT_EQ(3u, result[2][1]);
  ASSERT_EQ(0u, result[3][1]);
  ASSERT_EQ(3u, result[4][1]);
  ASSERT_EQ(0u, result[5][1]);
  ASSERT_EQ(1u, result[6][1]);
  ASSERT_EQ(3u, result[7][1]);
  ASSERT_EQ(4u, result[8][1]);
  ASSERT_EQ(2u, result[9][1]);
  ASSERT_EQ(0u, result[10][1]);
  ASSERT_EQ(0u, result[11][1]);
  ASSERT_EQ(2u, result[12][1]);
  ASSERT_EQ(3u, result[13][1]);
  ASSERT_EQ(0u, result[14][1]);
  ASSERT_EQ(3u, result[15][1]);
}

TEST(HasPredicateScan, subtreeS) {
  // Used to store the result.
  ResultTable resultTable;
  resultTable._data.setCols(3);
  // Maps entities to their patterns. If an entity id is higher than the lists
  // length the hasRelation relation is used instead.
  vector<PatternID> hasPattern = {0, NO_PATTERN, NO_PATTERN, 1, 0};
  // The has relation relation, which is used when an entity does not have a
  // pattern
  vector<vector<Id>> hasRelationSrc = {{},     {0, 3}, {0},    {}, {},
                                       {0, 3}, {3, 4}, {2, 4}, {3}};
  // Maps pattern ids to patterns
  vector<vector<Id>> patternsSrc = {{0, 2, 3}, {1, 3, 4, 2, 0}};

  // These are used to store the relations and patterns in contiguous blocks
  // of memory.
  CompactStringVector<Id, Id> hasRelation(hasRelationSrc);
  CompactStringVector<size_t, Id> patterns(patternsSrc);

  Index index;
  Engine engine;
  SubtreeCache cache(NOF_SUBTREES_TO_CACHE);
  QueryExecutionContext ctx(index, engine, &cache);

  // create the subtree operation
  std::shared_ptr<QueryExecutionTree> subtree =
      std::make_shared<QueryExecutionTree>(&ctx);
  auto operation = std::make_unique<DummyOperation>(&ctx);

  subtree->setOperation(QueryExecutionTree::OperationType::HAS_RELATION_SCAN,
                        std::move(operation));

  std::shared_ptr<const ResultTable> subresult = subtree->getResult();
  int in_width = 2;
  int out_width = 3;
  CALL_FIXED_SIZE_2(in_width, out_width, HasPredicateScan::computeSubqueryS,
                    &resultTable._data, subresult->_data, 1, hasPattern,
                    hasRelation, patterns);

  IdTable& result = resultTable._data;

  // the sum of the count of every second entities relations
  ASSERT_EQ(10u, result.size());

  // check for the first column

  // check for the entity ids
  ASSERT_EQ(10u, result[0][0]);
  ASSERT_EQ(10u, result[1][0]);
  ASSERT_EQ(10u, result[2][0]);
  ASSERT_EQ(9u, result[3][0]);
  ASSERT_EQ(8u, result[4][0]);
  ASSERT_EQ(8u, result[5][0]);
  ASSERT_EQ(8u, result[6][0]);
  ASSERT_EQ(7u, result[7][0]);
  ASSERT_EQ(7u, result[8][0]);
  ASSERT_EQ(6u, result[9][0]);

  // check for the entity ids
  ASSERT_EQ(0u, result[0][1]);
  ASSERT_EQ(0u, result[1][1]);
  ASSERT_EQ(0u, result[2][1]);
  ASSERT_EQ(2u, result[3][1]);
  ASSERT_EQ(4u, result[4][1]);
  ASSERT_EQ(4u, result[5][1]);
  ASSERT_EQ(4u, result[6][1]);
  ASSERT_EQ(6u, result[7][1]);
  ASSERT_EQ(6u, result[8][1]);
  ASSERT_EQ(8u, result[9][1]);

  // check for the predicate ids
  ASSERT_EQ(0u, result[0][2]);
  ASSERT_EQ(2u, result[1][2]);
  ASSERT_EQ(3u, result[2][2]);
  ASSERT_EQ(0u, result[3][2]);
  ASSERT_EQ(0u, result[4][2]);
  ASSERT_EQ(2u, result[5][2]);
  ASSERT_EQ(3u, result[6][2]);
  ASSERT_EQ(3u, result[7][2]);
  ASSERT_EQ(4u, result[8][2]);
  ASSERT_EQ(3u, result[9][2]);
}

TEST(CountAvailablePredicates, patternTrickTest) {
  // The input table containing entity ids
  IdTable input(1);
  for (Id i = 0; i < 8; i++) {
    input.push_back({i});
  }
  // Used to store the result.
  IdTable result(2);
  // Maps entities to their patterns. If an entity id is higher than the lists
  // length the hasRelation relation is used instead.
  vector<PatternID> hasPattern = {0, NO_PATTERN, NO_PATTERN, 1, 0};
  // The has relation relation, which is used when an entity does not have a
  // pattern
  vector<vector<Id>> hasRelationSrc = {{},     {0, 3}, {0},    {}, {},
                                       {0, 3}, {3, 4}, {2, 4}, {3}};
  // Maps pattern ids to patterns
  vector<vector<Id>> patternsSrc = {{0, 2, 3}, {1, 3, 4, 2, 0}};

  // These are used to store the relations and patterns in contiguous blocks
  // of memory.
  CompactStringVector<Id, Id> hasRelation(hasRelationSrc);
  CompactStringVector<size_t, Id> patterns(patternsSrc);

  RuntimeInformation runtimeInfo;
  try {
    CALL_FIXED_SIZE_1(
        input.cols(), CountAvailablePredicates::computePatternTrick, input,
        &result, hasPattern, hasRelation, patterns, 0, &runtimeInfo);
  } catch (const std::runtime_error& e) {
    // More verbose output in the case of an exception occuring.
    std::cout << e.what() << std::endl;
    ASSERT_TRUE(false);
  }

  std::sort(
      result.begin(), result.end(),
      [](const auto& i1, const auto& i2) -> bool { return i1[0] < i2[0]; });
  ASSERT_EQ(5u, result.size());

  ASSERT_EQ(0u, result(0, 0));
  ASSERT_EQ(6u, result(0, 1));

  ASSERT_EQ(1u, result(1, 0));
  ASSERT_EQ(1u, result(1, 1));

  ASSERT_EQ(2u, result(2, 0));
  ASSERT_EQ(4u, result(2, 1));

  ASSERT_EQ(3u, result(3, 0));
  ASSERT_EQ(6u, result(3, 1));

  ASSERT_EQ(4u, result(4, 0));
  ASSERT_EQ(3u, result(4, 1));

  //  ASSERT_EQ(0u, result[0][0]);
  //  ASSERT_EQ(5u, result[0][1]);
  //
  //  ASSERT_EQ(1u, result[1][0]);
  //  ASSERT_EQ(1u, result[1][1]);
  //
  //  ASSERT_EQ(2u, result[2][0]);
  //  ASSERT_EQ(4u, result[2][1]);
  //
  //  ASSERT_EQ(3u, result[3][0]);
  //  ASSERT_EQ(5u, result[3][1]);
  //
  //  ASSERT_EQ(4u, result[4][0]);
  //  ASSERT_EQ(3u, result[4][1]);

  // Test the pattern trick for all entities
  result.clear();
  try {
    CountAvailablePredicates::computePatternTrickAllEntities(
        &result, hasPattern, hasRelation, patterns);
  } catch (const std::runtime_error& e) {
    // More verbose output in the case of an exception occuring.
    std::cout << e.what() << std::endl;
    ASSERT_TRUE(false);
  }
  ASSERT_EQ(5u, result.size());

  ASSERT_EQ(0u, result[0][0]);
  ASSERT_EQ(6u, result[0][1]);

  ASSERT_EQ(1u, result[1][0]);
  ASSERT_EQ(1u, result[1][1]);

  ASSERT_EQ(2u, result[2][0]);
  ASSERT_EQ(4u, result[2][1]);

  ASSERT_EQ(3u, result[3][0]);
  ASSERT_EQ(7u, result[3][1]);

  ASSERT_EQ(4u, result[4][0]);
  ASSERT_EQ(3u, result[4][1]);
}
