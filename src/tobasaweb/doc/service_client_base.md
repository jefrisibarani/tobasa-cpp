# Use Database Services from Tobasa Web Components

`ServiceClientBase` is a mixin for web components that need the app's database-service factory. It stores a shared `DbServiceFactoryPtr`; it does not open connections or run queries.

For example, a component can request a named service like this:

```cpp
auto factory = dbServiceFactory();
if (!factory)
   throw AppException("Database service factory is not available");

auto authDbRepo = std::static_pointer_cast<web::AuthDbRepoBase>(
   factory->getDbService("AuthDbRepo"));

if (!authDbRepo)
   throw AppException("Failed to get AuthDbRepo");
```

This follows [authentication_middleware.cpp](../src/tobasaweb/authentication_middleware.cpp). `getDbService()` returns a base SQL service pointer, so cast it to the service interface your app registered. Check both factory and service before use. The factory API is declared in [database_service_factory_base.h](../../tobasasql/include/tobasasql/database_service_factory_base.h).

## Which components receive it

`ControllerBase` and `MiddlewareBase` derive from `ServiceClientBase`. The default `Router` derives from `Middleware`, so it receives the same factory too. `ControllerFactory`, `MiddlewareFactory`, and `RouterFactory` inject it during setup. A controller receives it before `bindHandler()` and `onInit()` run. Application code normally does not call the protected `setDbServiceFactory()` method. See [controller_factory.h](../include/tobasaweb/controller_factory.h), [middleware_factory.h](../include/tobasaweb/middleware_factory.h), and [router_factory.h](../include/tobasaweb/router_factory.h).

Use `dbServiceFactory()` when the component needs a service supplied by web-service configuration. Pass feature-specific objects through the component constructor when that dependency belongs to the feature. The factory pointer is shared; a component instance is reused across requests, so do not use mutable member state for per-request data. See [controller.md](controller.md) for controller construction and lifetime.