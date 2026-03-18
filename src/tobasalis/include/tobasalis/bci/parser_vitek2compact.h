#pragma once

#include "tobasalis/lis/parser.h"

namespace tbs {
namespace bci {

class Record;

/** \ingroup LIS
 * Vitek2CompactParser
 */
class Vitek2CompactParser
   : public lis::Parser
{
public:
   Vitek2CompactParser(bool enableLog=false);
   ~Vitek2CompactParser() = default;
   void parse(const std::string& rawData = "");

private:
   // publish bci::Record to our subsscriber
   // convert bci::Record into lis::Record before publishing
   void publisRecord(const std::shared_ptr<Record>& record);

private:
   bool _enableLog = false;
};

} // namespace bci
} // namespace tbs 