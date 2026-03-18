#pragma once



#if ! ( defined(TOBASA_SQL_USE_SQLITE) || defined(TOBASA_SQL_USE_PGSQL)  || (defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)) || defined(TOBASA_SQL_USE_ODBC) || defined(TOBASA_SQL_USE_MYSQL) ) 
   #define TOBASA_SQL_USE_SQLITE
#endif 


#if defined(TOBASA_SQL_USE_SQLITE) && !(defined(TOBASA_SQL_USE_PGSQL)  || (defined(TOBASA_SQL_USE_ADODB) && defined(_MSC_VER)) || defined(TOBASA_SQL_USE_ODBC) || defined(TOBASA_SQL_USE_MYSQL) ) 
   #define TOBASA_SQL_USE_ONLY_SQLITE
#endif 