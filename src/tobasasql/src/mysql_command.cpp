#include <iostream>
#include <tobasa/variant_helper.h>
#include "tobasasql/sql_dataset.h"
#include "tobasasql/mysql_util.h"
#include "tobasasql/mysql_command.h"

namespace tbs {
namespace sql {

constexpr int32_t MEDIUMINT_MIN = -8388608;   // -2^23
constexpr int32_t MEDIUMINT_MAX =  8388607;   //  2^23 - 1

MysqlCommand::MysqlCommand(MYSQL *conn)
   : _pConn(conn)
   , _pStmt(nullptr)
   , _pResultMetadata(nullptr)
   , _affectedRows(UINT64_MAX)
   , _pResultContext(nullptr)
{}

bool MysqlCommand::init(const std::string& sql, const MysqlParameterCollection& parameters)
{
   try
   {
      // Note:
      // https://dev.mysql.com/doc/c-api/8.0/en/mysql-stmt-execute.html
      // https://dev.mysql.com/doc/c-api/8.0/en/mysql-stmt-fetch.html
      // https://dev.mysql.com/doc/c-api/8.0/en/c-api-prepared-statement-type-codes.html

      if (_pConn == nullptr)
         return false;

      _pStmt = mysql_stmt_init(_pConn);
      if (!_pStmt)
         throw SqlException(lastBackendError());

      if (mysql_stmt_prepare(_pStmt, sql.c_str(), static_cast<unsigned long>(sql.length()) ) != 0)
         throw SqlException(lastBackendError());

      // TODO_JEFRI: Validate parameter count
      // Get the parameter count from the statement
      //int paramCount = mysql_stmt_param_count(_pStmt);
      //if (paramCount == 0)
      //   throw SqlException("invalid parameter count returned by MySQL");

      int totalParam = (int)parameters.size();
      _paramContext = ParameterContext(totalParam);

      // -------------------------------------------------------
      // Bind the parameters
      // -------------------------------------------------------
      // Bind the data for all parameters
      for (unsigned int i = 0; i < parameters.size(); i++)
      {
         auto& param = parameters.at(i);
         MySqlType parameterType = mysqlDataTypeFromDataType(param->type());

         if (std::holds_alternative<std::monostate>(param->value()) )
         {
            _paramContext.binds[i].buffer_type = parameterType;
            _paramContext.binds[i].buffer      = nullptr;
            _paramContext.binds[i].is_null     = &_paramContext.isNulls[i];
            _paramContext.binds[i].length      = 0;
         }
         else
         {
            std::string errMsg = "Invalid variant storage for parameter value type " + dataTypeToString(param->type());
            _paramContext.binds[i].buffer_type = parameterType;
            _paramContext.binds[i].is_null     = &_paramContext.isNulls[i];
            
            if (param->forceUnsigned())
            {
               _paramContext.binds[i].is_unsigned = 1;
            }

            switch (parameterType)
            {
               case MYSQL_TYPE_NULL:
               {
                  _paramContext.binds[i].buffer_type = parameterType;
                  _paramContext.binds[i].buffer      = nullptr;
                  _paramContext.binds[i].is_null     = &_paramContext.isNulls[i];
                  _paramContext.binds[i].length      = 0;
               }
                  break;
               case MYSQL_TYPE_BIT:
               case MYSQL_TYPE_TINY:
               {
                  if (param->forceUnsigned())
                  {
                     if (std::holds_alternative<bool>(param->value()))
                     {
                        // get and reset value for boolean parameter
                        bool value = std::get<bool>(param->value());
                        uint8_t realVal = value ? (uint8_t)1 : (uint8_t)0;
                        param->value(realVal);

                        _paramContext.binds[i].buffer = (void*) &(std::get<uint8_t>(param->value()));
                     }
                     else if (std::holds_alternative<uint8_t>(param->value()))
                     {
                        _paramContext.binds[i].buffer = (void*) &( std::get<uint8_t>(param->value()) );
                     }
                     else
                        throw SqlException(errMsg, "MysqlCommand");
                  }
                  else 
                  {
                     if (std::holds_alternative<bool>(param->value()))
                     {
                        // get and reset value for boolean parameter
                        bool value = std::get<bool>(param->value());
                        int8_t realVal = value ? (int8_t)1 : (int8_t)0;
                        param->value( (int8_t)realVal );

                        _paramContext.binds[i].buffer = (void*) &(std::get<int8_t>(param->value()));
                     }
                     else if (std::holds_alternative<int8_t>(param->value())) 
                     {
                        _paramContext.binds[i].buffer = (void*) &(std::get<int8_t>(param->value())) ;
                     }
                     else if (std::holds_alternative<int16_t>(param->value()))
                     {
                        auto currentVal = std::get<int16_t>(param->value());
                        param->value( static_cast<int8_t>(currentVal) );
                        _paramContext.binds[i].buffer = (void*) &( std::get<int8_t>(param->value()) );
                     }
                     else if (std::holds_alternative<int32_t>(param->value()))
                     {
                        auto currentVal = std::get<int32_t>(param->value());
                        param->value( static_cast<int8_t>(currentVal) );
                        _paramContext.binds[i].buffer = (void*) &( std::get<int8_t>(param->value()) );
                     }
                     else if (std::holds_alternative<int64_t>(param->value()))
                     {
                        auto currentVal = std::get<int64_t>(param->value());
                        param->value( static_cast<int8_t>(currentVal) );
                        _paramContext.binds[i].buffer = (void*) &( std::get<int8_t>(param->value()) );
                     }
                     else
                        throw SqlException(errMsg, "MysqlCommand");
                  }
               }
                  break;
               case MYSQL_TYPE_SHORT: // SMALLINT
               {
                  if (param->forceUnsigned())
                  {
                      if (std::holds_alternative<uint16_t>(param->value()))
                     {
                        _paramContext.binds[i].buffer = (void*) &( std::get<uint16_t>(param->value()) );
                     }
                     else
                        throw SqlException(errMsg, "MysqlCommand");
                  }
                  else 
                  {
                     if (std::holds_alternative<int8_t>(param->value())) 
                     {
                        auto currentVal = std::get<int8_t>(param->value());
                        param->value( static_cast<int16_t>(currentVal) );
                        _paramContext.binds[i].buffer = (void*) &( std::get<int16_t>(param->value()) );
                     }
                     else if (std::holds_alternative<int16_t>(param->value()))
                     {
                        _paramContext.binds[i].buffer = (void*) &( std::get<int16_t>(param->value()) );
                     }
                     else if (std::holds_alternative<int32_t>(param->value()))
                     {
                        auto currentVal = std::get<int32_t>(param->value());
                        param->value( static_cast<int16_t>(currentVal) );
                        _paramContext.binds[i].buffer = (void*) &( std::get<int16_t>(param->value()) );
                     }
                     else if (std::holds_alternative<int64_t>(param->value()))
                     {
                        auto currentVal = std::get<int64_t>(param->value());
                        param->value( static_cast<int16_t>(currentVal) );
                        _paramContext.binds[i].buffer = (void*) &( std::get<int16_t>(param->value()) );
                     }
                     else
                        throw SqlException(errMsg, "MysqlCommand");
                  }
                  
                  break;
               }
               case MYSQL_TYPE_YEAR:
               case MYSQL_TYPE_INT24: // 3 bytes (24 bits)
               case MYSQL_TYPE_LONG:  // 32 bits
               {
                  if (param->forceUnsigned())
                  {
                      if (std::holds_alternative<uint32_t>(param->value()))
                     {
                        _paramContext.binds[i].buffer = (void*) &( std::get<uint32_t>(param->value()) );
                     }
                     else
                        throw SqlException(errMsg, "MysqlCommand");
                  }
                  else 
                  {
                     if (std::holds_alternative<int8_t>(param->value())) 
                     {
                        auto currentVal = std::get<int8_t>(param->value());
                        param->value( static_cast<int32_t>(currentVal) );
                        _paramContext.binds[i].buffer = (void*) &( std::get<int32_t>(param->value()) );
                     }
                     else if (std::holds_alternative<int16_t>(param->value()))
                     {
                        auto currentVal = std::get<int16_t>(param->value());
                        param->value( static_cast<int32_t>(currentVal) );
                        _paramContext.binds[i].buffer = (void*) &( std::get<int32_t>(param->value()) );
                     }
                     else if (std::holds_alternative<int32_t>(param->value()))
                     {
                        auto currentVal = std::get<int32_t>(param->value());
                        // Range check before binding
                        if (parameterType == MYSQL_TYPE_INT24 && (currentVal < MEDIUMINT_MIN || currentVal > MEDIUMINT_MAX) ) {
                           throw std::out_of_range("Value out of range for MySQL MEDIUMINT");
                        }

                        _paramContext.binds[i].buffer = (void*) &( std::get<int32_t>(param->value()) );
                     }
                     else if (std::holds_alternative<int64_t>(param->value()))
                     {
                        auto currentVal = std::get<int64_t>(param->value());
                        // Range check before binding
                        if (parameterType == MYSQL_TYPE_INT24 && (currentVal < MEDIUMINT_MIN || currentVal > MEDIUMINT_MAX) ) {
                           throw std::out_of_range("Value out of range for MySQL MEDIUMINT");
                        }

                        param->value( static_cast<int32_t>(currentVal) );
                        _paramContext.binds[i].buffer = (void*) &( std::get<int32_t>(param->value()) );
                     }
                     else
                        throw SqlException(errMsg, "MysqlCommand");
                  }

                  break;
               }
               case MYSQL_TYPE_LONGLONG:
               {
                  if (param->forceUnsigned())
                  {
                      if (std::holds_alternative<uint64_t>(param->value()))
                     {
                        _paramContext.binds[i].buffer = (void*) &( std::get<uint64_t>(param->value()) );
                     }
                     else
                        throw SqlException(errMsg, "MysqlCommand");
                  }
                  else
                  {
                     if (std::holds_alternative<int8_t>(param->value())) 
                     {
                        auto currentVal = std::get<int8_t>(param->value());
                        param->value( static_cast<int64_t>(currentVal) );
                        _paramContext.binds[i].buffer = (void*) &( std::get<int64_t>(param->value()) );
                     }
                     else if (std::holds_alternative<int16_t>(param->value()))
                     {
                        auto currentVal = std::get<int16_t>(param->value());
                        param->value( static_cast<int64_t>(currentVal) );
                        _paramContext.binds[i].buffer = (void*) &( std::get<int64_t>(param->value()) );
                     }
                     else if (std::holds_alternative<int32_t>(param->value()))
                     {
                        auto currentVal = std::get<int32_t>(param->value());
                        param->value( static_cast<int64_t>(currentVal) );
                        _paramContext.binds[i].buffer = (void*) &( std::get<int64_t>(param->value()) );
                     }
                     else if (std::holds_alternative<int64_t>(param->value()))
                     {
                        _paramContext.binds[i].buffer = (void*) &( std::get<int64_t>(param->value()) );
                     }
                     else
                        throw SqlException(errMsg, "MysqlCommand");
                  }

                  break;
               }
               case MYSQL_TYPE_FLOAT:
                  _paramContext.binds[i].buffer = (void*) &(VariantHelper::value<float>(param->value(), errMsg));
                  break;
               case MYSQL_TYPE_DOUBLE:
                  _paramContext.binds[i].buffer = (void*) &(VariantHelper::value<double>(param->value(), errMsg));
                  break;
               case MYSQL_TYPE_DECIMAL:
               case MYSQL_TYPE_NEWDECIMAL:
               case MYSQL_TYPE_STRING:
               case MYSQL_TYPE_VAR_STRING:
               //case MYSQL_TYPE_VARCHAR: // mysql_stmt_bind_param does not support
               {
                  const std::string& strValue   = VariantHelper::value<std::string>(param->value(), errMsg);
                  unsigned long strValueLen     = static_cast<unsigned long>(strValue.length());
                  _paramContext.lengths[i]      = strValueLen;
                  _paramContext.binds[i].buffer = (void*) strValue.data();
                  _paramContext.binds[i].length = &_paramContext.lengths[i];
               }
                  break;
#if 1
               case MYSQL_TYPE_TIME:
               case MYSQL_TYPE_DATE:
               case MYSQL_TYPE_DATETIME:
               case MYSQL_TYPE_TIMESTAMP:
               {
                  // we use MYSQL_TIME as datetime value source
                  // -------------------------------------------------------
                  // convert datetime string value to MysqlTime 
                  MysqlTime mt = VariantHelper::toMysqlTime(param->value());
                  // save back MysqlTime value
                  param->value( mt );
                  // use the MYSQL_TIME pointer as bind's buffer source
                  _paramContext.binds[i].buffer = (void*) &( std::get<MysqlTime>(param->value()) ).myTime;
                  //_paramContext.lengths[i]      = sizeof(MYSQL_TIME);
                  //_paramContext.binds[i].length = &_paramContext.lengths[i];
               }
                  break;
#endif
#if 0
               case MYSQL_TYPE_TIME:
               case MYSQL_TYPE_DATE:
               case MYSQL_TYPE_DATETIME:
               case MYSQL_TYPE_TIMESTAMP:
               {
                  // Directly use string as datetime value source
                  // -------------------------------------------------------
                  const std::string& strValue   = VariantHelper::value<std::string>(param->value(), errMsg);
                  unsigned long strValueLen     = static_cast<unsigned long>(strValue.length());
                  _paramContext.lengths[i]      = strValueLen;
                  _paramContext.binds[i].buffer = (char*)strValue.data();
                  _paramContext.binds[i].length = &_paramContext.lengths[i];
                  // because we use string buffer as datetime value source , we must set buffer_type to MYSQL_TYPE_STRING
                  _paramContext.binds[i].buffer_type = MYSQL_TYPE_STRING;   
               }                 
                  break;
#endif                  
               case MYSQL_TYPE_TINY_BLOB:
               case MYSQL_TYPE_MEDIUM_BLOB:
               case MYSQL_TYPE_LONG_BLOB:
               case MYSQL_TYPE_BLOB:
               {
                  _paramContext.lengths[i]      = static_cast<unsigned long>(param->size());
                  _paramContext.binds[i].buffer = (void*) *(param->valueBytePtr());
                  _paramContext.binds[i].length = &_paramContext.lengths[i];
               }
                  break;
               default:
                  throw tbs::SqlException("Unsupported Mysql data type for parameter ", "MysqlCommand");
                  break;
            } // switch
         } // else
      } // for
      // -------------------------------------------------------

      // Bind the buffers
      auto rc = mysql_stmt_bind_param(_pStmt, _paramContext.binds.data());
      if (rc != 0)
      {
         //auto errNo  = mysql_stmt_errno(_pStmt);
         auto errMsg = statementError();

         throw SqlException(errMsg, "MysqlCommand");
      }
   }
   catch (const std::bad_variant_access&)
   {
      throw tbs::SqlException("bad variant access", "MysqlCommand");
   }
   catch (const VariantException &ex)
   {
      throw tbs::SqlException(ex.what(), "MysqlCommand");
   }
   catch (const TypeException &ex)
   {
      throw SqlException(ex.what(), "MysqlCommand");
   }

   return true;
}

MysqlCommand::~MysqlCommand()
{
   if (_pResultMetadata != nullptr)
   {
      mysql_free_result(_pResultMetadata);
      _pResultMetadata = nullptr;
   }

   if (_pStmt != nullptr)
   {
      mysql_stmt_free_result(_pStmt);
      mysql_stmt_close(_pStmt);
      _pStmt = nullptr;
   }

   if (_pResultContext != nullptr)
      delete _pResultContext;
}

int MysqlCommand::execute()
{
   // Note:
   // https://dev.mysql.com/doc/c-api/8.0/en/mysql-stmt-fetch.html#:~:text=mysql_stmt_fetch%20%28%29%20returns%20row%20data%20using%20the%20buffers,by%20the%20application%20before%20it%20calls%20mysql_stmt_fetch%20%28%29.

   if (mysql_stmt_execute(_pStmt) != 0)
      throw SqlException(statementError(), "MysqlCommand");

   _pResultMetadata = mysql_stmt_result_metadata(_pStmt);
   if (_pResultMetadata)
   {
      // we have data
      //unsigned int numFields = mysql_num_fields(_pResultMetadata);
      mysql_free_result(_pResultMetadata);
      _pResultMetadata = nullptr;   
      
      _affectedRows = 0;
   }
   else
   {
      // Get the number of affected rows
      _affectedRows = mysql_stmt_affected_rows(_pStmt);

      if (_affectedRows ==  MYSQL_NON_AFFECTING_ROWS_QUERY ) {
         // Not UPDATE, DELETE, or INSERT query
         _affectedRows = 0;
      }

      if (_affectedRows == UINT64_MAX)
         throw SqlException(statementError(), "MysqlCommand");
      else if (_affectedRows == (uint64_t)-1)
         throw SqlException(statementError(), "MysqlCommand");
   }

   if (_affectedRows >= 0)
      return static_cast<int>(_affectedRows);
   else 
      return -1;
}


std::shared_ptr<DataSet<MysqlVariantType>> MysqlCommand::executeResult()
{
   // Note::
   // https://dev.mysql.com/doc/c-api/8.0/en/mysql-stmt-fetch.html#:~:text=mysql_stmt_fetch%20%28%29%20returns%20row%20data%20using%20the%20buffers,by%20the%20application%20before%20it%20calls%20mysql_stmt_fetch%20%28%29.

   if (mysql_stmt_execute(_pStmt) != 0)
      throw SqlException(statementError());

   // get metada for query returning results (eg: SELECT)
   _pResultMetadata = mysql_stmt_result_metadata(_pStmt);
   if (!_pResultMetadata)
   {
      // NON Returning data Query

      // Get the number of affected rows
      _affectedRows = mysql_stmt_affected_rows(_pStmt);
      if (_affectedRows == MYSQL_NON_AFFECTING_ROWS_QUERY ) 
      {
         // Not UPDATE, DELETE, or INSERT query
         _affectedRows = 0;
      }

      if (_affectedRows == UINT64_MAX)
         throw SqlException(statementError(), "MysqlCommand");
      else if (_affectedRows == (uint64_t)-1)
         throw SqlException(statementError(), "MysqlCommand");

      return std::make_shared<DataSet<MysqlVariantType>>();
   }

   // NOW we proceed with the DATA
   _affectedRows = 0;
   auto dbResult = std::make_shared<DataSet<MysqlVariantType>>();

   int totalColumns = mysql_num_fields(_pResultMetadata);
   if (totalColumns == 0)
      throw SqlException("result metadata does not have field", "MysqlCommand");

   dbResult->totalColumns = totalColumns;

   // Init row/fields information
   _pResultContext = new ResultContext(totalColumns);
   
   // Bind result buffers
   for (int i = 0; i < totalColumns; i++)
   {
      MYSQL_FIELD* field = mysql_fetch_field_direct(_pResultMetadata, i);
      _pResultContext->fields[i] = field;
      _pResultContext->binds[i].buffer_type = field->type;

      // Note: field->length is data type length, not the REAL LENGTH
      // Configure receive data buffer
      switch(field->type)
      {
         case MYSQL_TYPE_TINY_BLOB:
         case MYSQL_TYPE_MEDIUM_BLOB:
         case MYSQL_TYPE_LONG_BLOB:
         case MYSQL_TYPE_BLOB:
         {
            _pResultContext->initFieldBuffer(field->type, i, field->length, field->flags);
            _pResultContext->binds[i].buffer = _pResultContext->getFieldBufferPointer(field->type, i, field->flags);
            _pResultContext->binds[i].buffer_length = field->length;
         }
            break;
         default:
         {
            _pResultContext->initFieldBuffer(field->type, i, field->length, field->flags);
            _pResultContext->binds[i].buffer = _pResultContext->getFieldBufferPointer(field->type, i, field->flags);
            _pResultContext->binds[i].buffer_length = field->length;
         }
            break;
      }
   }


   // get bind datas
   // rowInfo's pLengths, pIsNulls, pErrors  initialized after this call
   // Note: rowInfo's pLengths contains the REAL LENGTH
   if (mysql_stmt_bind_result(_pStmt, _pResultContext->binds.data() ) != 0)
      throw SqlException(statementError());

   // Optional: Buffer all results to client
   //if (mysql_stmt_store_result(_pStmt) == 0) {
   //    //we have result
   //}

   // Fetch result set row by row
   int currentRow = 0;
   int status;
   while (1)
   {
      status = mysql_stmt_fetch(_pStmt);
      if (status == 1)
         throw SqlException(statementError());
      else if (status == MYSQL_NO_DATA)
         break;
      else if (status == MYSQL_DATA_TRUNCATED )
      {
         for(size_t i=0;i<_paramContext.errors.size();i++)
         {
            if (_paramContext.errors[i])
               throw SqlException(tbsfmt::format("data truncated for parameter {}",i ));
         }
      }

      // status is 0 / SUCCESS

      // Process each row
      std::vector<VariantType> results;
      for (int col=0; col < totalColumns; col++)
      {
         results.emplace_back( std::move( VariantType()) );

         MYSQL_BIND& resbind = _pResultContext->binds[col];
         auto realLength     = _pResultContext->pLengths[col];
         std::string columnName(_pResultContext->fields[col]->name);

         if (_pResultContext->pIsNulls[col] != 0)
         {
            // column is NULL
            results[col] = std::monostate{};
         }
         else
         {
            // NOT NULL
            if (realLength > 0)
            {
               switch(resbind.buffer_type)
               {
                  case MYSQL_TYPE_BIT:
                  case MYSQL_TYPE_TINY:
                     results[col] = _pResultContext->fieldBuffers[col];
                     break;
                  case MYSQL_TYPE_YEAR:
                     results[col] = _pResultContext->fieldBuffers[col];
                     break;
                  case MYSQL_TYPE_SHORT: // SMALLINT
                     results[col] = _pResultContext->fieldBuffers[col];
                     break;
                  case MYSQL_TYPE_INT24:
                  case MYSQL_TYPE_LONG:
                     results[col] = _pResultContext->fieldBuffers[col];
                     break;
                  case MYSQL_TYPE_LONGLONG:
                     results[col] = _pResultContext->fieldBuffers[col];
                     break;
                  case MYSQL_TYPE_FLOAT:
                     results[col] = _pResultContext->fieldBuffers[col];
                     break;
                  case MYSQL_TYPE_DOUBLE:
                     results[col] = _pResultContext->fieldBuffers[col];
                     break;
                  case MYSQL_TYPE_NEWDECIMAL:
                  case MYSQL_TYPE_STRING:
                  case MYSQL_TYPE_VAR_STRING:
                  case MYSQL_TYPE_VARCHAR:
                  {
                     // Note: mysql_stmt_fetch_column() does not work with SELECT *
                     //resbind.buffer_length = realLength;
                     //std::vector<char> val(realLength);
                     //resbind.buffer = (char*)val.data();
                     //mysql_stmt_fetch_column(_pStmt, _pResultContext->binds.data(), col, currentRow);

                     bool isBinary = _pResultContext->fields[col]->flags & BINARY_FLAG;
                     VariantType value;

                     if (isBinary)
                     {
                        auto blobdata = std::get_if<std::vector<uint8_t>>( &(_pResultContext->fieldBuffers[col]) );
                        if (!blobdata)
                           throw SqlException("Invalid STRING blobdata pointer", "MysqlCommand");

                        if (!blobdata->empty())
                        {
                           std::string result;
                           crypt::hexEncode((crypt::byte_t*)blobdata->data(), realLength, result);
                           value = std::move(result);
                        }
                        else 
                           value = std::string{};
                     }
                     else 
                     {
                        auto strdata = std::get_if<std::vector<char>>( &(_pResultContext->fieldBuffers[col]) );
                        if (!strdata)
                           throw SqlException("Invalid STRING string data pointer", "MysqlCommand");

                        if (!strdata->empty())
                        {
                           std::string result(strdata->begin(), strdata->begin() + std::min(strdata->size(), size_t(realLength)));
                           value = std::move(result);
                        }
                        else 
                           value = std::string{};
                     }

                     results[col] = std::move(value);
                  }
                     break;
                  case MYSQL_TYPE_TIME:
                  case MYSQL_TYPE_DATE:
                  case MYSQL_TYPE_DATETIME:
                  case MYSQL_TYPE_TIMESTAMP:
                  {
                     results[col] = _pResultContext->fieldBuffers[col];
                  }
                     break;
                  case MYSQL_TYPE_TINY_BLOB:
                  case MYSQL_TYPE_MEDIUM_BLOB:
                  case MYSQL_TYPE_LONG_BLOB:
                  case MYSQL_TYPE_BLOB:
                  {
                     //bool isBlobField = _pResultContext->fields[col]->flags & BLOB_FLAG;
                     bool isBinary    = _pResultContext->fields[col]->flags & BINARY_FLAG;

                     VariantType value;
                     if (isBinary)
                     {
                        // TINY_BLOB, MEDIUM_BLOB, LONG_BLOB, BLOB
                        auto blobdata = std::get_if<std::vector<uint8_t>>( &(_pResultContext->fieldBuffers[col]) );
                        if (!blobdata)
                           throw SqlException("Invalid BLOB blobdata pointer", "MysqlCommand");

                        if (!blobdata->empty())
                        {
                           std::string result;
                           crypt::hexEncode((crypt::byte_t*)blobdata->data(), realLength, result);
                           value = std::move(result);
                        }
                        else 
                           value = std::string{};
                     }
                     else
                     {
                        // TINY_TEXT, MEDIUM_TEXT, LONG_TEXT, TEXT
                        auto strdata = std::get_if<std::vector<char>>( &(_pResultContext->fieldBuffers[col]) );
                        if (!strdata)
                           throw SqlException("Invalid CLOB string data pointer", "MysqlCommand");

                        if (!strdata->empty())
                        {
                           std::string result(strdata->begin(), strdata->begin() + std::min(strdata->size(), size_t(realLength)));
                           value = std::move(result);
                        }
                        else 
                           value = std::string{};
                     }

                     results[col] = std::move(value);
                  }
                     break;
                  default:
                     throw tbs::SqlException("Unsupported Mysql result data type received from backend", "MysqlCommand");
                     break;
               }
            }
         }
      }

      currentRow++;
      dbResult->data.emplace_back(std::move(results));
   }

   dbResult->totalRows = currentRow;

   // cleanup
   mysql_free_result(_pResultMetadata);
   _pResultMetadata = nullptr;

   return dbResult;
}


std::string MysqlCommand::lastBackendError()
{
   std::string errmsg;
   if (_pConn)
   {
      const char* err = mysql_error(_pConn);
      if (err)
         return std::string(err);
   }

   return {};
}

std::string MysqlCommand::statementError()
{
   if (_pStmt)
   {
      const char* err = mysql_stmt_error(_pStmt);
      if (err[0])
         return std::string(err);
   }

   return {};
}


MysqlCommand::ResultContext::ResultContext(int columnsCount)
   : totalColumns(columnsCount)
{
   fields.resize(columnsCount);
   binds.resize(columnsCount);
   pIsNulls.resize(columnsCount);
   pErrors.resize(columnsCount);
   pLengths.resize(columnsCount);

   for (int i=0; i<totalColumns; i++)
   {
      fieldBuffers.emplace_back( std::move( VariantType()) );
   }

   // assign bindings map
   for (int i = 0; i < totalColumns; i++) 
   {
      MYSQL_BIND& bind   = binds[i];
      bind.length        = &pLengths[i];
      bind.is_null       = &pIsNulls[i];
      bind.error         = &pErrors[i];

      bind.buffer_length = 0;
      bind.buffer        = 0;
   }
}

// Initialze Receive data buffer
void MysqlCommand::ResultContext::initFieldBuffer(enum_field_types fieldType, int col, unsigned long length, unsigned int flags)
{
   bool isUnsigned = (flags & UNSIGNED_FLAG) != 0;

   switch(fieldType)
   {
      case MYSQL_TYPE_BIT:
      case MYSQL_TYPE_TINY:
         if (isUnsigned)
            fieldBuffers[col] = uint8_t();
         else
            fieldBuffers[col] = int8_t();
         break;
      case MYSQL_TYPE_YEAR:
         fieldBuffers[col] = int32_t();
         break;
      case MYSQL_TYPE_SHORT: // SMALLINT
         if (isUnsigned)
            fieldBuffers[col] = uint16_t();
         else
            fieldBuffers[col] = int16_t();
         break;
      case MYSQL_TYPE_INT24: // 24 BIT  
      case MYSQL_TYPE_LONG:
         if (isUnsigned)
            fieldBuffers[col] = uint32_t();
         else
            fieldBuffers[col] = int32_t();

         break;
      case MYSQL_TYPE_LONGLONG:
         if (isUnsigned)
            fieldBuffers[col] = uint64_t();
         else      
            fieldBuffers[col] = int64_t();

         break;
      case MYSQL_TYPE_FLOAT:
         fieldBuffers[col] = float();
         break;
      case MYSQL_TYPE_DOUBLE:
         fieldBuffers[col] = double();
         break;
      case MYSQL_TYPE_NEWDECIMAL:
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_VARCHAR:
      {
         bool isBinary = flags & BINARY_FLAG;
         if (isBinary)
            fieldBuffers[col] = std::vector<uint8_t>(length); 
         else
            fieldBuffers[col] = std::vector<char>(length);
      }
         break;
      case MYSQL_TYPE_TIME:
      case MYSQL_TYPE_DATE:
      case MYSQL_TYPE_DATETIME:
      case MYSQL_TYPE_TIMESTAMP:
         fieldBuffers[col] = MysqlTime();
         break;
      case MYSQL_TYPE_TINY_BLOB:
      case MYSQL_TYPE_MEDIUM_BLOB:
      case MYSQL_TYPE_LONG_BLOB:
      case MYSQL_TYPE_BLOB:
      {
         bool isBinary = flags & BINARY_FLAG;
         if (isBinary)
            fieldBuffers[col] = std::vector<uint8_t>(length); 
         else
            fieldBuffers[col] = std::vector<char>(length); 
      }
         break;
      default:
         throw tbs::SqlException("unsupported Mysql data type received from backend", "MysqlCommand");
         break;
   }
}

// Get pointer to the receive data buffer
void* MysqlCommand::ResultContext::getFieldBufferPointer(enum_field_types fieldType, int col, unsigned int flags)
{
   std::string errMsg = "Invalid variant storage for mysql type " + mysqlDataTypeToString(fieldType);  
   
   bool isUnsigned = (flags & UNSIGNED_FLAG) != 0;
   
   try
   {
      auto& storage = fieldBuffers[col];
      switch(fieldType)
      {
         case MYSQL_TYPE_BIT:
         case MYSQL_TYPE_TINY:
            if (isUnsigned)
               return (void*) &(VariantHelper::value<uint8_t>(storage, errMsg));
            else
               return (void*) &(VariantHelper::value<int8_t>(storage, errMsg));
         case MYSQL_TYPE_YEAR:
            return (void*) &(VariantHelper::value<int32_t>(storage, errMsg));
         case MYSQL_TYPE_SHORT: // SMALLINT
            if (isUnsigned)
               return (void*) &(VariantHelper::value<uint16_t>(storage, errMsg));
            else
               return (void*) &(VariantHelper::value<int16_t>(storage, errMsg));
         case MYSQL_TYPE_INT24:
         case MYSQL_TYPE_LONG:
            if (isUnsigned)
               return (void*) &(VariantHelper::value<uint32_t>(storage, errMsg));
            else
               return (void*) &(VariantHelper::value<int32_t>(storage, errMsg));
         case MYSQL_TYPE_LONGLONG:
            if (isUnsigned)
               return (void*) &(VariantHelper::value<uint64_t>(storage, errMsg));
            else
               return (void*) &(VariantHelper::value<int64_t>(storage, errMsg));
         case MYSQL_TYPE_FLOAT:
            return (void*) &(VariantHelper::value<float>(storage, errMsg));
         case MYSQL_TYPE_DOUBLE:
            return (void*) &(VariantHelper::value<double>(storage, errMsg));
         case MYSQL_TYPE_NEWDECIMAL:
         case MYSQL_TYPE_STRING:
         case MYSQL_TYPE_VAR_STRING:
         case MYSQL_TYPE_VARCHAR:
         {
            bool isBinary = flags & BINARY_FLAG;
            if (isBinary)
            {
               auto& vec = VariantHelper::value<std::vector<uint8_t>>(storage, errMsg);
               return (void*) vec.data();
            }
            else
            {
               auto& vec = VariantHelper::value<std::vector<char>>(storage, errMsg);
               return (void*) vec.data();
            }   
         }
            break;
         case MYSQL_TYPE_TIME:
         case MYSQL_TYPE_DATE:
         case MYSQL_TYPE_DATETIME:
         case MYSQL_TYPE_TIMESTAMP:
         {
            const MysqlTime& data = VariantHelper::value<MysqlTime>(storage, errMsg);
            return (MYSQL_TIME*) &(data.myTime);
         }
            break;
         case MYSQL_TYPE_TINY_BLOB:
         case MYSQL_TYPE_MEDIUM_BLOB:
         case MYSQL_TYPE_LONG_BLOB:
         case MYSQL_TYPE_BLOB:
         {
            bool isBinary = flags & BINARY_FLAG;
            if (isBinary)
            {
               auto& vec = VariantHelper::value<std::vector<uint8_t>>(storage, errMsg);
               return (void*) vec.data();
            }
            else
            {
               auto& vec = VariantHelper::value<std::vector<char>>(storage, errMsg);
               return (void*) vec.data();
            }
         }
            break;
         default:
            throw tbs::SqlException("unsupported Mysql data type received from backend", "MysqlCommand");
            break;
      }
   }
   catch (const tbs::VariantException& ex)
   {
      throw tbs::SqlException(ex.what(), "MysqlCommand");
   }
   catch (const tbs::TypeException& ex)
   {
      throw tbs::SqlException(ex.what(), "MysqlCommand");
   }

   return nullptr;
}


} // namespace sql
} // namespace tbs