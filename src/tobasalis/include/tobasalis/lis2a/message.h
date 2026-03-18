#pragma once

#include "tobasalis/lis/message.h"
#include "tobasalis/lis2a/record.h"
#include "tobasalis/lis2a/header.h"
#include "tobasalis/lis2a/requestinfo.h"

namespace tbs {
namespace lis2a {

/** \ingroup LIS
   \brief CLSI LIS02-A2 Message Hierarchy

         +------------------------+
    +--- | Level 0: Header Record |
    |    +------------------------+
    |       |
    |       |   +----------------------------+
    |       +---| Level 1: Patient Record(1) |
    |       |   +----------------------------+
    |       |      |
    |       |      |   +----------------------------+
    |       |      +---|  Level 2: Order Record(1)  |
    |       |          +----------------------------+
    |       |              |
    |       |              |   +----------------------------+
    |       |              +---|  Level 3: Result Record(1) |
    |       |                  +----------------------------+
    |       |
    |       |   +----------------------------+
    |       +---| Level 1: Patient Record(2) |
    |           +----------------------------+
    |              |
    |              |   +----------------------------+
    |              +---|  Level 2: Order Record(1)  |
    |              |   +----------------------------+
    |              |       |
    |              |       |   +----------------------------+
    |              |       +---| Level 3: Comment Record(1) |
    |              |       |   +----------------------------+
    |              |       |
    |              |       |   +----------------------------+
    |              |       +---| Level 3: Result Record(1)  |
    |              |           +----------------------------+
    |              |
    |              |   +----------------------------+
    |              +---|  Level 2: Order Record(2)  |
    |                  +----------------------------+
    |                      |
    |                      |   +----------------------------+
    |                      +---| Level 3: Result Record(1)  |
    |                      |   +----------------------------+
    |                      |
    |                      |   +----------------------------+
    |                      +---| Level 3: Result Record(2)  |
    |                          +----------------------------+
    |
    |    +----------------------------+
    +----| Level 0: Terminator Record |
         +----------------------------+


   Analyzer sends query message to the LIS

         +------------------------+
    +--- | Level 0: Header Record |
    |    +------------------------+
    |       |
    |       |   +-------------------------------------------+
    |       +---| Level 1: Request Info Record(query order) |
    |           +-------------------------------------------+
    |
    |    +----------------------------+
    +----| Level 0: Terminator Record |
         +----------------------------+

*/

class PatientRecord;

/** \ingroup LIS
 * Message
 */
class Message
   : public lis::Message
{
public:
   Message();
   virtual ~Message() = default;

   void addPatient(const lis::RecordPtr& patient);
   void addRequestInfo(const lis::RecordPtr& query);
   std::shared_ptr<PatientRecord> getLastPatient();
   virtual std::string toString();
   
   std::shared_ptr<HeaderRecord> getHeaderRecord();
   std::shared_ptr<RequestInfoRecord> getRequestInfo();
   bool hasRequest() { return _hasRequest; }
   void hasRequest(bool value) { _hasRequest = value; }

   bool isAckMessage();
   bool isOrderRequest();

protected:
   bool _hasRequest = false;   
};

using MessagePtr = std::shared_ptr<lis2a::Message>;

} // namespace lis2a
} // namespace tbs