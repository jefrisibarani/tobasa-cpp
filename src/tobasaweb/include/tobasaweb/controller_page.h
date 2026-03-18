#pragma once

#include "tobasaweb/controller_base.h"
#include "tobasaweb/alert.h"

namespace tbs {
namespace web {

/**
 * \ingroup WEB
 * \brief Controller for handling page requests.
 * Instances of this class are instantiated by a Factory.
 */
class ControllerPage
   : public ControllerBase
{
public:
   ControllerPage(const ControllerBase &) = delete;
   ControllerPage(ControllerBase &&) = delete;
   ControllerPage() = default;
   virtual ~ControllerPage() = default;

protected:
   /// Factory to manage this controller
   template<class ControllerImpl> friend class ControllerFactory;

   /**
    * \brief Binds a request's path to a handler function.
    * This method, called by the Factory within the initController() function
    * after attaching services to the controller, associates a request's path
    * with a specific handler function in the context of ControllerPage.
    */
   virtual void bindHandler() = 0;

   /**
    * \brief Performs additional controller-specific initialization.
    * This method, called by the Factory within the initController() function
    * after attaching services to the controller and binding the handler,
    * allows for any extra initialization steps specific to the ControllerPage.
    * It serves as the final method called during ControllerPage initialization.
    */
   virtual void onInit() {}

   bool isLoggedIn();

   void alertSuccess(const std::string& message, const std::string& location=Alert::LOC_TOAST, bool autoClose=true);
   void alertInfo(   const std::string& message, const std::string& location=Alert::LOC_TOAST, bool autoClose=true);
   void alertWarning(const std::string& message, const std::string& location=Alert::LOC_TOAST, bool autoClose=true);
   void alertError(  const std::string& message, const std::string& location=Alert::LOC_TOAST, bool autoClose=true);
};

} // namespace web
} // namespace tbs