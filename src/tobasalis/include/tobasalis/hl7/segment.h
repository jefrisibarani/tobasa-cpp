#pragma once

#include <string>
#include "tobasalis/hl7/field.h"

namespace tbs {
namespace hl7 {

class Segment : public MessageElement
{
private:
   FieldCollection _fieldList;
   int _sequenceNo;
   std::string _name;

protected:
   virtual void processValue();

public:
   ~Segment();
   Segment() = default;
   Segment(HL7EncodingPtr encoding);
   Segment(const std::string& name, HL7EncodingPtr encoding);

   std::string name() const { return _name; }
   void name(const std::string& name) { _name = name; }

   Segment deepCopy();

   void addEmptyField();

   void addNewField(const std::string& content, int position = -1);

   void addNewField(const std::string& content, bool isDelimiters);

   bool addNewField(const FieldPtr& field, int position = -1);

   FieldPtr fields(int position);

   FieldCollection& fieldList() { return _fieldList; }
   FieldCollection& getAllFields();

   void sequenceNo(int no) { _sequenceNo = no; }
   int sequenceNo() const;
};

using SegmentPtr = std::shared_ptr<Segment>;
using SegementCollection = std::vector<SegmentPtr>;

} // namespace hl7
} // namespace tbs
