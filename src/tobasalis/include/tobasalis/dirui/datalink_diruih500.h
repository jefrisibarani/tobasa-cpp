#pragma once

#include "tobasalis/lis/datalink.h"

namespace tbs {
namespace dirui {

/** \ingroup LIS
 * DataLinkDirUih500
 */
class DataLinkDirUih500
   : public lis::DataLink
{
public:

   /**
    * DataLinkDirUih500 constructor.
    */  
   DataLinkDirUih500(asio::io_context& io_ctx, std::shared_ptr<lis::Connection> connection, const lis::LinkLimit& limit);
   
   DataLinkDirUih500() = default;
   virtual ~DataLinkDirUih500() = default;

private:

   /**
    * Connection receive data event handler.
    */  
   virtual void connectionOnReceiveData(const std::string& data);
};

} // namespace dirui
} // namespace tbs