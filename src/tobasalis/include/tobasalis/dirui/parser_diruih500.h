#pragma once

#include "tobasalis/lis/parser.h"

namespace tbs {
namespace dirui {

//class Message;

/** \ingroup LIS
 * ParserDirUih500
 */
class ParserDirUih500
   : public lis::Parser
{
public:
   ParserDirUih500(bool enableLog=false);
   ~ParserDirUih500() = default;
   void parse(const std::string& data = "");

private:
   bool _enableLog = false;   
};

} // namespace dirui
} // namespace tbs