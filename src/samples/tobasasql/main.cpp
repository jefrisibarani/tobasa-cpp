#include <iostream>
#include <tobasa/datetime.h>
#include <tobasa/logger.h>
#include <tobasasql/sql_driver.h>
#include <tobasasql/sql_connection.h>
#include <tobasasql/sql_query.h>
#include <tobasasql/sql_table.h>


int main()
{
   using namespace tbs;

   if (! tbs::DateTime::initTimezoneData())
      return 1;

   std::cout << "TOBASA SQL Drivers\n";
   
   // We have to set log target for tbs::Logger
   tbs::Logger::setTarget(new tbs::log::CoutLogSink()) ;

   std::string connString;

#if defined(TOBASA_SQL_USE_SQLITE)   
   try
   {
      connString = "Database=./tbs_coba.db3;OpenCreate=True;OpenMemory=False;Password=AdmBaru98;";
      sql::SqlConnection<sql::SqliteDriver> conn;
      if (conn.connect(connString))
      {
            auto currentTime = conn.executeScalar("SELECT CURRENT_TIMESTAMP");
            std::cout << "SQLITE Driver\n";
            std::cout << "Current time    : " << currentTime << "\n";
            std::cout << "Backend Version : " << conn.versionString() << "\n";
            std::cout << "\n";
      }
   }
   catch(const std::exception& ex)
   {
      std::cerr << "Exception : " << ex.what();
   }            
#endif

#if defined(TOBASA_SQL_USE_PGSQL)
   try
   {
      connString = "dbname=postgres user=postgres password=@PGuser2004 hostaddr=10.62.22.2 port=5462";
      sql::SqlConnection<sql::PgsqlDriver> conn;
      if (conn.connect(connString))
      {
         auto currentTime = conn.executeScalar("SELECT CURRENT_TIMESTAMP");
         std::cout << "PGSQL Driver\n";
         std::cout << "Current time    : " << currentTime << "\n";
         std::cout << "Backend Version: " << conn.versionString() << "\n";
         std::cout << "\n";
         conn.disconnect();
      }
   }
   catch(const std::exception& ex)
   {
      std::cerr << "Exception : " << ex.what();
   }
#endif

#if defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)
   try
   {
      std::cout << "\n";
      if ( FAILED(::CoInitializeEx(NULL, COINIT_MULTITHREADED)) )
         std::cerr << "Initializing ADODB COM library has failed" << "\n";
      else
         std::cout << "Initializing ADODB COM library" << "\n";

      connString = "Provider=SQLNCLI11;Server=10.62.22.2;Database=master;Uid=tbs_user;Pwd=AdmBaru98;DataTypeCompatibility=80;";
      sql::SqlConnection<sql::AdodbDriver> conn;
      if (conn.connect(connString))
      {
         auto currentTime = conn.executeScalar("SELECT GETDATE()");
         std::cout << "ADODB Driver\n";
         std::cout << "Current time    : " << currentTime << "\n";
         std::cout << "Backend Version : " << conn.versionString() << "\n";
         conn.disconnect();
      } 
      std::cout << "Uninitializing ADODB COM library\n";
      std::cout << "\n";
      ::CoUninitialize();              
   }
   catch(const std::exception& ex)
   {
      std::cerr << "Exception : " << ex.what();
   }      
#endif

#if defined(TOBASA_SQL_USE_ODBC)
   try
   {
      connString = "Driver={ODBC Driver 17 for SQL Server};Server=10.62.22.2;Database=master;Uid=tbs_user;Pwd=AdmBaru98;TrustServerCertificate=Yes;";
      sql::SqlConnection<sql::OdbcDriver> conn;
      if (conn.connect(connString))
      {
         auto currentTime = conn.executeScalar("SELECT GETDATE()");
         std::cout << "ODBC Driver\n";
         std::cout << "Current time    : " << currentTime << "\n";
         std::cout << "Backend Version : " << conn.versionString() << "\n";
         std::cout << "\n";
         conn.disconnect();
      }      
   }
   catch(const std::exception& ex)
   {
      std::cerr << "Exception : " << ex.what();
   }      
#endif

#if defined(TOBASA_SQL_USE_MYSQL)
   try
   {
      using namespace tbs::sql;
      using SqlParameterCollection = MysqlParameterCollection ;
      using SqlParameter = MysqlParameter;
      using VariantHelper = MysqlVariantHelper;

      connString = "Database=tobasa_base_manager;User=tbs_user;Password=AdmBaru98;Server=10.62.22.2;Port=3306";
      SqlConnection<MysqlDriver> conn;
      if (conn.connect(connString))
      {
            conn.setLogSqlQuery(true);
            conn.setLogSqlQueryInternal(true);
            conn.setLogExecuteStatus(true);

            std::string sql = R"-(
                  SELECT u.id FROM base_users u WHERE
                  u.id=? AND u.id IN
                     (SELECT ur.user_id FROM base_user_role ur
                     JOIN base_roles r ON ur.role_id = r.id
                     WHERE r.name=? AND r.sysrole=? AND r.enabled=?) )-";

            SqlQuery<MysqlDriver> query(conn, sql, ParameterStyle::native);
            query.addParam("id",      sql::DataType::integer, 1L);
            query.addParam("name",    sql::DataType::varchar, std::string("Admin"));
            query.addParam("sysrole", sql::DataType::boolean, true);
            query.addParam("enabled", sql::DataType::boolean, true);

            std::string result = query.executeScalar();
         #if 0
         {
            auto currentTime = conn.executeScalar("SELECT NOW()");
            std::cout << "MYSQL Driver\n";
            std::cout << "Current time    : " << currentTime << "\n";
            std::cout << "Backend Version : " << conn.versionString() << "\n";

            auto numVal = conn.executeScalar("SELECT CAST(345 AS INTEGER)");
            std::cout << "numVal int      : " << numVal << "\n";

            auto bitVal1 = conn.executeScalar("SELECT b'1000001'"); // MySQL treat this as MYSQL_TYPE_VAR_STRING
            std::cout << "bitVal 1        : " << bitVal1 << "\n";

            auto bitVal2 = conn.executeScalar("SELECT vbit FROM sampledata"); // MySQL treat this as MYSQL_TYPE_BIT
            std::cout << "bitVal 2        : " << bitVal2 << "\n";

            auto numVal2 = conn.executeScalar("SELECT CAST(34.54 AS DOUBLE);)");
            std::cout << "numVal2 double  : " << numVal2 << "\n";
         }
         #endif

         #if 0
         {
            SqlParameterCollection param;
            param.push_back(std::make_shared<SqlParameter>("id",  DataType::bigint, (int32_t)1));
            auto res0 = conn.executeScalar("SELECT * FROM mysql_fields WHERE id = ?", param);
            auto res1 = conn.executeScalar("SELECT * FROM mysql_fields");
            auto res2 = conn.execute("SELECT vchar, vvarchar FROM mysql_fields");

            SqlResult<MysqlDriver> sqlResult(conn);
            sqlResult.setOptionCacheData(true);
            sqlResult.setOptionOpenTable(false);
            sqlResult.runQuery("SELECT * FROM mysql_fields LIMIT 1 OFFSET 0");
            if (sqlResult.isValid() && sqlResult.totalRows() > 0)
            {
               long totalRows       = sqlResult.totalRows();
               sqlResult.moveLast();

               auto vdate         = sqlResult.getVariantValue("vdate");
               auto vdatetime     = sqlResult.getVariantValue("vdatetime");
               auto vtimestamp    = sqlResult.getVariantValue("vtimestamp");
               auto vtime         = sqlResult.getVariantValue("vtime");
               auto vyear         = sqlResult.getVariantValue("vyear");
               auto vdateStr      = VariantHelper::toString(vdate);
               auto vdatetimeStr  = VariantHelper::toString(vdatetime);
               auto vtimestampStr = VariantHelper::toString(vtimestamp);
               auto vtimeStr      = VariantHelper::toString(vtime);               
               auto vyearStr      = VariantHelper::toString(vyear);
               
               auto vdateS        = sqlResult.getStringValue("vdate");
               auto vdatetimeS    = sqlResult.getStringValue("vdatetime");
               auto vtimestampS   = sqlResult.getStringValue("vtimestamp");
               auto vtimeS        = sqlResult.getStringValue("vtime");
               auto vyearS        = sqlResult.getStringValue("vyear");
            }
         }
         #endif

         // bool foo = "WEWEWEWEWE";  // In C++, the conversion from a string literal (like "WEWEWEWEWE") to a boolean value is allowed. 


         SqlParameterCollection param0;
         param0.push_back(std::make_shared<SqlParameter>("vchar",      DataType::character, std::string("WEWEWEWEWE")));
         param0.push_back(std::make_shared<SqlParameter>("vvarchar",   DataType::varchar,   std::string("HHHHHHHHHHHHHHHHHHHH") ));
         param0.push_back(std::make_shared<SqlParameter>("vbit",       DataType::tinyint,   (uint8_t)9 ));
         param0.push_back(std::make_shared<SqlParameter>("vbigint",    DataType::bigint,    8767676767676));
         // use HEX encoded string as blob data source
         param0.push_back(std::make_shared<SqlParameter>("vblob",      DataType::varbinary, std::string("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"), 24));
         // use std::vector<uint8_t> as blob data source
         std::vector<uint8_t> blobdata(24);
         crypt::hexDecode(std::string("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"), blobdata.data());
         param0.push_back(std::make_shared<SqlParameter>("vmediumblob", DataType::varbinary, std::move(blobdata), 24));
         
         #if 0
         {
            //auto resScalar0 = conn.executeScalar("INSERT INTO sampledata (vchar,vvarchar,vbit,vbigint) values (?, ? , ?, ? )", param0);
            auto affectedRows = conn.execute("INSERT INTO sampledata (vchar,vvarchar,vbit,vbigint,vblob,vmediumblob) values (?, ? , ?, ?, ?, ? )", param0);
            if (affectedRows>0)
            {
               auto lastId0 = conn.lastInsertRowid();
               SqlParameterCollection param1;
               param1.push_back(std::make_shared<SqlParameter>("id",  DataType::bigint, lastId0));
               auto resScalar1 = conn.executeScalar("SELECT vmediumblob FROM sampledata WHERE id = ?", param1);
            }
         }
         #endif   

         #if 0
         {
            //SqlQuery<MysqlDriver> query(&conn,"SELECT vmediumblob FROM sampledata WHERE id=?");
            //query.addParam("id",  DataType::bigint, (int64_t)34);
            //auto res = query.executeResult();

            int64_t lastId1 = -1;
            std::string newId = conn.executeScalar("INSERT INTO sampledata (vchar,vvarchar,vbit,vbigint) values (?, ? , ?, ? ) RETURNING id;", param0);
            if (util::isNumber(newId))
            {
               lastId1 = std::stoll(newId);
               if (lastId1>0)
               {
                  SqlParameterCollection param2;
                  param2.push_back(std::make_shared<SqlParameter>("id",  DataType::bigint, lastId1));
                  auto resScalar1 = conn.executeScalar("SELECT vvarchar FROM sampledata WHERE id = ?", param2);               
               }
            }
         }
         #endif

         #if 0
         {
            SqlTable<MysqlDriver> table(conn, "sampledata");
            table.init();
            if (table.isValid())
            {
               table.moveFirst();
               auto a = table.getStringValue("vdevimal");
               auto b = table.getVariantValue("vbigint");
               auto c = table.getVariantValue("vbit");
               auto d = table.getStringValue("vdate");
               auto e = table.getStringValue("vdatetime");
               auto f = table.getStringValue("vtimestamp");
               auto g = table.getStringValue("vtime");
               auto h = table.getStringValue("vyear");
               auto i = 1;
            }
         }
         #endif

         #if 0
         {
            SqlParameterCollection params;
            params.push_back(std::make_shared<SqlParameter>("id",         DataType::integer, (long)3));
            params.push_back(std::make_shared<SqlParameter>("val_bigint",  DataType::bigint, (int64_t)9223372036854775806));

            auto res0 = conn.execute("SELECT val_datetime FROM sampledata WHERE id = ? AND val_bigint = ?", params);
            auto res1 = conn.execute("UPDATE sampledata SET val_date='2002-04-03' WHERE id = ? AND val_bigint = ?", params);

            auto res2 = conn.executeScalar("SELECT val_datetime FROM sampledata WHERE id = ? AND val_bigint = ?", params);
            auto res3 = conn.executeScalar("UPDATE sampledata SET val_date='2002-04-27' WHERE id = ? AND val_bigint = ?", params);
         }
         #endif

         #if 0
         {
            auto res4 = conn.execute("UPDATE sampledata SET val_date='2002-04-10' WHERE id = 3 AND val_bigint = 9223372036854775806");
            auto res5 = conn.execute("SELECT val_datetime FROM sampledata WHERE id = 3 AND val_bigint = 9223372036854775806");

            auto res6 = conn.executeScalar("SELECT CAST(345 AS INTEGER)");
            auto res7 = conn.executeScalar("UPDATE sampledata SET val_date='2002-04-09' WHERE id = 3 AND val_bigint = 9223372036854775806");
         }
         #endif

         #if 0
         {
            SqlResult<MysqlDriver> sqlResult(conn);
            sqlResult.setOptionCacheData(true);
            sqlResult.setOptionOpenTable(false);

            sqlResult.runQuery("SELECT * FROM rawdata LIMIT 1 OFFSET 0");
            if (sqlResult.isValid() && sqlResult.totalRows() > 0)
            {
               long totalRows       = sqlResult.totalRows();
               sqlResult.moveLast();

               VariantType vId      = sqlResult.getVariantValue("id");
               VariantType vNote    = sqlResult.getVariantValue("note");
               VariantType vRawdata = sqlResult.getVariantValue("rawdata");
               auto vIdStr          = VariantHelper<>::toString(vId);
               auto vNoteStr        = VariantHelper<>::toString(vNote);
               auto vRawdataStr     = VariantHelper<>::toString(vRawdata);
               auto valNote         = sqlResult.getStringValue(1);
            }
         }
         #endif

         #if 0
         {
            using SqlParameter = MysqlParameter;
            using VariantType = MysqlVariantType;

            auto paramSelectId = std::make_shared<SqlParameter>("id", sql::DataType::integer, 1/*, sizeof(int)*/);

            sql::SqlTable<MysqlDriver> table(conn, "sampledata");
            table.getRetrieveDataOption()
               .pageSize(100)              // 10 records per page
               .pagePosition(1)           // set page position
               .orderBy("id ASC")
               .getParameters().push_back(paramSelectId);   // sql query now has one parameter => id

            table.init();

            if (table.isValid())
            {
               DateTime datetime;
               datetime.parse("2000-12-31 00:00:00");

               // TEST INSERT VariantType, make sure appendRows() first!
               //datetime.timePoint() += std::chrono::years{ 1 } + std::chrono::months{ 1 } + std::chrono::days{ 1 } + hours{ 1 } + minutes{ 1 } + seconds{1};
               datetime.timePoint() += tbsdate::years{ 5 };

               table.appendRows(1);
               table.moveLast();
               //table.setValue("val_bigint",     VariantType(9000000000000000000 + 111));
               table.setValue("val_varchar",    VariantType(std::string("ꦲꦭꦺꦴ") + "111"));
               table.setValue("val_text",       VariantType(std::string("77 ꦏꦢꦺꦴꦱ꧀ꦥꦸꦤ꧀ꦢꦶꦏꦧꦂꦫꦶꦥꦸꦤ? ") + "111"));
               table.setValue("val_datetime",   VariantType(datetime.format("{:%Y-%m-%d %H:%M:%S}")));
               table.setValue("val_date",       VariantType(datetime.format("{:%Y-%m-%d}")));
               table.setValue("val_time",       VariantType(datetime.format("{:%H:%M:%S}")));
               //table.setValue("val_char",       VariantType(std::string("Welcome") + "111"));
               table.setValue("val_real",       VariantType((float)27.1234567890 + (float)1));
               table.setValue("val_double",     VariantType((double)37.12345678901234567890 + (double)1));
               table.setValue("val_numeric",    VariantType(std::string("373444666778.68")));
               table.setValue("val_bool",       VariantType(false));
               table.setValue("val_bigint",     std::monostate{} );
               table.setValue("val_char",       std::monostate{} );
               table.saveTable();
            }
         }
         #endif

         std::cout << "\n";
         conn.disconnect();
      }     
   }
   catch(const std::exception& ex)
   {
      std::cerr << "Exception : " << ex.what();
   }
#endif

   return 0;
}