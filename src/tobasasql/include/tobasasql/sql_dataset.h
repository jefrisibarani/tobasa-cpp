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
class DataSet
{
public:   
   using VariantType   = VariantTypeImplemented;
   using VectorVariant = std::vector<VariantType>;
   using RecordVariant = std::vector<VectorVariant>;

   VectorVariant& addRow()
   {
      VectorVariant row;
      _data.emplace_back( std::move(row) );
      size_t count = _data.size();
      return _data.at(count-1);
   }

   VectorVariant& addRow(int totalColumns)
   {
      VectorVariant row;
      for (int i=0;i<totalColumns;i++)
      {
         row.emplace_back( std::move( VariantType()) );
      }

      _data.emplace_back(std::move(row));
      size_t count = _data.size();
      return _data[count-1];
   }

   RecordVariant& data() { return _data; }

   size_t empty() const { return _data.empty(); }
   long totalRows() const { return static_cast<long>(_data.size()); }
   int totalColumns() const { return _nCols;}
   void totalColumns(long val) { _nCols = val;}

private:
   RecordVariant _data;
   int _nCols  = 0;
};

/** @}*/

} // namespace sql
} // namespace tbs