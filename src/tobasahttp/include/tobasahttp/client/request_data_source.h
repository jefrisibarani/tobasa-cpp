#pragma once

#include <asio/any_io_executor.hpp>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "tobasa/data_reader.h"

namespace tbs {
namespace http {

class RequestDataSource : public std::enable_shared_from_this<RequestDataSource>
{
public:
   using ReadHandler = std::function<void(std::vector<uint8_t>, std::exception_ptr)>;

   static std::shared_ptr<RequestDataSource> fromFile(const std::string& path);
   static std::shared_ptr<RequestDataSource> fromBytes(std::vector<uint8_t> bytes);

   size_t size() const noexcept { return _size; }

   void asyncReadAt(
      size_t offset,
      size_t maxSize,
      asio::any_io_executor completionExecutor,
      ReadHandler handler);

private:
   RequestDataSource(
      std::shared_ptr<std::vector<uint8_t>> ownedBytes,
      std::unique_ptr<DataReader> reader);

   std::streamsize readAt(size_t offset, uint8_t *buffer, size_t size);

   std::shared_ptr<std::vector<uint8_t>> _ownedBytes;
   std::unique_ptr<DataReader> _reader;
   std::mutex _readerMutex;
   size_t _size {0};
};

} // namespace http
} // namespace tbs