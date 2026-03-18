#pragma once

#include "tobasalis/lis/parser.h"

namespace tbs {
namespace hl7 {

class Message;
class HL7Record;

/** \ingroup LIS
 * ParserHl7
 */
class ParserHl7
   : public lis::Parser
{
public:
   ParserHl7(bool enableLog=false);
   ~ParserHl7() = default;
   void parse(const std::string& data = "");

private:
   bool _enableLog = false;
};

} // namespace hl7
} // namespace tbs