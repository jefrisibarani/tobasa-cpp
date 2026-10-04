#include <gtest/gtest.h>

#include "tobasalis/lis2a/requestinfo.h"

TEST(TobasaLisTest, StoresRequestInfoFields)
{
   tbs::lis2a::RequestInfoRecord record;
   record.setRecordTypeID('Q');
   record.setSequenceNumber(7);
   record.setRequestInfoStatusCode(tbs::lis2a::QueryStatusCode::Final);

   EXPECT_EQ(record.recordTypeID(), "Q");
   EXPECT_EQ(record.sequenceNumber(), 7);
   EXPECT_EQ(record.requestInfoStatusCodeEnum(), tbs::lis2a::QueryStatusCode::Final);
}