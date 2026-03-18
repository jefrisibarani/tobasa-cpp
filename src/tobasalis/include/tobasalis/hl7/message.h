#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <regex>

#include "tobasalis/lis/message.h"
#include "tobasalis/hl7/segment.h"
#include "tobasalis/hl7/field.h"

namespace tbs {
namespace hl7 {

// Note: 
// based on: HL7-dotnetcore https://github.com/Efferent-Health/HL7-dotnetcore

/** \ingroup LIS
   \brief HL7 Message Hierarchy

MSH : Message Header
{
   PID : Patient demographic information
   [PV1] : Patient visit information
   {
      OBR : Sample information
      {[OBX]} : Analysis data
   }
}

Note:
[] encloses optional segments.
{} encloses segments which can repeat once or more.

*/

using byte = unsigned char;

class Message
   : public lis::Message
{
private:
   std::vector<std::string> _allSegments;
   std::unordered_map<std::string, SegementCollection> _segmentList;

   const std::regex segmentRegex { R"(^([A-Z][A-Z][A-Z1-9])([\(\[]([0-9]+)[\)\]]){0,1}$)" };
   const std::regex fieldRegex   { R"(^([0-9]+)([\(\[]([0-9]+)[\)\]]){0,1}$)" };
   const std::regex otherRegEx   { R"(^[1-9]([0-9]{1,2})?$)" };

   std::string    _hl7Message;
   std::string    _version;
   std::string    _messageStructure;
   std::string    _messageControlID;
   std::string    _processingID;
   int            _segmentCount = 0;
   HL7EncodingPtr _encoding;

public:
   std::string     toString();
   std::string     hl7Message() const   { return _hl7Message; }
   std::string     version()            { return _version; }
   std::string     messageStructure()   { return _messageStructure; }
   std::string     messageControlID()   { return _messageControlID; }
   std::string     processingID()       { return _processingID; }
   int             segmentCount()       { return _segmentCount; }
   HL7EncodingPtr& encoding()           { return _encoding; }

   void encoding(const HL7EncodingPtr& encoding) { _encoding = encoding; }

   virtual ~Message() = default;
   Message();
   Message(const std::string& strMessage);

   bool equals(const Message& obj);
   bool equals(const std::string& messsage);
   int getHashCode();
   bool parseMessage(bool bypassValidation = false);
   std::string serializeMessage(bool validate);

   /// <summary>
   /// Get the Value of specific Field/Component/SubCpomponent, throws error if field/component index is not valid
   /// </summary>
   /// <param name="strValueFormat">Field/Component position in format SEGMENTNAME.FieldIndex.ComponentIndex.SubComponentIndex example PID.5.2</param>
   /// <returns>Value of specified field/component/subcomponent</returns>
   std::string getValue(const std::string& strValueFormat);

   /// <summary>
   /// Sets the Value of specific Field/Component/SubComponent in matching Segments, throws error if field/component index is not valid
   /// </summary>
   /// <param name="strValueFormat">Field/Component position in format SEGMENTNAME.FieldIndex.ComponentIndex.SubComponentIndex example PID.5.2</param>
   /// <param name="strValue">Value for the specified field/component</param>
   /// <returns>boolean</returns>
   bool setValue(const std::string& strValueFormat, const std::string& strValue);

   /// <summary>
   /// Checks if specified field has components
   /// </summary>
   /// <param name="strValueFormat">Field/Component position in format SEGMENTNAME.FieldIndex.ComponentIndex.SubComponentIndex example PID.5.2</param>
   /// <returns>boolean</returns>
   bool isComponentized(const std::string& strValueFormat);

   /// <summary>
   /// Checks if specified fields has repetitions
   /// </summary>
   /// <param name="strValueFormat">Field/Component position in format SEGMENTNAME.FieldIndex.ComponentIndex.SubComponentIndex example PID.5.2</param>
   /// <returns>boolean</returns>
   bool hasRepetitions(const std::string& strValueFormat);

   /// <summary>
   /// Checks if specified component has sub components
   /// </summary>
   /// <param name="strValueFormat">Field/Component position in format SEGMENTNAME.FieldIndex.ComponentIndex.SubComponentIndex example PID.5.2</param>
   /// <returns>boolean</returns>
   bool isSubComponentized(const std::string& strValueFormat);

   /// <summary>
   /// Builds the acknowledgement message for this message
   /// </summary>
   /// <param name="bypassValidation">Bypasses validation of the resulting ACK message</param>
   /// <returns>An ACK message if success, otherwise null</returns>
   Message getACK(bool bypassValidation = false);

   /// <summary>
   /// Builds a negative ack for this message
   /// </summary>
   /// <param name="code">ack code like AR, AE</param>
   /// <param name="errMsg">Error message to be sent with NACK</param>
   /// <param name="bypassValidation">Bypasses validation of the resulting NACK message</param>
   /// <returns>A NACK message if success, otherwise null</returns>
   Message getNACK(const std::string& code, const std::string& errMsg, bool bypassValidation = false);

   /// <summary>
   /// Adds a segment to the message
   /// </summary>
   /// <param name="newSegment">Segment to be appended to the end of the message</param>
   /// <returns>True if added successfully, otherwise false</returns>
   bool addNewSegment(SegmentPtr newSegment);

   /// <summary>
   /// Removes a segment from the message
   /// </summary>
   /// <param name="segmentName">Segment to be removed</param>
   /// <param name="index">Zero-based index of the segment to be removed, in case of multiple. Default is 0.</param>
   /// <returns>True if found and removed successfully, otherwise false</returns>
   bool removeSegment(const std::string& segmentName, int index = 0);

   SegementCollection segments();

   SegementCollection segments(const std::string& segmentName);

   SegmentPtr defaultSegment(const std::string& segmentName);

   /// <summary>
   /// Addsthe header segment to a new message
   /// </summary>
   /// <param name="sendingApplication">Sending application name</param>
   /// <param name="sendingFacility">Sending facility name</param>
   /// <param name="receivingApplication">Receiving application name</param>
   /// <param name="receivingFacility">Receiving facility name</param>
   /// <param name="security">Security features. Can be null.</param>
   /// <param name="messageType">Message type ^ trigger event</param>
   /// <param name="messageControlID">Message control unique ID</param>
   /// <param name="processingID">Processing ID ^ processing mode</param>
   /// <param name="version">HL7 message version (2.x)</param>
   void addSegmentMSH(
      const std::string& sendingApplication,
      const std::string& sendingFacility,
      const std::string& receivingApplication,
      const std::string& receivingFacility,
      const std::string& security,
      const std::string& messageType,
      const std::string& messageControlID,
      const std::string& processingID,
      const std::string& version);

   /// <summary>
   /// Serialize to MLLP escaped byte array
   /// </summary>
   /// <param name="validate">Optional. Validate the message before serializing</param>
   /// <returns>MLLP escaped byte array</returns>
   byte* getMLLP(bool validate = false);

private:

   /// <summary>
   /// Builds an ACK or NACK message for this message
   /// </summary>
   /// <param name="code">ack code like AA, AR, AE</param>
   /// <param name="isNack">true for generating a NACK message, otherwise false</param>
   /// <param name="errMsg">error message to be sent with NACK</param>
   /// <param name="bypassValidation">Bypasses validation of the resulting ACK/NACK message</param>
   /// <returns>An ACK or NACK message if success, otherwise null</returns>
   Message createAckMessage(const std::string& code, bool isNack, const std::string& errMsg, bool bypassValidation);

   /// <summary>
   /// Gets a field object within a segment by index
   /// </summary>
   /// <param name="segment">The segment object to search in/param>
   /// <param name="index">The index of the field within the segment/param>
   /// <returns>A Field object</returns>
   FieldPtr getField(const SegmentPtr& segment, const std::string& index);

   /// <summary>
   /// Determines if a segment field has repetitions
   /// </summary>
   /// <param name="segment">The segment object to search in/param>
   /// <param name="index">The index of the field within the segment/param>
   /// <returns>A boolean indicating whether the field has repetitions</returns>
   int getFieldRepetitions(const SegmentPtr& segment, const std::string& index);

   /// <summary>
   /// Validates the HL7 message for basic syntax
   /// </summary>
   /// <returns>A boolean indicating whether the whole message is valid or not</returns>
   bool validateMessage();

   /// <summary>
   /// Serializes a field into a string with proper encoding
   /// </summary>
   /// <returns>A serialized string</returns>
   void serializeField(const FieldPtr& field, std::string& result);

   /// <summary>
   /// Get all segments in order as they appear in original message. This the usual order: IN1|1 IN2|1 IN1|2 IN2|2
   /// </summary>
   /// <returns>A list of segments in the proper order</returns>
   SegementCollection getAllSegmentsInOrder();

   /// <summary>
   /// Validates the components of a value's position descriptor
   /// </summary>
   /// <returns>A boolean indicating whether all the components are valid or not</returns>
   bool validateValueFormat(const std::vector<std::string>& allComponents);
};

using MessagePtr = std::shared_ptr<hl7::Message>;

} // namespace hl7
} // namespace tbs