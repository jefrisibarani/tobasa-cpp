#include <asio/post.hpp>
#include <asio/system_executor.hpp>
#include <limits>
#include <stdexcept>

#include "tobasa/bytes_reader.h"
#include "tobasa/file_reader.h"
#include "tobasa/nonstd_span.hpp"
#include "tobasahttp/client/request_data_source.h"

namespace tbs {
namespace http {

RequestDataSource::RequestDataSource(
   std::shared_ptr<std::vector<uint8_t>> ownedBytes,
   std::unique_ptr<DataReader> reader)
   : _ownedBytes { std::move(ownedBytes) }
   , _reader     { std::move(reader) }
{
   if (!_reader || !_reader->isOpen())
      throw std::runtime_error("Request body source is not open");

   const auto dataSize = _reader->dataSize();
   if (dataSize < 0)
      throw std::runtime_error("Request body source has an invalid size");

   _size = static_cast<size_t>(dataSize);
}

std::shared_ptr<RequestDataSource> RequestDataSource::fromFile(const std::string& path)
{
   return std::shared_ptr<RequestDataSource>(new RequestDataSource(
      nullptr, std::make_unique<FileReader>(path)));
}

std::shared_ptr<RequestDataSource> RequestDataSource::fromBytes(std::vector<uint8_t> bytes)
{
   auto ownedBytes = std::make_shared<std::vector<uint8_t>>(std::move(bytes));
   nonstd::span<const unsigned char> view(ownedBytes->data(), ownedBytes->size());
   auto reader = std::make_unique<BytesReader>(view);
   return std::shared_ptr<RequestDataSource>(new RequestDataSource(
      std::move(ownedBytes), std::move(reader)));
}

std::streamsize RequestDataSource::readAt(size_t offset, uint8_t *buffer, size_t size)
{
   if (offset > static_cast<size_t>(std::numeric_limits<std::streamoff>::max()) ||
       size > static_cast<size_t>(std::numeric_limits<std::streamsize>::max()))
      throw std::runtime_error("Request body source offset is too large");

   std::lock_guard<std::mutex> lock(_readerMutex);
   const auto bytesRead = _reader->readAt(
      std::streampos(static_cast<std::streamoff>(offset)), buffer,
      static_cast<std::streamsize>(size));
   if (bytesRead < 0)
      throw std::runtime_error("Failed to read request body source");

   return bytesRead;
}

void RequestDataSource::asyncReadAt(
   size_t offset,
   size_t maxSize,
   asio::any_io_executor completionExecutor,
   ReadHandler handler)
{
   auto self = shared_from_this();
   asio::post(asio::system_executor(),
      [self, offset, maxSize, completionExecutor = std::move(completionExecutor),
       handler = std::move(handler)]() mutable
      {
         std::vector<uint8_t> data(maxSize);
         std::exception_ptr error;
         try
         {
            const auto bytesRead = self->readAt(offset, data.data(), maxSize);
            data.resize(static_cast<size_t>(bytesRead));
         }
         catch (...)
         {
            error = std::current_exception();
            data.clear();
         }

         asio::post(completionExecutor,
            [handler = std::move(handler), data = std::move(data), error]() mutable
            {
               if (handler)
                  handler(std::move(data), error);
            });
      });
}

} // namespace http
} // namespace tbs