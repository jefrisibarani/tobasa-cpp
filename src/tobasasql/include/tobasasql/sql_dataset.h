#pragma once

#include <tobasa/variant.h>

namespace tbs {
namespace sql {

/** \addtogroup SQL
 * @{
 */

/**
 * \brief DataSet.
 * Simple query result class
 */
template <typename VariantTypeImplemented>
struct DataSet
{
   using VariantType   = VariantTypeImplemented;
   using VectorVariant = std::vector<VariantType>;
   using RecordVariant = std::vector<VectorVariant>;

   VectorVariant& addRow()
   {
      VectorVariant row;
      data.emplace_back( std::move(row) );
      size_t count = data.size();
      return data.at(count-1);
   }

   VectorVariant& addRow(int totalColumns)
   {
      VectorVariant row;
      for (int i=0;i<totalColumns;i++)
      {
         row.emplace_back( std::move( VariantType()) );
      }

      data.emplace_back(std::move(row));
      size_t count = data.size();
      return data[count-1];   
   }

   long totalRows;
   int totalColumns;
   RecordVariant data;
};

/** @}*/

} // namespace sql
} // namespace tbs