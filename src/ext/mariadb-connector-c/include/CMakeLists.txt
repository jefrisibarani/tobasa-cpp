SET(MARIADB_CLIENT_INCLUDES ${CC_SOURCE_DIR}/include/mariadb_com.h
                            ${CC_SOURCE_DIR}/include/mysql.h
                            ${CC_SOURCE_DIR}/include/mariadb_stmt.h
                            ${CC_SOURCE_DIR}/include/ma_pvio.h
                            ${CC_SOURCE_DIR}/include/ma_tls.h
                            ${CC_BINARY_DIR}/include/mariadb_version.h
                            ${CC_SOURCE_DIR}/include/ma_list.h
                            ${CC_SOURCE_DIR}/include/errmsg.h
                            ${CC_SOURCE_DIR}/include/mariadb_dyncol.h
                            ${CC_SOURCE_DIR}/include/mariadb_ctype.h
                            ${CC_SOURCE_DIR}/include/mariadb_rpl.h
                            )
IF(NOT IS_SUBPROJECT)
  SET(MARIADB_CLIENT_INCLUDES ${MARIADB_CLIENT_INCLUDES}
                              ${CC_SOURCE_DIR}/include/mysqld_error.h
                              )
ENDIF()
SET(MYSQL_ADDITIONAL_INCLUDES
   ${CC_SOURCE_DIR}/include/mysql/client_plugin.h
   ${CC_SOURCE_DIR}/include/mysql/plugin_auth_common.h
   ${CC_SOURCE_DIR}/include/mysql/plugin_auth.h
   )
SET(MARIADB_ADDITIONAL_INCLUDES
   ${CC_SOURCE_DIR}/include/mariadb/ma_io.h
   )
IF(WIN32)
  SET(WIX_INCLUDES ${MARIADB_CLIENT_INCLUDES} ${MARIADB_ADDITIONAL_INCLUDES} ${MYSQL_ADDITIONAL_INCLUDES} PARENT_SCOPE)
ENDIF()

INSTALL(FILES
   ${MARIADB_CLIENT_INCLUDES}
   DESTINATION ${INSTALL_INCLUDEDIR}
   COMPONENT Development)
INSTALL(FILES
   ${MYSQL_ADDITIONAL_INCLUDES}
   DESTINATION ${INSTALL_INCLUDEDIR}/mysql
   COMPONENT Development)
INSTALL(FILES
   ${MARIADB_ADDITIONAL_INCLUDES}
   DESTINATION ${INSTALL_INCLUDEDIR}/mariadb
   COMPONENT Development)

# -----------------------------------------------------------------------------
# NOTE_JEFRI: Added
# Prepare mariadb client header files for dependend projects and building examples
# -----------------------------------------------------------------------------
set(TOBASA_MARIADB_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR})

foreach(header0 ${MARIADB_CLIENT_INCLUDES})
    file(COPY ${header0} DESTINATION ${TOBASA_MARIADB_INCLUDE_DIR})
endforeach()

foreach(header1 ${MYSQL_ADDITIONAL_INCLUDES})
    file(COPY ${header1} DESTINATION ${TOBASA_MARIADB_INCLUDE_DIR}/mysql)
endforeach()

foreach(header2 ${MYSQL_ADDITIONAL_INCLUDES})
    file(COPY ${header2} DESTINATION ${TOBASA_MARIADB_INCLUDE_DIR}/mariadb)
endforeach()

message(STATUS "TOBASA_MARIADB_INCLUDE_DIR  ${TOBASA_MARIADB_INCLUDE_DIR}")
# -----------------------------------------------------------------------------