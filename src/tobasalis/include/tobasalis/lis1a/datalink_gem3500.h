#pragma once

#include "tobasalis/exp_imp.h"

#include "tobasalis/lis1a/datalink_std.h"

namespace tbs {
namespace lis1a {

/** \ingroup LIS
 * DataLinkGem3500
 */
class TBS_LIS_API DataLinkGem3500
   : public DataLinkStd
{
public:

   /**
    * DataLinkGem3500 constructor.
    */  
   DataLinkGem3500(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit);
   
   DataLinkGem3500() = default;
   virtual ~DataLinkGem3500() = default;

private:

   /**
    * Validate message cheksum
    */  
   bool checkChecksum(const std::string& line);
};

} // namespace lis1a
} // namespace tbs