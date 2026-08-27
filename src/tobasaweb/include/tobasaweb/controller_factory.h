#pragma once

#include "tobasaweb/controller_base.h"
#include "tobasaweb/web_service_base.h"
#include "tobasaweb/controller_factory_base.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

/** 
 * ControllerFactory
 * \tparam ControllerImpl
 */
template <typename ControllerImpl>
class ControllerFactory
   : public ControllerFactoryBase
{
protected:
   std::shared_ptr<ControllerImpl> _controller;
   bool _initialized = false;

public:
   ControllerFactory(const ControllerFactory &) = delete;
   ControllerFactory(ControllerFactory &&) = delete;
   virtual ~ControllerFactory() = default;

   ControllerFactory()
   {
      _controller = std::make_shared<ControllerImpl>();
   }

   template<typename... Params>
   ControllerFactory(Params &&... params)
   {
      _controller = std::make_shared<ControllerImpl>(std::forward<Params>(params)...);
   }

   virtual bool initialized()
   {
      return _initialized;
   }

   virtual void initController(WebServiceBase* webService)
   {
      if ( ! _initialized )
      {
         // ControllerFactory<ControllerImpl> is a friend of ControllerBase, 
         // allowing it to access ControllerBase's private and protected members.
         _controller->setRouter(webService->router() );

         _controller->setDbServiceFactory( webService->dbServiceFactory() );

         // _controller is a ControllerImpl. ControllerFactory<ControllerImpl> is
         // a friend of ControllerBase, but not of ControllerImpl. Cast to the
         // ControllerBase interface so the factory can call the protected
         // ControllerBase::bindHandler() method.
         auto basePtr = std::static_pointer_cast<ControllerBase>(_controller);
         basePtr->bindHandler();

         basePtr->onInit();
         _initialized = true;
      }
   }

   virtual void reconfigureDb(WebServiceBase* webService)
   {
      _controller->setDbServiceFactory( webService->dbServiceFactory() );
   }
};


template <class ControllerType>
[[nodiscard]]
ControllerFactoryPtr makeController()
{
   return std::make_shared<tbs::web::ControllerFactory<ControllerType>>();
}

template <class ControllerType, typename... Params>
[[nodiscard]]
ControllerFactoryPtr makeController(Params&&... args)
{
   return std::make_shared<tbs::web::ControllerFactory<ControllerType>>(std::forward<Params>(args)...);
}

/** @}*/

} // namespace web
} // namespace tbs