#pragma once
#include <memory>

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

class WebServiceBase;

/** 
 * ControllerFactoryBase
 */
class ControllerFactoryBase
{
public:
   ControllerFactoryBase(const ControllerFactoryBase &) = delete;
   ControllerFactoryBase(ControllerFactoryBase &&) = delete;
   ControllerFactoryBase() = default;
   virtual ~ControllerFactoryBase() = default;

   virtual bool initialized() = 0;
   virtual void initController(WebServiceBase* webService) = 0;
   virtual void reconfigureDb(WebServiceBase* webService) = 0;
};

using ControllerFactoryPtr = std::shared_ptr<ControllerFactoryBase>;

/** @}*/

} // namespace web
} // namespace tbs