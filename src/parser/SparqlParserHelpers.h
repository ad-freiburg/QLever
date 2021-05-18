//
// Created by johannes on 16.05.21.
//

#ifndef QLEVER_SPARQLPARSERHELPERS_H
#define QLEVER_SPARQLPARSERHELPERS_H

#include <memory>
#include <string>
#include "./SparqlExpressionWrapper.h"

namespace sparqlParserHelpers {

template <typename ParseResult>
struct ParseResultAndRemainingText {
  ParseResult _parseResult;
  std::string _remainingText;
};

ParseResultAndRemainingText<sparqlExpression::SparqlExpressionWrapper>
parseExpression(const std::string& input);

}  // namespace sparqlParserHelpers

#endif  // QLEVER_SPARQLPARSERHELPERS_H