#pragma once

#include "tobasalis/exp_imp.h"

#include "tobasalis/lis/parser.h"

namespace tbs {
namespace bci {

/** \ingroup LIS
 * VidasParser
 */
class TBS_LIS_API VidasParser
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