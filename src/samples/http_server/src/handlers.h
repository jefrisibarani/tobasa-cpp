#pragma once

#include <tobasahttp/server/http_server.h>

tbs::http::RequestStatus statusResult(const tbs::http::HttpContext& context, tbs::http::StatusCode statusCode);
tbs::http::RequestStatus handleHelloPage(const tbs::http::HttpContext& context);
tbs::http::RequestStatus handleUpload(const tbs::http::HttpContext& context);
tbs::http::RequestStatus handleRawUpload(const tbs::http::HttpContext& context);
tbs::http::RequestStatus handleRawUploadDownload(const tbs::http::HttpContext& context);
tbs::http::RequestStatus handleIndexPage(const tbs::http::HttpContext& context);
tbs::http::RequestStatus handleBrowseUpload(const tbs::http::HttpContext& context);
tbs::http::RequestStatus handleWebsocketEndpoint(const tbs::http::HttpContext& context);
tbs::http::RequestStatus handleWebsocket(const tbs::http::HttpContext& context);

void initializeWebSocketContext();
void resetWebSocketContext();