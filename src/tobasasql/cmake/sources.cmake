file(GLOB TOBASASQL_CORE_SOURCES_LIST CONFIGURE_DEPENDS
   ${CMAKE_CURRENT_SOURCE_DIR}/src/database_*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/sql_*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/util.cpp
)

if(TOBASA_SQL_USE_PGSQL)
   file(GLOB TOBASASQL_PGSQL_SOURCES_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/src/pgsql*.cpp 
   )
endif()

if(TOBASA_SQL_USE_ADODB AND MSVC)
   file(GLOB TOBASASQL_ADODB_SOURCES_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/src/adodb*.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/com_variant_helper.cpp
   )
endif()

if(TOBASA_SQL_USE_ODBC)
   file(GLOB TOBASASQL_ODBC_SOURCES_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/src/odbc*.cpp
   )   
endif()   

if(TOBASA_SQL_USE_SQLITE)
   file(GLOB TOBASASQL_SQLITE_SOURCES_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/src/sqlite*.cpp
   )
endif()   

if(TOBASA_SQL_USE_MYSQL)
   file(GLOB TOBASASQL_MARIADB_SOURCES_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/src/mysql*.cpp
   )
endif()



file(GLOB TOBASASQL_CORE_HEADERS_LIST CONFIGURE_DEPENDS
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/column_info.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/column_lookup_info.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/column_map.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/common_types.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/database_connector_base.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/database_connector.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/exception.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/settings.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_conn_variant.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_connection_common.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_connection.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_dataset.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_defines.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_driver.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_log_identifier.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_parameter_rewriter.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_parameter.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_query_option.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_query.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_result.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_result_common.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_service_base.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_table.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sql_transaction.h
   ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/util.h
)

if(TOBASA_SQL_USE_PGSQL)
   file(GLOB TOBASASQL_PGSQL_HEADERS_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/pgsql*.h
   )
endif()

if(TOBASA_SQL_USE_ADODB AND MSVC)
   file(GLOB TOBASASQL_ADODB_HEADERS_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/adodb*.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/com_variant_helper.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/com_variant.h
   )
endif()

if(TOBASA_SQL_USE_ODBC)
   file(GLOB TOBASASQL_ODBC_HEADERS_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/odbc*.h
   )
endif()

if(TOBASA_SQL_USE_SQLITE)
   file(GLOB TOBASASQL_SQLITE_HEADERS_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/sqlite*.h
   )
endif()

if(TOBASA_SQL_USE_MYSQL)
   file(GLOB TOBASASQL_MARIADB_HEADERS_LIST CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/include/tobasasql/mysql*.h
   )
endif()