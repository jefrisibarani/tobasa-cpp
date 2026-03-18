#pragma once

#include "tobasalis/lis/parser.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * VidasParser
 */
class VidasParser
   : public lis::Parser
{
public:
   VidasParser(bool enableLog=false);
   ~VidasParser() = default;
   void parse(const std::string& rawData = "");

private:
   bool _enableLog = false;
};

} // namespace bci
} // namespace tbs 