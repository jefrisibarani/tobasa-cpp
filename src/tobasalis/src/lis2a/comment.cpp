#include <string>
#include "tobasalis/lis2a/comment.h"

namespace tbs {
namespace lis2a {

CommentRecord::CommentRecord(const std::string& lisString)
   : Record(lisString)
{
   setRecordType(RecordType::Comment);
   _totalField = 5;
   initFields();
}

void CommentRecord::initFields()
{
   _lisFields.push_back(Record::createField(1, "RecordTypeID", "C"));
   _lisFields.push_back(Record::createField(2, "SequenceNumber", "1"));
   _lisFields.push_back(Record::createField(3, "Source"));
   _lisFields.push_back(Record::createField(4, "Text"));
   _lisFields.push_back(Record::createField(5, "Type"));
}

void CommentRecord::setRecordTypeID(const char id)
{
   std::string x(1, id);
   setFieldValue(1, x);
}

} // namespace lis2a
} // namespace tbs