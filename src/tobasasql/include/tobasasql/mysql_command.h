#pragma once

#include <tobasa/notifier.h>
#include "tobasasql/sql_parameter.h"
#include "tobasasql/mysql_common.h"
#include "tobasasql/mysql_variant_helper.h"
#include "tobasasql/mysql_util.h"

#include <mysql/mysql.h>

namespace tbs {
namespace sql {

template <typename VariantTypeImplemented>
class DataSet;

class MysqlConnection;
class MysqlResult;

const uint64_t MYSQL_NON_AFFECTING_ROWS_QUERY = (unsigned long long) ~0;

class MysqlCommand : public Notifier
{
   using VariantType   = MysqlVariantType;
   using VariantHelper = MysqlVariantHelper;
   using VectorVariant = std::vector<VariantType>;
   using RecordVariant = std::vector<VectorVariant>;
   using DataSetPtr    = std::shared_ptr<DataSet<VariantType>>;

public:
   MysqlCommand(MysqlConnection* conn);
   ~MysqlCommand();

   /// Prepare for one-shot query execution
   bool query(const std::string& sql, const MysqlParameterCollection& parameters);

   bool prepare(const std::string& sql);
   bool bind(const MysqlParameterCollection& parameters);
   void reset();
   void close();

   /**
    * @brief Executes the prepared query.
    * @return The number of affected rows for INSERT, UPDATE, or DELETE;
    *         zero for SELECT and other queries that do not affect rows.
    */
   int execute();

   std::string executeScalar();

   /**
    * @brief Executes the prepared query and returns its result set.
    * @return A shared dataset for row-producing queries such as SELECT;
    *         an empty shared pointer for INSERT, UPDATE, or DELETE.
    */
   DataSetPtr executeResult();

   MysqlResult executeSqlResult();

   uint64_t affectedRows() { return _affectedRows; }

   std::string sqlCommandText() const { return _sql; }
   

   class ParameterContext
   {
      friend class MysqlCommand;
   public:
      ParameterContext() = default;
      ParameterContext(int totalParam)
      {
         binds.resize(totalParam);
         isNulls.resize(totalParam);
         errors.resize(totalParam);
         lengths.resize(totalParam);
      }

   private:
      std::vector<MYSQL_BIND>    binds;
      std::vector<my_bool>       isNulls;
      std::vector<my_bool>       errors;
      std::vector<unsigned long> lengths;
   };


   class ResultContext
   {
      friend class MysqlCommand;
      friend class MysqlResult;

   public:
      ~ResultContext() = default;
      ResultContext() = default;
      ResultContext(int columnsCount);

   private:
      void initFieldBuffer(enum_field_types fieldType, int col, unsigned long length, unsigned int flags=0);
      // Get pointer to the receive data buffer
      void* getFieldBufferPointer(enum_field_types fieldType, int col, unsigned int flags=0);

      int                        totalColumns;
      std::vector<MYSQL_FIELD*>  fields;
      std::vector<MYSQL_BIND>    binds;
      std::vector<my_bool>       pIsNulls;
      std::vector<my_bool>       pErrors;
      std::vector<unsigned long> pLengths;
      // Receive data buffer
      VectorVariant              fieldBuffers;
   };


   ResultContext* resultContext() const { return _pResultContext; }

private:

   MysqlConnection* _pConn;
   MYSQL*           _pMYConn;
   MYSQL_STMT*      _pStmt;
   MYSQL_RES*       _pResultMetadata;
   uint64_t         _affectedRows;
   std::string      _sql;

   /// Get last backend error.
   std::string lastBackendError();
   std::string statementError();

   ParameterContext _paramContext;
   ResultContext*   _pResultContext;
};


} // namespace sql
} // namespace tbs