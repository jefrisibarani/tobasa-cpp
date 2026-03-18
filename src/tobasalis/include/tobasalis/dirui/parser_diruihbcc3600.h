#pragma once

#include "tobasalis/lis/parser.h"

namespace tbs {
namespace dirui {

//class Message;

/** \ingroup LIS
 * DirUiBcc3600Parser
 */
class ParserDirUiBcc3600
   : public lis::Parser
{
public:
   ParserDirUiBcc3600(bool enableLog=false);
   ~ParserDirUiBcc3600() = default;
   void parse(const std::string &data = "");

private:
   bool _enableLog = false; 
};

} // namespace dirui
} // namespace tbs