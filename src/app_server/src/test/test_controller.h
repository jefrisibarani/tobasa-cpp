#pragma once

#include <tobasaweb/controller_base.h>
#include <tobasaweb/router.h>

namespace tbs {
namespace test {

class TestController
   : public web::ControllerBase
{
public:
   TestController(const TestController &) = delete;
   TestController(TestController &&) = delete;

   explicit TestController()
      : web::ControllerBase() {}

   virtual ~TestController();
   virtual void onInit();
   virtual void bindHandler();

   //! Handle POST request to /test/crypto
  http::ResultPtr onCrypto(const web::RouteArgument& arg);

   //! Handle POST request to /test/datetime
  http::ResultPtr onDateTime(const web::RouteArgument& arg);  

   //! Handle POST request to /test/pgsql
   //! Handle POST request to /test/adosql
   //! Handle POST request to /test/odbc
   //! Handle POST request to /test/sqlite
   //! Handle POST request to /test/mysql
  http::ResultPtr onSql(const web::RouteArgument& arg);

   //! Handle POST request to /upload
  http::ResultPtr onUpload(const web::RouteArgument& arg);
};

} // namespace test
} // namespace tbs
