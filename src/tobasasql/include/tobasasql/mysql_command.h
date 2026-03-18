#pragma once

#include "tobasasql/sql_parameter.h"
#include "tobasasql/mysql_common.h"
#include "tobasasql/mysql_variant_helper.h"
#include "tobasasql/mysql_util.h"
#include <mysql/mysql.h>

namespace tbs {
namespace sql {

template <typename VariantTypeImplemented>
struct DataSet;

class MysqlConnection;
class MysqlCommand;
class MysqlResult;

const uint64_t MYSQL_NON_AFFECTING_ROWS_QUERY = (unsigned long long) ~0;

class MysqlCommand
{
   using VariantType   = MysqlVariantType;
   using VectorVariant = std::vector<VariantType>;
   using VariantHelper = MysqlVariantHelper;

   friend class MysqlResult;

public:
   MysqlCommand(MYSQL *conn);
   ~MysqlCommand();

   bool init(const std::string& sql, const MysqlParameterCollection& parameters);

   /**
    * Execute the query
    * return affected rows
    * SELECT query affected rows is 0
    * INSERT,UPDATE,DELETE affected is >= 0
    */
   int execute();

   /**
    * Execute the query
    * For INSERT,UPDATE,DELETE query return nullptr
    * For SELECT query return std::shared_ptr<DataSet>
    */
   std::shared_ptr<DataSet<MysqlVariantType>> executeResult();

   uint64_t affectedRows() { return _affectedRows; }

private:
   MYSQL*         _pConn;
   MYSQL_STMT*    _pStmt;
   MYSQL_RES*     _pResultMetadata;
   uint64_t       _affectedRows;

   /// Get last backend error.
   std::string lastBackendError();
   std::string statementError();

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

   ParameterContext _paramContext;
   ResultContext* _pResultContext;
};


} // namespace sql
} // namespace tbs