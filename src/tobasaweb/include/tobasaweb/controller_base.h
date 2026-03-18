#pragma once

#include "tobasaweb/service_client_base.h"
#include "tobasaweb/router_base.h"

namespace tbs {
namespace web {


/**
 * \ingroup WEB
 * \brief Foundational class for controllers, which handle router request handling.
 * These controllers are instantiated by a Factory.
 */
class ControllerBase
   : public ServiceClientBase
{
public:
   ControllerBase(const ControllerBase &) = delete;
   ControllerBase(ControllerBase &&) = delete;
   ControllerBase() = default;
   virtual ~ControllerBase() = default;

protected:
   /// Factory to manage this controller
   template<class ControllerImpl> friend class ControllerFactory;

private:
   RouterPtr _router;

public:
   virtual RouterPtr router()
   {
      return _router;
   }

protected:

   /**
    * \brief Binds a request's path to a handler function.
    * This method, called by the Factory within the initController() function
    * after attaching services to the controller, associates a request's path
    * with a specific handler function.
    */
   virtual void bindHandler() = 0;

   /**
    * \brief Performs additional controller initialization.
    * This method, called by the Factory within the initController() function
    * after attaching services to the controller and binding the handler,
    * allows for any extra initialization steps specific to the controller.
    * It serves as the last method called during controller initialization.
    */   
   virtual void onInit() {}

   http::ResultPtr redirect(const std::string& location, http::StatusCode statusCode=http::StatusCode::FOUND);

private:
   /// Factory call this methods in initController()
   virtual void setRouter(RouterPtr router)
   {
      _router = router;
   }
};

} // namespace web
} // namespace tbs