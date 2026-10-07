# Controllers in Tobasa Web

Think of a controller as a long-lived owner of a feature's route callbacks. `ControllerBase` gives it the shared router; `bindHandler()` registers callbacks during app setup; each callback handles one request and returns an `http::ResultPtr`.

## Start with a controller

```cpp
class UsersController : public web::ControllerBase
{
protected:
   void bindHandler() override;

private:
   http::ResultPtr onGetAll(const web::RouteArgument& args);
};

void UsersController::bindHandler()
{
   auto self(this);
   using namespace std::placeholders;

   router()->httpGet("/api/users",
      std::bind(&UsersController::onGetAll, self, _1),
      http::AuthScheme::BEARER);
}

webapp.addController(web::makeController<UsersController>());
```

   The route's handler receives a `RouteArgument` and returns a result:

   ```cpp
   http::ResultPtr UsersController::onGetAll(const web::RouteArgument&)
   {
      return http::statusResultHtml(http::StatusCode::OK, "Users");
   }
   ```

   The router converts that result into the HTTP response. See [registering_route.md](registering_route.md) for the route arguments and [http_response_and_result_class.md](http_response_and_result_class.md) for response ownership and result types.

   ## Setup and lifetime

   `makeController()` creates a `ControllerFactory`. During web-service setup, the factory creates and retains a shared controller, injects the router and database service factory, then calls `bindHandler()` followed by `onInit()`. Override `onInit()` for setup that depends on routes already being bound. See [controller_factory.h](../include/tobasaweb/controller_factory.h).

   The route callback is stored in the router. A callback bound to `this` is non-owning; the controller factory keeps the controller alive, and `WebService` keeps the factory while the service is running. If your callback captures other objects by reference, those objects must outlive every request that can call the route.

   The app creates one controller instance per factory, not one per request. With worker threads enabled, requests may use that instance at the same time. Keep request-specific values in local variables; protect shared mutable members if concurrent handlers access them.

   ## Add dependencies

   Pass feature-specific dependencies to the controller constructor:

   ```cpp
   webapp.addController(web::makeController<UsersController>(userService));
   ```

   `ControllerBase` also derives from `ServiceClientBase`, so the factory supplies the web app's database service factory. Do not set that factory yourself. See [service_client_base.md](service_client_base.md). Use `ControllerPage` from [controller_page.h](../include/tobasaweb/controller_page.h) when its page helpers fit. Put behavior shared across features in a service or middleware rather than duplicating it in handlers. The app server's [CoreController](../../app_server/src/core/core_controller.cpp) is a working example.
