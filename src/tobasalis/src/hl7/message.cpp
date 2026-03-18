#include <regex>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <tobasa/datetime.h>
#include <tobasa/util_string.h>
#include "tobasalis/lis/common.h"
#include "tobasalis/hl7/exception.h"
#include "tobasalis/hl7/message.h"

namespace tbs {
namespace hl7 {
namespace internal {

static int hashCode = 1;
int getHashCode()
{
   return hashCode++;
}

std::vector<unsigned char> getMLLP(const std::string& message, const std::string& encoding = "UTF-8")
{
   // Convert encoding string to actual encoding if needed
   // For simplicity, assuming UTF-8 encoding in this example

   std::vector<unsigned char> data(message.begin(), message.end());
   std::vector<unsigned char> buffer(data.size() + 3);

   buffer[0] = 0x0B;                // 11 VT (Vertical Tab)
   std::copy(data.begin(), data.end(), buffer.begin() + 1);

   buffer[buffer.size() - 2] = 0x1C; // 28 FS (File Separator)
   buffer[buffer.size() - 1] = 0x0D; // 13 CR (Carriage Return)

   return buffer;
}

std::string currentDateTimeStr(bool useSecondFraction=true)
{
   std::string dstr = DateTime::now().format("{:%Y%m%d%H%M%S}", useSecondFraction);
   
   {
      // Get the current time
      auto now = std::chrono::system_clock::now();
      auto now_time = std::chrono::system_clock::to_time_t(now);
      // Convert to tm struct to extract various components
      std::tm* time_info = std::localtime(&now_time);
      // Format the date and time
      std::stringstream formattedTime;
      formattedTime << std::put_time(time_info, "%Y%m%d%H%M%S.") << std::setw(4) << std::setfill('0') << (now.time_since_epoch() % std::chrono::seconds(1)) / std::chrono::milliseconds(1);
      
      std::string dstr2 = formattedTime.str();
   }

   return dstr;
}

} // namespace internal

Message::Message()
{
   _vendorProtocolId = lis::MSG_HL7;
   _encoding = std::make_shared<HL7Encoding>();
}

Message::Message(const std::string& strMessage)
{
   _vendorProtocolId = lis::MSG_HL7;
   _encoding = std::make_shared<HL7Encoding>();
   _hl7Message = strMessage;
}

std::string Message::toString()
{
   std::string msg = serializeMessage(true);
   return msg;
}

bool Message::equals(const Message& obj)
{
   return equals( obj.hl7Message() );
}

bool Message::equals(const std::string& messsage)
{
   bool keepEmptyEntry = false;
   auto arr1 = util::split(_hl7Message, _encoding->segmentDelimiter(), keepEmptyEntry);
   auto arr2 = util::split(messsage,    _encoding->segmentDelimiter(), keepEmptyEntry);
   return arr1 == arr2;
}

int Message::getHashCode()
{
   return internal::getHashCode();
}

bool Message::parseMessage(bool bypassValidation)
{
   bool isValid   = false;
   bool isParsed  = false;
   bool validated = false;

   try
   {
      if (!bypassValidation)
      {
         isValid = validateMessage();
         validated = isValid;
      }
      else
         isValid = true;
   }
   catch (const HL7Exception& ex)
   {
      throw ex;
   }
   catch (const std::exception& ex)
   {
      throw HL7Exception(std::string("Unhandled Exception in validation - ") + ex.what(), hl7::ERR_BAD_MESSAGE);
   }

   if (isValid)
   {
      try
      {
         if (_allSegments.size() <= 0)
         {
            if (!validated) {
               _encoding->evaluateSegmentDelimiter(_hl7Message);
            }
            
            _allSegments = util::split(_hl7Message, _encoding->segmentDelimiter(), false);
         }

         int segSeqNo = 0;

         for (std::string strSegment: _allSegments)
         {
            if (strSegment.empty() )
                  continue;

            auto newSegment = std::make_shared<Segment>(_encoding);
            newSegment->name(strSegment.substr(0, 3));
            newSegment->value(strSegment);
            newSegment->sequenceNo(segSeqNo++);

            addNewSegment(newSegment);
         }

         _segmentCount = segSeqNo;

         std::string strSerializedMessage;

         try
         {
            strSerializedMessage = serializeMessage(false);
         }
         catch (const std::exception& ex)
         {
            throw HL7Exception(std::string("Failed to serialize parsed message with error - ") + ex.what(), hl7::ERR_PARSING_ERROR);
         }

         if (! strSerializedMessage.empty() )
         {
            if (equals(strSerializedMessage))
               isParsed = true;
         }
         else
         {
            throw HL7Exception(std::string("Unable to serialize to original message"), hl7::ERR_PARSING_ERROR);
         }
      }
      catch (const std::exception& ex)
      {
         throw HL7Exception(std::string("Failed to parse the message with error - ") + ex.what(), hl7::ERR_PARSING_ERROR);
      }
   }

   return isParsed;
}

std::string Message::serializeMessage(bool validate)
{
   if (validate && !validateMessage()) {
      throw HL7Exception("Failed to validate the updated message", hl7::ERR_BAD_MESSAGE);
   }

   std::string strMessage;
   std::string currentSegName;
   auto segListOrdered = getAllSegmentsInOrder();

   try
   {
      for(auto seg: segListOrdered)
      {
         currentSegName = seg->name();
         strMessage.append(seg->name());

         if (seg->fieldList().size() > 0)
            strMessage.append(1, _encoding->fieldDelimiter());

         int startField = currentSegName == "MSH" ? 1 : 0;

         for (int i = startField; i < seg->fieldList().size(); i++)
         {
            if (i > startField) {
               strMessage.append(1, _encoding->fieldDelimiter());
            }

            auto field = seg->fieldList()[i];

            if (field->isDelimitersField())
            {
               strMessage.append(field->undecodedValue());
               continue;
            }

            if (field->hasRepetitions())
            {
               for (int j = 0; j < field->repetitions().size(); j++)
               {
                  if (j > 0) {
                     strMessage.append(1, _encoding->repeatDelimiter());
                  }

                  serializeField(field->repetitions()[j], strMessage);
               }
            }
            else
               serializeField(field, strMessage);
         }

         strMessage.append(_encoding->segmentDelimiter());
      }
   }
   catch (const std::exception& ex)
   {
      if (currentSegName == "MSH")
         throw HL7Exception(std::string("Failed to serialize the MSH segment with error - ") + ex.what(), hl7::ERR_SERIALIZATION_ERROR);
      else
         throw HL7Exception(std::string("Failed to serialize the message with error - ") + ex.what(), hl7::ERR_SERIALIZATION_ERROR);
   }


   return strMessage;
}

std::string Message::getValue(const std::string& strValueFormat)
{
   /*
   Example: strValueFormat is NK1(2).2
   Search for the second NIK, then get the field at index 2
   With below sample message, we get DOE^JHON^^^^

   MSH|^~\&|AcmeHIS|StJohn|CATH|StJohn|20061019172719||ADT^O01|MSGID12349876|P|2.3
   PID||0493575^^^2^ID 1|454721||DOE^JOHN^^^^|DOE^JOHN^^^^|19480203|M||B|254 MYSTREET AVE^^MYTOWN^OH^44123^USA||(216)123-4567|||M|NON|400003403~1129086|
   NK1||ROE^MARIE^^^^|SPO||(216)123-4567||EC|||||||||||||||||||||||||||
   NK1||DOE^JHON^^^^|FTH||(216)123-4567||EC|||||||||||||||||||||||||||
   PV1||O|OP^PAREG^^^^^^^^||||277^MYLASTNAME^BONNIE^^^^|||||||||| ||2688684|||||||||||||||||||||||||199912271408||||||002376853
   */

   std::string segmentName;
   int segmentOccurrence = 0;
   int componentIndex    = 0;
   int subComponentIndex = 0;
   std::string strValue;
   
   /*
      with strValueFormat is NK1(2).2 
      allComponents values are:
      [0] NK1(2)
      [1] 2
   */
   std::vector<std::string> allComponents = util::split(strValueFormat, '.');

   int comCount = allComponents.size();
   bool isValid = validateValueFormat(allComponents);

   if (isValid)
   {
      /*
         Note: Use std::regex_search
         with strValueFormat is NK1(2).2
         matches values are:
         [0]: NK1(2)
         [1]: NK1
         [2]: (2)
         [3]: 2
      */
      std::smatch matches;
      if (!std::regex_search(allComponents[0], matches, segmentRegex)) {
         // matches.size() is 0
         throw HL7Exception("Request format is not valid: " + strValueFormat);
      }
      // with strValueFormat is NK1(2).2 segmentName is NK1
      segmentName = matches[1].str();

      // with strValueFormat is NK1(2).2  matches[0]  value is NK1(2)
      if (matches[0].str().length() > 3)
      {
         auto segOcc = matches[3].str();
         if (util::isNumber(segOcc))
         {
            // get segment occurrence
            segmentOccurrence = std::stoi(segOcc);
            // we use zero based index
            segmentOccurrence--;
         }
         else 
            throw HL7Exception("Segment occurrence invalid: " + strValueFormat);
      }

      if (_segmentList.find(segmentName) != _segmentList.end())
      {
         auto segment = _segmentList[segmentName][segmentOccurrence];

         if (comCount == 4)
         {
            try
            {
               componentIndex    = std::stoi(allComponents[2]);
               subComponentIndex = std::stoi(allComponents[3]);

               auto field = getField(segment, allComponents[1]);
               strValue = field->components()[componentIndex - 1]->subComponents()[subComponentIndex - 1]->value();
            }
            catch (const std::exception& ex)
            {
               throw HL7Exception("SubComponent not available - " + strValueFormat + " Error: " + ex.what());
            }
         }
         else if (comCount == 3)
         {
            try
            {
               componentIndex = std::stoi(allComponents[2]);
               auto field = getField(segment, allComponents[1]);
               strValue = field->components()[componentIndex - 1]->value();
            }
            catch (const std::exception& ex)
            {
               throw HL7Exception("Component not available - " + strValueFormat + " Error: " + ex.what());
            }
         }
         else if (comCount == 2)
         {
            try
            {
               auto field = getField(segment, allComponents[1]);
               strValue = field->value();
            }
            catch (const std::exception& ex)
            {
               throw HL7Exception("Field not available - " + strValueFormat + " Error: " + ex.what());
            }
         }
         else
         {
            try
            {
               strValue = segment->value();
            }
            catch (const std::exception& ex)
            {
               throw HL7Exception("Segment value not available - " + strValueFormat + " Error: " + ex.what());
            }
         }
      }
      else
      {
         throw HL7Exception("Segment name not available: " + strValueFormat);
      }
   }
   else
   {
      throw HL7Exception("Request format is not valid: " + strValueFormat);
   }

   return _encoding->decode(strValue);
}

bool Message::setValue(const std::string& strValueFormat, const std::string& strValue)
{
   bool isSet = false;

   std::string segmentName;
   int componentIndex = 0;
   int subComponentIndex = 0;
   auto allComponents = util::split(strValueFormat, '.');
   int comCount = allComponents.size();
   bool isValid = validateValueFormat(allComponents);

   if (isValid)
   {
      segmentName = allComponents[0];

      if (_segmentList.find(segmentName) != _segmentList.end())
      {
         for (auto segment: _segmentList[segmentName])
         {
            if (comCount == 4)
            {
               componentIndex = std::stoi(allComponents[2]);
               subComponentIndex = std::stoi(allComponents[3]);

               try
               {
                  auto field = getField(segment, allComponents[1]);
                  field->components()[componentIndex - 1]->subComponents()[subComponentIndex - 1]->value(strValue);
                  isSet = true;
               }
               catch (const std::exception& ex)
               {
                  throw HL7Exception("SubComponent not available - " + strValueFormat + " Error: " + ex.what());
               }
            }
            else if (comCount == 3)
            {
               componentIndex = std::stoi(allComponents[2]);

               try
               {
                  auto field = getField(segment, allComponents[1]);
                  field->components()[componentIndex - 1]->value(strValue);
                  isSet = true;
               }
               catch (const std::exception& ex)
               {
                  throw HL7Exception("Component not available - " + strValueFormat + " Error: " + ex.what());
               }
            }
            else if (comCount == 2)
            {
               try
               {
                  auto field = getField(segment, allComponents[1]);
                  field->value(strValue);
                  isSet = true;
               }
               catch (const std::exception& ex)
               {
                  throw HL7Exception("Field not available - " + strValueFormat + " Error: " + ex.what());
               }
            }
            else
            {
               throw HL7Exception("Cannot overwrite a segment value");
            }
         }
      }
      else
         throw HL7Exception("Segment name not available");
   }
   else
      throw HL7Exception("Request format is not valid");
   
   return isSet;
}

bool Message::isComponentized(const std::string& strValueFormat)
{
   bool isComponentized = false;

   std::string segmentName;
   auto allComponents = util::split(strValueFormat, '.');
   int comCount = static_cast<int>(allComponents.size());
   bool isValid = validateValueFormat(allComponents);

   if (isValid)
   {
      segmentName = allComponents[0];

      if (comCount >= 2)
      {
         try
         {
            auto segment = _segmentList[segmentName].at(0);
            auto field = getField(segment, allComponents[1]);

            isComponentized = field->isComponentized();
         }
         catch (const std::exception& ex)
         {
            throw HL7Exception("Field not available - " + strValueFormat + " Error: " + ex.what());
         }
      }
      else
         throw HL7Exception("Field not identified in request");
   }
   else
      throw HL7Exception("Request format is not valid");

   return isComponentized;
}

bool Message::hasRepetitions(const std::string& strValueFormat)
{
   std::string segmentName;
   auto allComponents = util::split(strValueFormat, '.');
   int comCount = allComponents.size();
   bool isValid = validateValueFormat(allComponents);

   if (isValid)
   {
      segmentName = allComponents[0];
      auto segment = _segmentList[segmentName].at(0);

      if (comCount >= 2)
      {
         try
         {
            auto count = getFieldRepetitions(segment, allComponents[1]);
            return count > 1;
         }
         catch (const std::exception& ex)
         {
            throw HL7Exception("Field not available - " + strValueFormat + " Error: " + ex.what());
         }
      }
      else
         throw HL7Exception("Field not identified in request");
   }
   else
      throw HL7Exception("Request format is not valid");
}

bool Message::isSubComponentized(const std::string& strValueFormat)
{
   bool isSubComponentized = false;
   std::string segmentName;
   int componentIndex = 0;
   std::vector<std::string> allComponents = util::split(strValueFormat, '.');
   int comCount = allComponents.size();
   bool isValid = validateValueFormat(allComponents);

   if (isValid)
   {
      segmentName = allComponents[0];

      if (comCount >= 3)
      {
         try
         {
            auto segment = _segmentList[segmentName].at(0);
            auto field = getField(segment, allComponents[1]);

            int componentIndex =  std::stoi(allComponents[2]);
            isSubComponentized = field->components()[componentIndex - 1]->isSubComponentized();
         }
         catch (const std::exception& ex)
         {
            throw HL7Exception(std::string("Component not available - ") + strValueFormat + " Error: " + ex.what());
         }
      }
      else
         throw HL7Exception("Component not identified in request");
   }
   else
      throw HL7Exception("Request format is not valid");

   return isSubComponentized;
}

Message Message::getACK(bool bypassValidation)
{
   return createAckMessage("AA", false, "", bypassValidation);
}

Message Message::getNACK(const std::string& code, const std::string& errMsg, bool bypassValidation)
{
   return createAckMessage(code, true, errMsg, bypassValidation);
}

bool Message::addNewSegment(SegmentPtr newSegment)
{
   try
   {
      newSegment->sequenceNo(_segmentCount++);
      if (_segmentList.find(newSegment->name()) == _segmentList.end())
      {
         _segmentList[newSegment->name()] = SegementCollection();
      }

      _segmentList[newSegment->name()].push_back(newSegment);

      return true;
   }
   catch (const std::exception& ex)
   {
      _segmentCount--;
      throw HL7Exception(std::string("Unable to add new segment. Error - ") + ex.what());
   }
}

bool Message::removeSegment(const std::string& segmentName, int index)
{
   try
   {
      if (_segmentList.find(segmentName) == _segmentList.end()) {
         return false;
      }

      auto& list = _segmentList[segmentName];

      if (list.size() <= static_cast<size_t>(index)) {
         return false;
      }

      list.erase(list.begin() + index);
      _segmentCount--;
      return true;
   }
   catch (const std::exception& ex)
   {
      throw HL7Exception(std::string("Unable to add remove segment. Error - ") + ex.what());
   }
}

SegementCollection Message::segments()
{
   return getAllSegmentsInOrder();
}

SegementCollection Message::segments(const std::string& segmentName)
{
   SegementCollection coll;
   for (auto seg : getAllSegmentsInOrder())
   {
      if (seg->name() == segmentName)
      {
         coll.push_back(seg);
      }
   }
   return coll;
}

SegmentPtr Message::defaultSegment(const std::string& segmentName)
{
   for (auto seg : getAllSegmentsInOrder())
   {
      if (seg->name() == segmentName)
         return seg;
   }

   return {};
}

void Message::addSegmentMSH(
   const std::string& sendingApplication,
   const std::string& sendingFacility,
   const std::string& receivingApplication,
   const std::string& receivingFacility,
   const std::string& security,
   const std::string& messageType,
   const std::string& messageControlID,
   const std::string& processingID,
   const std::string& version)
{
   std::string dateString = internal::currentDateTimeStr();
   //if (_instrumentType == lis::NONSTD_HL7) {
   //   dateString = internal::currentDateTimeStr(false);
   //}

   auto delim  = _encoding->fieldDelimiter();

   std::string response = "MSH";
   response.append(_encoding->allDelimiters() + delim);
   response.append(sendingApplication + delim);
   response.append(sendingFacility + delim);
   response.append(receivingApplication + delim);
   response.append(receivingFacility + delim);
   response.append(_encoding->encode(dateString) + delim);
   response.append(security + delim);
   response.append(messageType + delim);
   response.append(messageControlID + delim);
   response.append(processingID + delim);
   response.append(version + _encoding->segmentDelimiter());

   Message message(response);
   message.instrumentType(this->instrumentType());
   message.encoding()->instrumentType(this->instrumentType());
   //if (this->instrumentType() == lis::NONSTD_HL7)
   //{
      // Use FS as segement delimiter
      //message.encoding()->segmentDelimiter(std::string(1,'\x1C'));
   //}


   message.parseMessage();
   addNewSegment(message.defaultSegment("MSH"));
}

byte* Message::getMLLP(bool validate)
{
   std::string hl7 = serializeMessage(validate);

   auto rawbytes = internal::getMLLP(hl7);
   return rawbytes.data();
}

Message Message::createAckMessage(const std::string& code, bool isNack, const std::string& errMsg, bool bypassValidation)
{
   std::string response;

   if (_messageStructure != "ACK")
   {
      std::string dateString;
      
      //if (_instrumentType == lis::NONSTD_HL7)
      //   dateString = internal::currentDateTimeStr(false);
      //else
         dateString = internal::currentDateTimeStr();

      auto fieldDelimiter = _encoding->fieldDelimiter();
      auto coll = _segmentList["MSH"];
      auto msh  = coll.at(0);

      response.append("MSH");
      response.append(_encoding->allDelimiters());
      response.append(1, fieldDelimiter);
      response.append(msh->fieldList()[4]->value());
      response.append(1, fieldDelimiter);
      response.append(msh->fieldList()[5]->value());
      response.append(1, fieldDelimiter);
      response.append(msh->fieldList()[2]->value());
      response.append(1, fieldDelimiter);
      response.append(msh->fieldList()[3]->value());
      response.append(1, fieldDelimiter);
      response.append(dateString);
      response.append(1, fieldDelimiter);
      response.append(1, fieldDelimiter);
      
      //if (_instrumentType == lis::NONSTD_HL7) {
      //   response.append("ACK^R01");
      //}
      //else {
         response.append("ACK");
      //}
      
      response.append(1, fieldDelimiter);
      response.append(_messageControlID);
      response.append(1, fieldDelimiter);
      response.append(_processingID);
      response.append(1, fieldDelimiter);
      response.append(_version);

      // if (_instrumentType == lis::NONSTD_HL7) 
      // {
      //    // Still works without this
      //    response.append("||||||Unicode|||");
      // }
      // else if( _instrumentType == lis::NONSTD_HL7A || _instrumentType == lis::NONSTD_HL7B )
      // {
      //    response.append("||||||UNICODE");
      // }

      response.append(_encoding->segmentDelimiter());

      response.append("MSA");
      response.append(1, fieldDelimiter);
      response.append(code);
      response.append(1, fieldDelimiter);
      response.append(_messageControlID);
      response.append((isNack ? fieldDelimiter + errMsg : "" ));
      response.append(_encoding->segmentDelimiter());
   }
   else {
      return {};
   }

   Message message(response);
   message.instrumentType(this->instrumentType());
   message.encoding()->instrumentType(this->instrumentType());
   // if (this->instrumentType() == lis::NONSTD_HL7)
   // {
   //    // Uses FS as segment delimiter
   //    message.encoding()->segmentDelimiter(std::string(1,'\x1C'));
   // }

   message.parseMessage(bypassValidation);
   return message;
}


FieldPtr Message::getField(const SegmentPtr& segment, const std::string& index)
{
   int repetition = 0;
   std::smatch matches;

   if (!std::regex_search(index, matches, fieldRegex))
      throw std::runtime_error("Invalid field index");

   if (matches.size() < 1)
      throw std::runtime_error("Invalid field index");

   int fieldIndex = 0;
   fieldIndex = std::stoi(matches[1].str());
   fieldIndex--;

   auto matchStr = matches[1].str();
   if (matchStr.length() > 3)
   {
      repetition = std::stoi(matches[1].str());
      repetition--;
   }

   auto field = segment->fieldList()[fieldIndex];

   if (field->hasRepetitions())
      return field->repetitions()[repetition];
   else if (repetition == 0)
      return field;
   else
      return {};
}


int Message::getFieldRepetitions(const SegmentPtr& segment, const std::string& index)
{
   std::smatch matches;

   if (!std::regex_search(index, matches, fieldRegex)) 
      return 0;

   if (matches.size() < 1)
      return 0;

   int fieldIndex;
   std::string matchedValue = matches[1].str();
   fieldIndex = std::stoi(matchedValue);
   fieldIndex--;

   auto seg = segment;
   if (fieldIndex >= 0 && fieldIndex < seg->fieldList().size())
   {
      auto field = seg->fieldList()[fieldIndex];
      if (field->hasRepetitions())
         return field->repetitions().size();
      else
         return 1;
   }

   return 0;
}


bool Message::validateMessage()
{
   if (_hl7Message.empty()) {
      throw HL7Exception("No Message Found", hl7::ERR_BAD_MESSAGE);
   }

   // Check message length - MSH+Delimeters+12Fields in MSH
   if (_hl7Message.length() < 20) {
      throw HL7Exception(std::string("Message Length too short: ") + std::to_string(_hl7Message.length()) + " chars.", hl7::ERR_BAD_MESSAGE);
   }

   // Check if message starts with header segment
   if ( ! util::startsWith(_hl7Message, "MSH")) {
      throw HL7Exception("MSH segment not found at the beginning of the message", hl7::ERR_BAD_MESSAGE);
   }

   try
   {
      _encoding->evaluateSegmentDelimiter(_hl7Message);

      {
         auto splitted = util::split(_hl7Message, _encoding->segmentDelimiter());
         std::string message_;
         std::stringstream subComStream{};
         std::string separator;
         for (auto x : splitted)
         {
            subComStream << separator << x;
            separator = _encoding->segmentDelimiter();
         }
         message_ = subComStream.str();
         message_.append(_encoding->segmentDelimiter());
         _hl7Message = message_;
      }


      // Check Segment Name & 4th character of each segment
      char fourthCharMSH = _hl7Message[3];
      _allSegments = util::split(_hl7Message, _encoding->segmentDelimiter(), false );

      for (auto strSegment: _allSegments)
      {
         if (strSegment.empty())
            continue;

         auto segmentName = strSegment.substr(0, 3);
         bool isValidSegmentName = std::regex_match(segmentName, segmentRegex);

         if (!isValidSegmentName)
         {
            throw HL7Exception(std::string("Invalid segment name found: ") + strSegment, hl7::ERR_BAD_MESSAGE);
         }

         if (strSegment.length() > 3 && fourthCharMSH != strSegment[3])
         {
            throw HL7Exception(std::string("Invalid segment found: ") + strSegment, hl7::ERR_BAD_MESSAGE);
         }
      }

      std::string _fieldDelimiters_Message = _allSegments[0].substr(3, 8 - 3);
      _encoding->evaluateDelimiters(_fieldDelimiters_Message);

      // Count field separators, MSH.12 is required so there should be at least 11 field separators in MSH
      auto segmentMSH = _allSegments[0];
      int countFieldSepInMSH = std::count(segmentMSH.begin(), segmentMSH.end(), _encoding->fieldDelimiter());

      if (countFieldSepInMSH < 11) {
         throw HL7Exception("MSH segment doesn't contain all the required fields", hl7::ERR_BAD_MESSAGE);
      }

      // Find Message Version
      auto MSHFields = util::split(_allSegments[0], _encoding->fieldDelimiter());

      if (MSHFields.size() >= 12)
      {
         auto res = util::split(_encoding->decode(MSHFields[11]), _encoding->componentDelimiter());
         _version = res[0];
      }
      else {
         throw HL7Exception("HL7 version not found in the MSH segment", hl7::ERR_REQUIRED_FIELD_MISSING);
      }

      // Find Message Type & Trigger Event
      try
      {
         std::string MSH_9 = _encoding->decode(MSHFields[8]);

         if (MSH_9.empty()) {
            throw HL7Exception("MSH.9 not available", hl7::ERR_UNSUPPORTED_MESSAGE_TYPE);
         }

         auto MSH_9_comps = util::split(MSH_9, _encoding->componentDelimiter());

         if (MSH_9_comps.size() >= 3) {
            _messageStructure = MSH_9_comps[2];
         }
         else if (MSH_9_comps.size() > 0 && MSH_9_comps[0]=="ACK") {
            _messageStructure = "ACK";
         }
         else if (MSH_9_comps.size() == 2) {
            _messageStructure = MSH_9_comps[0] + "_" + MSH_9_comps[1];
         }
         else {
            throw HL7Exception("Message Type & Trigger Event value not found in message", hl7::ERR_UNSUPPORTED_MESSAGE_TYPE);
         }
      }
      catch (const std::exception& ex)
      {
         throw HL7Exception(std::string("Can't find message structure (MSH.9.3) - ") + ex.what(), hl7::ERR_UNSUPPORTED_MESSAGE_TYPE);
      }

      try
      {
         _messageControlID = _encoding->decode(MSHFields[9]);

         if (_messageControlID.empty())
            throw HL7Exception("MSH.10 - Message Control ID not found", hl7::ERR_REQUIRED_FIELD_MISSING);
      }
      catch (const std::exception& ex)
      {
         throw HL7Exception(std::string("Error occured while accessing MSH.10 - ") + ex.what(), hl7::ERR_REQUIRED_FIELD_MISSING);
      }

      try
      {
         _processingID = _encoding->decode(MSHFields[10]);

         if (_processingID.empty())
            throw HL7Exception(std::string("MSH.11 - Processing ID not found"), hl7::ERR_REQUIRED_FIELD_MISSING);
      }
      catch (const std::exception& ex)
      {
         throw HL7Exception(std::string("Error occured while accessing MSH.11 - ") + ex.what(), hl7::ERR_REQUIRED_FIELD_MISSING);
      }
   }
   catch (const std::exception& ex)
   {
      throw HL7Exception(std::string("Failed to validate the message with error - ") + ex.what(), hl7::ERR_BAD_MESSAGE);
   }

   return true;
}


void Message::serializeField(const FieldPtr& field, std::string& result)
{
   if (field->components().size() > 0)
   {
      int indexCom = 0;

      for (auto com: field->components().vector())
      {
         indexCom++;
         if (com->subComponents().size() > 0)
         {
            std::string subComStr;
            // Concatenate sub component
            std::stringstream subComStream{};
            std::string separator= "";
            for (auto sub : com->subComponents())
            {
               subComStream << separator << _encoding->encode(sub->value());
               separator = std::string(1,_encoding->subComponentDelimiter());
            }
            subComStr = subComStream.str();

            result.append(subComStr);
         }
         else {
            result.append(_encoding->encode( com->value() ));
         }

         if (indexCom < field->components().size())
            result.append(1, _encoding->componentDelimiter());
      }
   }
   else
   {
      auto f = field;
      result.append(_encoding->encode(f->value()));
   }

}


SegementCollection Message::getAllSegmentsInOrder()
{
   SegementCollection list;

   for (const auto& pair : _segmentList)
   {
      auto name = pair.first;
      SegementCollection segs = _segmentList[name];
      for (SegmentPtr seg : segs)
      {
         list.push_back(seg);
      }
   }

   // Sort the list by id
   std::sort(list.begin(), list.end(),
      [](const SegmentPtr& a, const SegmentPtr& b)
      {
         auto a1 = a;
         auto b1 = b;
         return a1->sequenceNo() < b1->sequenceNo();
      }
   );

   return std::move(list);
}


bool Message::validateValueFormat(const std::vector<std::string>& allComponents)
{
   bool isValid = false;

   if (allComponents.size() > 0)
   {
      bool isMatch = std::regex_match(allComponents[0], segmentRegex);

      if (isMatch)
      {
         for (int i = 1; i < allComponents.size(); i++)
         {
            if (i == 1 && std::regex_match(allComponents[i], fieldRegex))
               isValid = true;
            else if (i > 1 && std::regex_match(allComponents[i], otherRegEx))
               isValid = true;
            else
               return false;
         }
      }
      else
      {
         isValid = false;
      }
   }

   return isValid;
}

} // namespace hl7
} // namespace tbs