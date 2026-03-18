#include <vector>
#include <string>
#include <tobasa/util_string.h>
#include <tobasa/logger.h>
#include "tobasalis/bci/generalrecord_vitek2compact.h"
#include "tobasalis/bci/patientrecord_vitek2compact.h"
#include "tobasalis/bci/specimenrecord_vitek2compact.h"
#include "tobasalis/bci/culturerecord_vitek2compact.h"
#include "tobasalis/bci/testrecord_vitek2compact.h"
#include "tobasalis/bci/suscepresultrecord_vitek2compact.h"
#include "tobasalis/bci/afrecord_vitek2compact.h"
#include "tobasalis/bci/message.h"
#include "tobasalis/bci/parser_vitek2compact.h"

namespace tbs {
namespace bci {

Vitek2CompactParser::Vitek2CompactParser(bool enableLog)
   : lis::Parser()
   , _enableLog(enableLog) 
{
   if (_enableLog) 
      Logger::logD("[lis_parser] Vitek2Compact parser initialized");   
}   

void Vitek2CompactParser::publisRecord(const std::shared_ptr<Record>& record)
{
   // emit record to subscriber
   if (onRecordReady)
   {
      // convert bci::Record into lis::Record before publishing
      auto pR = std::static_pointer_cast<lis::Record>(record);
      onRecordReady(pR);
   }
}

void Vitek2CompactParser::parse(const std::string& rawData)
{
   try
   {
      if (_enableLog) 
         Logger::logD("[lis_parser] parse: {}", rawData);
         
      /*
      VITEK2 upload 2 messages in to LIS a session

      mtrsl|iiV2|is00001899E653|itID|pi156468165|pnParker|si|s0102|sspus|s5pus|s12018/05/02|s32018/05/02|ci102|c0102|ctpus|cnpus|ta|rtGN|rr85805|t11|o1citfre|o2Citrobacter freundii|o34617611754561010|o995|zz|
      mtrsl|iiV2|is00001899E653|itSU|pi156468165|pnParker|si|s0102|sspus|s5pus|s12018/05/02|s32018/05/02|ci102|c0102|ctpus|cnpus|ta|rtAST-GN93|rr85805|t11|o1citfre|o2Citrobacter freundii|afQUINOLONES|apPARTIALLY RESISTANT|afTETRACYCLINES|apWILD|apRESISTANT|afBETA-LACTAMS|apHIGH LEVEL CASE (AmpC)|afFURANES|apWILD|afAMINOGLYCOSIDES|apWILD|afTRIMETHOPRIM/SULFONAMIDES|apTRIMETHOPRIM RESISTANT|apWILD|ra|a1am|a2Ampicillin|a3>=32|a4R|anR|ra|a1ams|a2Ampicillin/Sulbactam|a3>=32|a4R|anR|ra|a1tzp|a2Piperacillin/Tazobactam|a3>=128|a4R|anR|ra|a1cz06|a2Cefazolin (urine)|a3>=64|a4R|anR|ra|a1cz03|a2Cefazolin (other)|a3>=64|a4R|anR|ra|a1taz|a2Ceftazidime|a3>=64|a4R|anR|ra|a1ctr|a2Ceftriaxone|a3>=64|a4R|anR|ra|a1fep|a2Cefepime|a3<=1|a4S|anS|ra|a1azm|a2Aztreonam|a3>=64|a4R|anR|ra|a1etp|a2Ertapenem|a3<=0.5|a4S|anS|ra|a1mem|a2Meropenem|a3<=0.25|a4S|anS|ra|a1an|a2Amikacin|a3<=2|a4S|anS|ra|a1gm|a2Gentamicin|a3<=1|a4S|anS|ra|a1cip|a2Ciprofloxacin|a30.5|a4S|anS|ra|a1tgc|a2Tigecycline|a3<=0.5|a4S|anS|ra|a1ftn|a2Nitrofurantoin|a3<=16|a4S|anS|ra|a1sxt|a2Trimethoprim/Sulfamethoxazole|a3<=20|a4S|anS|zz|

      mtrsl|iiV2|is00001899E653|itID|pi08042018|pnAndreas Bruce|si|s0100|ssKULTUR DARAH|s5KULTUR DARAH|s12018/05/02|s32018/05/02|ci100|c0100|ctKULTUR DARAH|cnKULTUR DARAH|ta|rtGN|rr85030|t11|o1myrspp|o2Myroides spp|o35042000100040020|o999|zz|
      mtrsl|iiV2|is00001899E653|itSU|pi08042018|pnAndreas Bruce|si|s0100|ssKULTUR DARAH|s5KULTUR DARAH|s12018/05/02|s32018/05/02|ci100|c0100|ctKULTUR DARAH|cnKULTUR DARAH|ta|rtAST-GN93|rr85030|t11|o1myrspp|o2Myroides spp|ra|a1am|a2Ampicillin|a3>=32|a4R|anR|ra|a1ams|a2Ampicillin/Sulbactam|a3>=32|a4R|anR|ra|a1tzp|a2Piperacillin/Tazobactam|a3>=128|a4R|anR|ra|a1cz|a2Cefazolin|a3>=64|a4R|anR|ra|a1taz|a2Ceftazidime|a3>=64|a4R|anR|ra|a1fep|a2Cefepime|a3>=64|a4R|anR|ra|a1azm|a2Aztreonam|a3>=64|a4R|anR|ra|a1mem|a2Meropenem|a34|a4S|anS|ra|a1an|a2Amikacin|a3>=64|a4R|anR|ra|a1gm|a2Gentamicin|a3>=16|a4R|anR|ra|a1cip|a2Ciprofloxacin|a3>=4|a4R|anR|ra|a1tgc|a2Tigecycline|a32|a4S|anS|ra|a1ftn|a2Nitrofurantoin|a3128|a4R|anR|ra|a1sxt|a2Trimethoprim/Sulfamethoxazole|a3>=320|a4R|anR|zz|

      mtrsl|iiV2|is00001899E653|itID|pi19042018|pnParker Peter|si|s0101|ssKULTUR ABSES PU|s5KULTUR ABSES PUNGGUNG|s12018/05/02|s32018/05/02|ci101|c0101|ctKULTUR ABSES PUNGGUNG|cnKULTUR ABSES PUNGGUNG|ta|rtGP|rr85432|t11|o1staaur|o2Staphylococcus aureus|o3050402033773231|o991|zz|
      mtrsl|iiV2|is00001899E653|itSU|pi19042018|pnParker Peter|si|s0101|ssKULTUR ABSES PU|s5KULTUR ABSES PUNGGUNG|s12018/05/02|s32018/05/02|ci101|c0101|ctKULTUR ABSES PUNGGUNG|cnKULTUR ABSES PUNGGUNG|ta|rtAST-GP67|rr85432|t11|o1staaur|o2Staphylococcus aureus|afGLYCOPEPTIDES|apWILD|afRIFAMYCINES|apRESISTANT (LOW LEVEL)|apWILD|afQUINOLONES|apWILD|afTETRACYCLINES|apWILD|afBETA-LACTAMS|apACQUIRED PENICILLINASE|afFURANES|apWILD|afOXAZOLIDINONE|apWILD|afAMINOGLYCOSIDES|apRESISTANT KAN TOB (ANT(4')(4)) |apWILD|apRESISTANT KAN(APH(3')-III)|afTRIMETHOPRIM/SULFONAMIDES|apTRIMETHOPRIM RESISTANT|apWILD|afMACROLIDES/LINCOSAMIDES/STREPTOGRAMINS|apWILD|ra|a1oxsf|a2Cefoxitin Screen|a3Neg|a4-|an-|ra|a1peng|a2Benzylpenicillin|a3>=0.5|a4R|anR|ra|a1ox|a2Oxacillin|a3<=0.25|a4S|anS|ra|a1gm|a2Gentamicin|a3<=0.5|a4S|anS|ra|a1cip|a2Ciprofloxacin|a3<=0.5|a4S|anS|ra|a1lev|a2Levofloxacin|a3<=0.12|a4S|anS|ra|a1mxf|a2Moxifloxacin|a3<=0.25|a4S|anS|ra|a1icr|a2Inducible Clindamycin Resistance|a3Neg|a4-|an-|ra|a1e|a2Erythromycin|a3<=0.25|a4S|anS|ra|a1cc|a2Clindamycin|a3<=0.25|a4S|anS|ra|a1qda|a2Quinupristin/Dalfopristin|a3<=0.25|a4S|anS|ra|a1lnz|a2Linezolid|a32|a4S|anS|ra|a1va|a2Vancomycin|a3<=0.5|a4S|anS|ra|a1tet|a2Tetracycline|a3<=1|a4S|anS|ra|a1tgc|a2Tigecycline|a3<=0.12|a4S|anS|ra|a1ftn|a2Nitrofurantoin|a3<=16|a4S|anS|ra|a1rif|a2Rifampicin|a3<=0.5|a4S|anS|ra|a1sxt|a2Trimethoprim/Sulfamethoxazole|a3<=10|a4S|anS|zz|
      */

      // create clean raw bci string by removing RS char : \x1e  = Decimal 30
      std::string data = "";
      for (size_t i = 0; i < rawData.length(); i++)
      {
         char c = rawData[i];
         if (c != 30)
            data += rawData[i];
      }

      if (data.compare(0, 5, "mtrsl") == 0)
      {
         // Temporary vector of vector_pointer for storing grouped fields by record type
         std::vector< std::vector<std::string>* > 	bcisegments;
         // content of bcisegments
         std::vector<std::string>* pGroup = nullptr;

         // Split clean raw BCI string by "|" delimiter
         std::vector<std::string> fields = util::split(data, Record::FieldDelimiter);
         // Then regroup by record type
         for (size_t i = 0; i < fields.size(); i++)
         {
            std::string field = fields.at(i);

            if (util::startsWith(field, "mt"))          // General fields
            {
               pGroup = new std::vector<std::string>();
               bcisegments.push_back(pGroup);
            }

            if (util::startsWith(field, "pi"))          // Patient fields
            {
               pGroup = new std::vector<std::string>();
               bcisegments.push_back(pGroup);
            }

            if (util::startsWith(field, "si"))          // Specimen fields
            {
               pGroup = new std::vector<std::string>();
               bcisegments.push_back(pGroup);
            }

            if (util::startsWith(field, "ci"))          // Culture fields
            {
               pGroup = new std::vector<std::string>();
               bcisegments.push_back(pGroup);
            }

            if (util::startsWith(field, "ta"))          // Test fields
            {
               pGroup = new std::vector<std::string>();
               bcisegments.push_back(pGroup);
            }

            if (util::startsWith(field, "af"))          // Af fields (Repeatable)
            {
               pGroup = new std::vector<std::string>();
               bcisegments.push_back(pGroup);
            }

            if (util::startsWith(field, "ap"))          // Ap fields (Repeatable)
            {
               pGroup = new std::vector<std::string>();
               bcisegments.push_back(pGroup);
            }

            if (util::startsWith(field, "ra"))          // Susceptibility Result fields (Repeatable)
            {
               pGroup = new std::vector<std::string>();
               bcisegments.push_back(pGroup);
            }

            pGroup->push_back(field);
         }


         // -------------------------------------------------------
         // Create then build BciMessage object, storing every record type
         auto bciMessage = std::make_shared<BciMessage>();

         std::vector<std::vector<std::string>*>::iterator it;
         for (it = bcisegments.begin(); it != bcisegments.end(); ++it)
         {
            std::vector<std::string>* segment = *it;
            // Convert grouped fields in array to Bci String
            // Then create Record object, then add it to BciMessage
            std::string bciStr = Record::createBciStringFromArray(segment);

            std::string segStr = segment->at(0);
            if (util::startsWith(segStr, "mt"))
            {
               // bciStr =  "mtrsl|iiV2|is00001899E653|itSU|"
               auto recPtr = std::make_shared<GeneralRecordVitek2Compact>(bciStr);
               recPtr->fromString();
               bciMessage->setHeader(recPtr);
               publisRecord(recPtr);
            }

            if (util::startsWith(segStr, "pi"))
            {
               auto recPtr = std::make_shared<PatientRecordVitek2Compact>(bciStr);
               recPtr->fromString();
               bciMessage->getHeader()->addChild(recPtr);
               publisRecord(recPtr);
            }

            if (util::startsWith(segStr, "si"))
            {
               auto recPtr = std::make_shared<SpecimenRecordVitek2Compact>(bciStr);
               recPtr->fromString();
               bciMessage->getHeader()->addChild(recPtr);
               publisRecord(recPtr);
            }

            if (util::startsWith(segStr, "ci"))
            {
               auto recPtr = std::make_shared<CultureRecordVitek2Compact>(bciStr);
               recPtr->fromString();
               bciMessage->getHeader()->addChild(recPtr);
               publisRecord(recPtr);
            }

            if (util::startsWith(segStr, "ta"))
            {
               auto recPtr = std::make_shared<TestRecordVitek2Compact>(bciStr);
               recPtr->fromString();
               bciMessage->getHeader()->addChild(recPtr);
               // "ta" is Headers's last child
               publisRecord(recPtr);
            }

            if (util::startsWith(segStr, "af"))
            {
               // af is  ta's child
               auto recPtr = std::make_shared<AfRecordVitek2Compact>(bciStr);
               recPtr->fromString();
               bciMessage->getHeader()->getLastChild()->addChild(recPtr);
               publisRecord(recPtr);
            }

            if (util::startsWith(segStr, "ap"))
            {
               // ap is af'schild
               auto recPtr = std::make_shared<ApRecordVitek2Compact>(bciStr);
               recPtr->fromString();
               bciMessage->getHeader()->getLastChild()->getLastChild()->addChild(recPtr);
               publisRecord(recPtr);
            }

            if (util::startsWith(segStr, "ra"))
            {
               // ra is ta's child
               auto recPtr = std::make_shared<SusceptibilityResultRecordVitek2Compact>(bciStr);
               recPtr->fromString();
               bciMessage->getHeader()->getLastChild()->addChild(recPtr);
               publisRecord(recPtr);
            }

            delete segment; // Cleanup
         }
         
         bcisegments.clear(); // Cleanup


         // -------------------------------------------------------
         // message contructed, emit event
         if (onMessageReady)
         {
            auto msg = std::static_pointer_cast<lis::Message>(bciMessage);
            onMessageReady(msg);
         }
      }
      else 
         throw std::runtime_error("Failed parsing Vitect2 Compact data");
   }
   catch(const std::exception& ex)
   {
      if (onParserError)
         onParserError(ex.what());
   }
}

} // namespace bci
} // namespace tbs