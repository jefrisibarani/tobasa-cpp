#include <iomanip>
#include <thread>
#include <tobasa/logger.h>
#include "tobasalis/lis/connection.h"
#include "tobasalis/lis1a/datalink_gem3500.h"

namespace tbs {
namespace lis1a {

DataLinkGem3500::DataLinkGem3500(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit)
   : DataLinkStd(io_ctx, connection, limit) {}

bool DataLinkGem3500::checkChecksum(const std::string& line)
{
   bool result = false;
   auto lineLength = line.length();
   if (lineLength < 5)
      return result;

   if (line[0] != STX)
      return result;

   if (line[lineLength - 1] != LF)
      return result;

   if (line[lineLength - 2] != CR)
      return result;

   char etxOrEtb = line[lineLength - 5];
   if (etxOrEtb != ETX && etxOrEtb != ETB)
      return result;

   _lastFrameWasIntermediate = (etxOrEtb == ETB);
   
   //if (line[lineLength - 6] != CR)   // GEM3500, no <CR> before <ETB>
   //	return result;

   std::string tempChecksum   = line.substr(lineLength - 4, 2);
   std::string tempLine       = line.substr(1, lineLength - 5);
   std::string tempCheckSum2  = calculateChecksum(tempLine);

   return !(tempChecksum != tempCheckSum2) || result;
}

} // namespace lis1a
} // namespace tbs
