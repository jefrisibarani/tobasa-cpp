#include <iostream>
#include "tobasahttp/server/common.h"
#include "tobasahttp/request.h"
#include "tobasahttp/websocket.h"
#include "tobasahttp/multipart_body_reader.h"

namespace tbs {
namespace http {

Context::Context(
     std::shared_ptr<Request>  req
   , std::shared_ptr<Response> resp
   , asio::ip::tcp::endpoint   ep
   , ConnectionId              id
   , uint32_t                  reqId
   , HttpVersion               httpVersion
#ifdef TOBASA_HTTP_USE_HTTP2
   , int32_t                   streamId
#endif
   )
   : _request        {std::move(req)}
   , _response       {std::move(resp)}
   , _remoteEndpoint {ep}
   , _connectionId   {id}
   , _requestId      {reqId} 
   , _httpVersion    {httpVersion}
#ifdef TOBASA_HTTP_USE_HTTP2
   , _streamId       {streamId}
#endif
{}

Context::~Context()
{
   _closed = true;
}

std::shared_ptr<Request>& Context::request()
{
   return _request;
}

std::shared_ptr<Response>& Context::response()
{
   return _response;
}

asio::ip::tcp::endpoint& Context::remoteEndpoint()
{
   return _remoteEndpoint;
}

std::any& Context::userData()
{
   return _userData;
}

void Context::userData(std::any userData)
{
   _userData = std::move(userData);
}

ConnectionId Context::connId()
{
   return _connectionId;
}

bool Context::isHttps()
{
   return _request->isHttps();
}

std::string Context::sessionId()
{
   return _request->sessionId();
}

void Context::webSocketContext(WebSocketContextPtr ctx)
{
   if (_webSockeInitHandler)
      _webSockeInitHandler(ctx);
}

void Context::webSocketInitHandler(std::function<void(WebSocketContextPtr)> handler)
{
   _webSockeInitHandler = handler;
}

std::shared_ptr<MultipartBodyReader> Context::getBodyReader() 
{  
   return _bodyReader; 
}

void Context::setBodyReader(std::shared_ptr<MultipartBodyReader> reader) 
{  
   _bodyReader = reader; 
}

void Context::onCompleteHandler(std::function<void(RequestStatus)> handler)
{
   _onCompleteHandler = std::move(handler);
}

void Context::complete(RequestStatus status)
{
   if (_onCompleteHandler) 
      _onCompleteHandler(status);
}

} // namespace http
} // namespace tbs