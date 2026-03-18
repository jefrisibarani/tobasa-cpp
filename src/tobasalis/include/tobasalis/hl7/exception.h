#pragma once

#include <stdexcept>
#include <string>

namespace tbs {
namespace hl7 {

static const std::string ERR_REQUIRED_FIELD_MISSING   = "Validation Error - Required field missing in message";
static const std::string ERR_UNSUPPORTED_MESSAGE_TYPE = "Validation Error - Message Type not supported by this implementation";
static const std::string ERR_BAD_MESSAGE              = "Validation Error - Bad Message";
static const std::string ERR_PARSING_ERROR            = "Parsing Error";
static const std::string ERR_SERIALIZATION_ERROR      = "Serialization Error";

class HL7Exception : public std::runtime_error
{
private:
   std::string _errorCode;

public:

   HL7Exception(const std::string& message)
      : std::runtime_error(message.c_str())
   {}

   HL7Exception(const std::string& message, const std::string& code)
      : std::runtime_error(message.c_str())
   {
      _errorCode = code;
   }
};

} // namespace hl7
} // namespace tbs
