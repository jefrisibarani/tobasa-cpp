#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)

#include <tobasa/exception.h>
#include <tobasa/crypt.h>
#include "tobasasql/adodb_util.h"
#include "tobasasql/adodb_command.h"
#include <atlsafe.h>

namespace tbs {
namespace sql {

AdoCommand::AdoCommand(ADODB::_ConnectionPtr pConn)
   : _pConn(pConn)
{
   notifierSource = "AdoCommand";
}

bool AdoCommand::execute(const std::string& sql, int&  affectedRows, const AdoParameterCollection& parameters)
{
   ADODB::_CommandPtr pCommand = nullptr;

   try
   {
      Logger::logD("[sql] [AdoCommand] executing query, total parameter: {}", parameters.size());

      _variant_t vRecords;
      pCommand     = createNativeCommand(sql, parameters);
      long options = ADODB::adCmdText | ADODB::adExecuteNoRecords;
      auto result  = pCommand->Execute(&vRecords, nullptr, options);
      affectedRows = vRecords.iVal;

      return true;
   }
   catch (_com_error& e)
   {
      // pCommand is a _com_ptr_t, before it is going out of scope, detach its internal interface
      // otherwise its destrutor will throw
      auto ifc = pCommand.Detach();
      ifc = nullptr;

      throw e;
   }
   catch (const std::exception& e)
   {
      throw e;
   }

   return false;
}


ADODB::_RecordsetPtr AdoCommand::executeResult(const std::string& sql, const AdoParameterCollection& parameters)
{
   ADODB::_CommandPtr pCommand = nullptr;
   ADODB::_RecordsetPtr pResult = nullptr;

   try
   {
      Logger::logD("[sql] [AdoCommand] executing query, total parameter: {}", parameters.size());
      pCommand = createNativeCommand(sql, parameters);
      pResult  = pCommand->Execute(nullptr, nullptr, ADODB::adCmdText);

      return pResult;
   }
   catch(const _com_error& e)
   {
      // pCommand is a _com_ptr_t, before it is going out of scope, detach its internal interface
      // otherwise its destrutor will throw
      auto ifc = pCommand.Detach();
      ifc = nullptr;

      throw e;
   }
   catch(const std::exception& e)
   {
      throw e;
   }

   return nullptr;
}


ADODB::_ParameterPtr AdoCommand::createParameter( AdoParameter& param )
{
   ADODB::_ParameterPtr pParameter = nullptr;
   HRESULT hr = pParameter.CreateInstance(__uuidof(ADODB::Parameter));
   if (hr != S_OK) {
      _com_issue_error(hr);
   }

   _bstr_t paramName = util::utf8_to_bstr_t(param.name());
   ADODB::DataTypeEnum paramType = adoDataTypeFromDataType(param.type());

   // Note:
   // for date, time and timestamp, adoDataTypeToDataType does not convert to characters
   // so we do it here. We send date, time and timestamp as characters
   // https://docs.microsoft.com/en-us/dotnet/framework/data/adonet/sql/date-and-time-data
   // https://docs.microsoft.com/en-us/dotnet/framework/data/adonet/configuring-parameters-and-parameter-data-types
   // https://stackoverflow.com/questions/38662438/using-sql-server-datetime2-with-tadoquery-open
   // https://stackoverflow.com/questions/60695196/how-to-parameterize-12-30-1899-to-sql-server-native-client-when-datatypecompatil
   // https://stackoverflow.com/questions/38662438/using-sql-server-datetime2-with-tadoquery-open
   
   if (  paramType == ADODB::adDate
      || paramType == ADODB::adDBTime
      || paramType == ADODB::adDBTimeStamp)
   {
      paramType = ADODB::adVarWChar;
   }

   long paramSize = param.size();
   _variant_t paramValue;

   // for these types, we set paramSize to -1
   if (  param.type() == DataType::character
      || param.type() == DataType::varchar
      || param.type() == DataType::text
      || param.type() == DataType::date
      || param.type() == DataType::time
      || param.type() == DataType::timestamp)
   {
      paramSize = -1;
   }

   // Send numeric or decimal type as string to database
   if (param.type() == DataType::numeric)
   {
      paramSize = -1;
      paramType = ADODB::adVarWChar;
   }

   if (paramType == ADODB::adVarBinary)
   {
      if (std::holds_alternative<std::monostate>(param.value()))
      {
         paramValue.vt = VT_NULL;
         paramSize  = 0;
      }
      else
      {
         //#include <atlsafe.h>
         paramSize = -1;

         try
         {
            using namespace tbs::crypt;

            std::string hexBinaryStr = VariantHelper::toString( param.value() );
            size_t hexLenOri = hexBinaryStr.size();
            if (hexLenOri % 2) {
               throw std::exception("invalid binary string input");
            }

            // convert hexadecimal encoded data to a byte array
            size_t hexLen = hexLenOri / 2;
            std::vector<byte_t> pasBuffer(hexLen);
            hexDecode(hexBinaryStr, pasBuffer.data());

            SAFEARRAY* ppsa;
            // Create a safe array storing 'count' BYTEs
            const long count = static_cast<long>(hexLen);
            CComSafeArray<BYTE> sa(count);
            // Fill the safe array with some data
            for (long i = 0; i < count; i++)
            {
               sa[i] = pasBuffer.at(i);
            }

            ppsa = sa.Detach();
            paramValue.parray = ppsa;
            paramValue.vt = 8209;
         }
         catch (const CAtlException& e)
         {
            (void) e;
            throw std::exception("error creating SafeArray");
         }
         catch (const std::exception& e)
         {
            throw e;
         }
      }
   }
   else 
   {
      if (std::holds_alternative<std::monostate>(param.value()))
      {
         paramValue.vt = VT_NULL;
         paramSize  = 0;
      }
      else
         paramValue = VariantHelper::toNativeVariant( param.value() );
   }
   

   ADODB::ParameterDirectionEnum paramDirection = adoParamDirectionFromParamDirection( param.direction() );
   pParameter->Name      = paramName;
   pParameter->Type      = paramType;
   pParameter->Size      = paramSize;
   pParameter->Direction = paramDirection;
   pParameter->Value     = paramValue;

   // This note for .NET, is it also apply for c++ adodb?
   // Note: https://docs.microsoft.com/en-us/dotnet/api/system.data.sqlclient.sqlparameter.precision?redirectedfrom=MSDN&view=dotnet-plat-ext-6.0#System_Data_SqlClient_SqlParameter_Precision
   // You do not need to specify values for the Precision and Scale properties for input parameters,
   // as they can be inferred from the parameter value
   
   if (!std::holds_alternative<std::monostate>(param.value()))
   {
      if (paramDirection == ADODB::adParamOutput && paramType == ADODB::adNumeric)
      {
         pParameter->Precision    = static_cast<unsigned char>(param.size());
         pParameter->NumericScale = static_cast<unsigned char>(param.decimalDigits());
      }
   }

   return pParameter;
}


ADODB::_CommandPtr AdoCommand::createNativeCommand(const std::string& sql, const AdoParameterCollection& parameters)
{
   ADODB::_CommandPtr pCommand = nullptr;

   try
   {
      HRESULT hr = pCommand.CreateInstance(__uuidof(ADODB::Command));
      if (hr != S_OK) {
         _com_issue_error(hr);
      }

      _bstr_t sqlCmdBW = util::utf8_to_bstr_t(sql);
      pCommand->CommandText = sqlCmdBW; /*(_bstr_t)sql.c_str()*/
      pCommand->CommandType = ADODB::adCmdText;

      for (size_t i = 0; i < parameters.size(); i++)
      {
         auto sqlParam = parameters.at(i);
         auto parameter = createParameter(*sqlParam);
         pCommand->Parameters->Append(parameter);
      }

      pCommand->ActiveConnection = _pConn;

      return pCommand;
   }
   catch (_com_error& e)
   {
      pCommand = nullptr;
      throw e;
   }
   catch (std::exception& e)
   {
      throw e;
   }

   return nullptr;
}

} // namespace sql
} // namespace tbs

#endif // defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)