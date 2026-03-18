#include <string>
#include "tobasasql/column_info.h"
#include "tobasasql/util.h"
#include "tobasasql/pgsql_util.h"
#include "tobasasql/pgsql_table_helper.h"

namespace tbs {
namespace sql {


bool PgsqlTableHelper::isView(PgsqlConnection& connection )
{
   SqlApplyLogInternal applyLogRule(&connection);

   std::string sql = tbsfmt::format("SELECT viewname FROM pg_views WHERE viewname='{}'", _tableName);
   if (connection.executeScalar(sql) == _tableName) {
      return true;
   }

   return false;
}

bool PgsqlTableHelper::setupColumnInfo(PgsqlResult& tableResult, ColumnInfo* columnInfo)
{
   Oid tableOid = tableResult.tableOid();
   if (tableOid == -1)
      return false;

   // setup primary key column numbers
   bool hasPrimaryKey = false;
   std::string primaryKeyValues = "";

   // temporarily disable sql logging
   SqlApplyLogInternal applyLogRule(tableResult.connection());

   std::string relid = std::to_string(long(tableOid));
   std::string sql = "SELECT conkey FROM pg_constraint where contype= 'p' AND conrelid=" + relid;
   std::string pkColNumbers = tableResult.connection()->executeScalar(sql);

   if (pkColNumbers.length() >= 3)
   {
      pkColNumbers.pop_back();
      pkColNumbers     = pkColNumbers.substr(1);
      primaryKeyValues = pkColNumbers;
      hasPrimaryKey    = true;
   }

   std::string sqlCmd =
      "  SELECT n.nspname AS nspname, relname, t.typname AS type, format_type(t.oid,NULL) AS typname, format_type(t.oid, att.atttypmod) AS displaytypname, "
      "   nt.nspname AS typnspname, attname, attnum, COALESCE(b.oid, t.oid) AS basetype, atthasdef, adsrc,\n"
      "       CASE WHEN t.typbasetype::oid=0 THEN att.atttypmod else t.typtypmod END AS typmod,\n"
      "       CASE WHEN t.typbasetype::oid=0 THEN att.attlen else t.typlen END AS typlen\n"
      "  FROM pg_attribute att\n"
      "  JOIN pg_type t ON t.oid=att.atttypid\n"
      "  JOIN pg_namespace nt ON nt.oid=t.typnamespace\n"
      "  JOIN pg_class c ON c.oid=attrelid\n"
      "  JOIN pg_namespace n ON n.oid=relnamespace\n"
      "  LEFT OUTER JOIN pg_type b ON b.oid=t.typbasetype\n"
      "  LEFT OUTER JOIN pg_attrdef def ON adrelid=attrelid AND adnum=attnum\n"
      "  WHERE attnum > 0 AND NOT attisdropped AND attrelid=" + relid + "::oid\n"
      //"WHERE attnum > 0 AND NOT attisdropped AND c.relname = " + tbs::sql::util::quote("") + "::name\n"
      " ORDER BY attnum";

   PgsqlResult colSet(tableResult.connection());
   colSet.runQuery(sqlCmd);

   if ( colSet.isValid() && colSet.totalColumns() > 0 )
   {
      // colSet.totalRows() should same as tableResult.totalColumns() !!
      for (int i = 0; i < tableResult.totalColumns(); i++)
      {
         colSet.navigator().locate(i);

         ColumnInfo& colInfo = columnInfo[i];
         long nativeType = std::stol(colSet.getStringValue("basetype"));
         colInfo.setName(colSet.getStringValue("attname"));
         colInfo.setNativeTypeStr(colSet.getStringValue("typname"));
         colInfo.setDisplayTypeName(colSet.getStringValue("displaytypname"));
         colInfo.setNativeType(nativeType);
         colInfo.setTypeClass(tableResult.columnTypeClass(i));
         colInfo.setDataType(sql::pgsqlDataTypeToDataType(nativeType));

         // TODO_JEFRI : do we really need to do this???
         // Special case for character datatypes. We always cast them to text to avoid
         // truncation issues with casts like ::character(3)
         if ( colInfo.getNativeTypeStr() == "character" ||
              colInfo.getNativeTypeStr() == "character varying" ||
              colInfo.getNativeTypeStr() == "\"char\"" )
         {
            colInfo.setDataType(DataType::text);
            colInfo.setNativeType(static_cast<long>(PgsqlType::text));
            colInfo.setNativeTypeStr("text");
         }

         if ( (columnInfo[i].getNativeType() == static_cast<long>(PgsqlType::int4) || colInfo.getNativeType() == static_cast<long>(PgsqlType::int8))
            && util::strToBool(colSet.getStringValue("atthasdef")) )
         {
            std::string adsrc = colSet.getStringValue("adsrc");

            if (  adsrc == "nextval('" + colSet.getStringValue("relname") + "_" + colInfo.getName() + "_seq'::text)" ||
                  adsrc == "nextval('" + colSet.getStringValue("nspname") + "." + colSet.getStringValue("relname") + "_" + colInfo.getName() + "_seq'::text)" ||
                  adsrc == "nextval('" + colSet.getStringValue("relname") + "_" + colInfo.getName() + "_seq'::regclass)" ||
                  adsrc == "nextval('" + colSet.getStringValue("nspname") + "." + colSet.getStringValue("relname") + "_" + colInfo.getName() + "_seq'::regclass)" )
            {
               if (columnInfo[i].getNativeType() == static_cast<long>(PgsqlType::int4))
                  colInfo.setNativeType(static_cast<long>(PgsqlType::serial));
               else
                  colInfo.setNativeType(static_cast<long>(PgsqlType::serial8));

               colInfo.setAutoIncrement();
            }
         }

         colInfo.setTypeLength( std::stol(colSet.getStringValue("typlen")) );
         colInfo.setTypeMod( std::stol(colSet.getStringValue("typmod")) );

         PgsqlType fieldType = (PgsqlType)columnInfo[i].getNativeType();
         switch (fieldType)
         {
            case PgsqlType::boolean:
            {
               colInfo.setNonNumeric();
               colInfo.setReadOnly(false);
               break;
            }
            case PgsqlType::int8:
            case PgsqlType::serial8:
            case PgsqlType::int4:
            case PgsqlType::serial:
            {
               colInfo.setNumeric();
               if (columnInfo[i].isAutoIncrement()) {
                  colInfo.setReadOnly(true);
               }

               break;
            }
            case PgsqlType::int2:
            case PgsqlType::oid:
            case PgsqlType::tid:
            case PgsqlType::xid:
            case PgsqlType::cid:
            {
               colInfo.setNumeric();
               break;
            }
            case PgsqlType::float4:
            case PgsqlType::float8:
            {
               colInfo.setNumeric();
               colInfo.setReadOnly(false);
               break;
            }
            case PgsqlType::money:
            {
               colInfo.setNonNumeric();
               colInfo.setReadOnly(false);
               break;
            }
            case PgsqlType::numeric:
            {
               colInfo.setNumeric();
               colInfo.setReadOnly(false);
               int len = colInfo.getSize();
               int prec = colInfo.getPrecision();
               if (prec > 0) {
                  len -= (prec);
               }

               break;
            }
            case PgsqlType::bytea:
            {
               colInfo.setNonNumeric();
               colInfo.setReadOnly(true);
               break;
            }
            case PgsqlType::date:
            {
               colInfo.setNonNumeric();
               colInfo.setReadOnly(false);
               break;
            }
            case PgsqlType::time:
            case PgsqlType::timetz:
            case PgsqlType::timestamp:
            case PgsqlType::timestamptz:
            case PgsqlType::interval:
            {
               colInfo.setNonNumeric();
               colInfo.setReadOnly(false);
               break;
            }
            case PgsqlType::character:
            case PgsqlType::name:
            case PgsqlType::text:
            default:
            {
               colInfo.setNonNumeric();
               colInfo.setReadOnly(false);
               colInfo.setNeedResize(true);
               break;
            }
         }

         if (! hasPrimaryKey)
         {
            // for security reasons, we need Oid or PK to enable updates.
            // If none is available, the table definition can be considered faulty.
            colInfo.setReadOnly(true);
         }
      }

      // -------------------------------------------------------
      // Setup primary key column(s)
      if (hasPrimaryKey)
      {
         // primaryKeyValues contain primary key columns position, (NOT zero based) separated by comma
         auto pkColIndexCollection = tbs::util::split(primaryKeyValues, ',');
         int columnPosition = 0;

         for (auto col = pkColIndexCollection.begin(); col != pkColIndexCollection.end(); ++col)
         {
            columnPosition = std::stoi(*col);
            columnInfo[columnPosition - 1].setPrimaryKey();

            std::string newNativeTypeStr = "[PK]";
            newNativeTypeStr += columnInfo[columnPosition-1].getNativeTypeStr();

            columnInfo[columnPosition - 1].setDisplayTypeName(newNativeTypeStr);
         }
      }

      // -------------------------------------------------------
      return true;
   }

   return false;
}

void PgsqlTableHelper::setTableName(const std::string& tableName)
{
   _tableName = tableName;
}

std::string PgsqlTableHelper::tableName() const
{
   return _tableName;
}


} // namespace sql
} // namespace tbs
