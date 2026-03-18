#include "tobasasql/mysql_command.h"
#include "tobasasql/mysql_result.h"

namespace tbs {
namespace sql {

bool isStringOrBlobField(MySqlType type)
{
   return (type == MYSQL_TYPE_TINY_BLOB || type == MYSQL_TYPE_MEDIUM_BLOB || type == MYSQL_TYPE_LONG_BLOB || type == MYSQL_TYPE_BLOB ||
      type == MYSQL_TYPE_VARCHAR || type == MYSQL_TYPE_VAR_STRING || type == MYSQL_TYPE_STRING );
}

MysqlResult::MysqlResult(MysqlConnection* pconn)
   : ResultCommon()
{
   _pConn = pconn;
   notifierSource = "MysqlResult";
   _navigator.init( std::bind(&MysqlResult::totalRows, this) );
}

MysqlResult::~MysqlResult() 
{
}

// -------------------------------------------------------
// Specific implementation methods
// -------------------------------------------------------

std::string MysqlResult::name() const
{
   return "Mysql Result";
}

bool MysqlResult::runQuery(const std::string& sql, const MysqlParameterCollection& parameters)
{
   if (_pConn == nullptr)
      return false;

   if (_pConn->status() != ConnectionStatus::ok)
      return false;

   if (_optionOpenTable)
      _qryStr = "SELECT * FROM " + sql;
   else
      _qryStr = sql;

   if (_pConn->logSqlQuery())
      onNotifyDebug(_pConn->logId() + tbsfmt::format("runQuery: {}", _qryStr));

   _nRows = 0;
   _nColumns = 0;
   MysqlCommand cmd(_pConn->nativeConnection());
   cmd.init(_qryStr, parameters);
   _pDataset = cmd.executeResult();
   if (_pDataset)
   {
      _affectedRows = static_cast<int>(cmd.affectedRows());
      _nColumns     = _pDataset->totalColumns;
      _nRows        = _pDataset->totalRows;
      
      if (_nColumns > 0)
      {
         // we have total columns, now set up columns info
         setupColumnProperties(cmd._pResultContext->fields);

         if (_pConn->logExecuteStatus()) 
            onNotifyTrace(_pConn->logId() + tbsfmt::format("SQL command executed successfully, row: {} column: {}, affectedRows: {}", _nRows, _nColumns, _affectedRows));
      }

      if (_nRows > 0)
         _resultStatus = ResultStatus::tuplesOk;
      else
         _resultStatus = ResultStatus::commandOk;

      _navigator.moveFirst();

      return true;
   }

   return false;
}

void MysqlResult::connection(MysqlConnection* conn)
{
   _pConn = conn;
}

MysqlConnection* MysqlResult::connection() const { return _pConn; }

NavigatorBasic& MysqlResult::navigator() { return _navigator; }

// -------------------------------------------------------
// Override methods from base class : ResultCommon
// -------------------------------------------------------

TypeClass MysqlResult::columnTypeClass(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);
 
   TypeClass retVal;
   long nativeType = _columnInfoCollection[columnIndex].nativeType;
 
   MySqlType myType = (MySqlType)nativeType;
   
   if (isStringOrBlobField(myType))
   {
      if ( _metadataCollection[columnIndex].isBinary)
         retVal = TypeClass::blob;
      else
         retVal = TypeClass::string;
   }
   else
      retVal = typeClassFromMySqlType(nativeType);
 
   return retVal;
}

MysqlResult::VariantType MysqlResult::getVariantValue(const int columnIndex) const
{
   long row = _navigator.position();
   throwIfColumnIndexInvalid(columnIndex);
   throwIfRowIndexInvalid(row);

   VariantType value = _pDataset->data.at(row).at(columnIndex);
   return value;
}

MysqlResult::VariantType MysqlResult::getVariantValue(const std::string& columnName) const
{
   return getVariantValue(columnNumber(columnName));
}

std::string MysqlResult::getStringValue(const int columnIndex) const
{
   long row = _navigator.position();
   throwIfColumnIndexInvalid(columnIndex);
   throwIfRowIndexInvalid(row);

   auto& value = _pDataset->data.at(row).at(columnIndex);
   return MysqlVariantHelper::toString(value);
}

std::string MysqlResult::getStringValue(const std::string& columnName) const
{
   return getStringValue(columnNumber(columnName));
}

bool MysqlResult::isNullField(const int columnIndex) const
{
   throwIfColumnIndexInvalid(columnIndex);
   throwIfRowIndexInvalid(_navigator.position());
   // TODO_JEFRI : do with better way
   return getStringValue(columnIndex) == sql::NULLSTR;
}

void MysqlResult::setupColumnProperties(const std::vector<MYSQL_FIELD*>& fieldsInfo)
{
   if (_nColumns <= 0)
      return;

   _metadataCollection.reserve(_nColumns);
   _columnInfoCollection.reserve(_nColumns);
   for (int i = 0; i < _nColumns; i++)
   {
      _columnInfoCollection.emplace_back(ColumnInfo());
      _metadataCollection.emplace_back(ColumnMetadata());
   }

   try
   {
      for (int i = 0; i < _nColumns; i++)
      {
         MYSQL_FIELD *field = fieldsInfo[i];   

         // save column name
         _columnInfoCollection[i].name = std::string(field->name);

         // save column defined size
         _columnInfoCollection[i].definedSize = field->length;

         MySqlType myType = field->type;

         // TODO_JEFRI
         if (isStringOrBlobField(myType))
         {
            //bool isBlobField = field->flags & BLOB_FLAG;
            bool isBinary    = field->flags & BINARY_FLAG;
            
            _metadataCollection[i].isBinary = isBinary;
            
            if (field->length == 255) {}
            // 262140 TEXT
            // 1020 TINY TEXT
            // 255 TINY BLOB and BLOB
            // 67108860 MEDIUM TEXT
            // 16777215 MEDIUM BLOB
            // 4294967295 LONG TEXT
            // 4294967295 LONG BLOB
         }

         std::string nativeTypeStr = sql::mysqlDataTypeToString(myType);
         std::string declaredTypeStr = nativeTypeStr;

         //char* declaredType = mysql_field_type(_pConn, field->type);
         //if (declaredType)
         //   declaredTypeStr = std::string(declaredType);

         // save column native type as string
         _columnInfoCollection[i].nativeTypeStr = nativeTypeStr;

         // save column native declared type as string
         _columnInfoCollection[i].nativeFullTypeStr = declaredTypeStr;

         // save column native type
         _columnInfoCollection[i].nativeType = (long) myType;

         // save column data type : sql::DataType
         if (isStringOrBlobField(myType))
         {
            bool isBinary = field->flags & BINARY_FLAG;
            //bool isBlobField = field->flags & BLOB_FLAG;

            if (!isBinary)
               _columnInfoCollection[i].dataType = DataType::varchar;
            else
               _columnInfoCollection[i].dataType = sql::mysqlDataTypeToDataType(myType);
         }
         else
            _columnInfoCollection[i].dataType = sql::mysqlDataTypeToDataType(myType);

         if (myType == MYSQL_TYPE_DECIMAL || myType == MYSQL_TYPE_NEWDECIMAL)
         {
            _columnInfoCollection[i].numericScale = (short) field->decimals;
            _columnInfoCollection[i].precision = (short) field->length - field->decimals;
         }

         if (myType == MYSQL_TYPE_FLOAT  || myType == MYSQL_TYPE_DOUBLE)
         {
            _columnInfoCollection[i].precision = (short) field->decimals;
         } 
         
         _columnInfoCollection[i].primaryKey = field->flags & PRI_KEY_FLAG;
         _columnInfoCollection[i].autoIncrement = field->flags & AUTO_INCREMENT_FLAG;
         _columnInfoCollection[i].allowNull = ! (field->flags & NOT_NULL_FLAG);
      }
   }
   catch (const TypeException & ex)
   {
      onNotifyError(_pConn->logId() + ex.what());
      throw tbs::SqlException(tbsfmt::format("setupColumnProperties, {}", ex.what()), "MysqlResult");
   }
}

} // namespace sql
} // namespace tbs