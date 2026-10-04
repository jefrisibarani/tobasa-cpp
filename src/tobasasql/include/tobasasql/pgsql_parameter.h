#pragma once

#include <string>
#include <libpq-fe.h>

namespace tbs {
namespace sql {

/** 
 * \ingroup SQL
 * \brief PostgreSql ParameterContext.
 */

class PgsqlParameterContext
{
   friend class PgsqlConnection;
   friend class PgsqlCommand;

public:
   PgsqlParameterContext() = default;
   PgsqlParameterContext(int totalParam)
   {
      initialize(totalParam);
   }

   void initialize(int totalParam)
   {
      reset();
      total    = totalParam;
      types    = new Oid[totalParam]();
      values   = new char* [totalParam]();
      lengths  = new int[totalParam]();
      formats  = new int[totalParam]();
   }

   ~PgsqlParameterContext()
   {
      reset();
   }

   void reset()
   {
      total = 0;

      if (types) 
      {
         delete[] types;
         types = nullptr;
      }

      if (values)
      {
         delete[] values;
         values = nullptr;
      }

      if (lengths)
      {
         delete[] lengths;
         lengths = nullptr;
      }

      if (formats)
      {
         delete[] formats;
         formats = nullptr;
      }
   }

private:
   int    total    = 0;
   Oid*   types    = nullptr;
   char** values   = nullptr;
   int*   lengths  = nullptr;
   int*   formats  = nullptr;
};

} // namespace sql
} // namespace tbs