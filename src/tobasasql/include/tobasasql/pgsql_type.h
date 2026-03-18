#pragma once

namespace tbs {
namespace sql {

// Note:
// https://www.postgresql.org/docs/13/datatype.html
// https://www.postgresql.org/docs/13/libpq-exec.html
// include/server/catalog/pg_type_d.h

/**
 * \ingroup SQL
 * PostgeSql Types
 */
enum class PgsqlType
{
   boolean        = 16L,
   bytea          = 17L,
   character      = 18L,
   name           = 19L,
   int8           = 20L,
   int2           = 21L,
   int4           = 23L,
   text           = 25L,
   oid            = 26L,
   tid            = 27L,
   xid            = 28L,
   cid            = 29L,
   float4         = 700L,
   float8         = 701L,
   money          = 790L,
   char_array     = 1002L,
   bpchar_array   = 1014L,
   varchar_array  = 1015L,
   bpchar         = 1042L,
   varchar        = 1043L,
   date           = 1082L,
   time           = 1083L,
   timestamp      = 1114L,
   timestamptz    = 1184L,
   interval       = 1186L,
   timetz         = 1266L,
   bit            = 1560L,
   varbit         = 1562L,
   numeric        = 1700L,
   serial         = -42L,
   serial8        = -43L,
   xml            = 142L,
   json           = 114L
};

} // namespace sql
} // namepsace tbs