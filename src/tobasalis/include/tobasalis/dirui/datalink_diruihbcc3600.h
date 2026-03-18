#pragma once

#include "tobasalis/lis/datalink.h"

namespace tbs {
namespace dirui {

/** \ingroup LIS
 * DataLinkDirUiBcc3600
 */
class DataLinkDirUiBcc3600
   : public lis::DataLink
{
public:

   /**
    * DataLinkDirUiBcc3600 constructor.
    */   
   DataLinkDirUiBcc3600(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit);
   
   DataLinkDirUiBcc3600() = default;
   virtual ~DataLinkDirUiBcc3600() = default;

private:

   /**
    * Connection receive data event handler.
    */  
   virtual void connectionOnReceiveData(const std::string& data);
};

} // namespace dirui
} // namespace tbs