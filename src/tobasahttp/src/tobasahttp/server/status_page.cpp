#include <memory>
#include "tobasahttp/server/status_page.h"

namespace tbs {
namespace http {

const std::string statusPageHtmlTemplate(
R"-(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="utf-8">
    <title>{{pageTitle}}</title>
    <style>
      body { height: 100%;background: #eaeaea;
        font-family: "Helvetica Neue", Helvetica, Arial, sans-serif;
        color: #777; font-weight: 300;}
      p {margin-top: 1.5rem; }
      h1, h2 { letter-spacing: 0.8; margin-top: 0; margin-bottom: 0; color: #222;}
      .display { font-size: calc(1.625rem + 4.5vw); font-weight: 300; line-height: 1;}
      .lead    { font-size: 1.25rem; font-weight: 200; }
      .content { max-width: 1024px; margin: 5rem auto; padding: 2rem;
        background: #fff; text-align: center; border: 1px solid #dddddd;
        border-radius: 0.5rem; position: relative; }
    </style>
  </head>
  <body>
    <div class="content">
      <h1 class="display">{{statusCode}}</h1>
      <p class="lead">{{statusMessage}}</p>
      <p>{{statusMessageLong}}</p>
    </div>
  </body>
</html>
)-");


std::shared_ptr<StatusPageData> statusPageData(HttpStatus status, const std::string& statusMessageLong)
{
   auto data = std::make_shared<StatusPageData>();
   data->pageTitle         = "Tobasa Web Server";
   data->pageBaseUrl       = "/";
   data->statusCode        = status.codeStr();
   data->statusMessage     = status.reason();
   data->statusMessageLong = statusMessageLong;

   return data;
}

std::string statusPageHtml(HttpStatus status, const std::string& statusMessageLong)
{
   std::map<std::string,std::string> tplMap;
   tplMap["{{pageTitle}}"]         = "Tobasa Web Server";
   tplMap["{{statusCode}}"]        = status.codeStr();
   tplMap["{{statusMessage}}"]     = status.reason();
   tplMap["{{statusMessageLong}}"] = statusMessageLong;

   std::string tpl{statusPageHtmlTemplate};

   for (auto const& [key, val] : tplMap)
   {
      size_t startPos = 0;
      if ( (startPos = tpl.find(key, startPos)) != std::string::npos )
        tpl.replace(startPos, key.length(), val);
   }

   return tpl;
}

} // namespace http
} // namespace tbs