#include <tobasa/logger.h>
#include <tobasa/format.h>
#include <tobasa/json.h>
#include "inja_util.h"

namespace tbs {
namespace injautil {

std::string intToAlphaChar(int val)
{
   int num = val-1;
   // Ensure the input is within the range 0-25 (inclusive)
   int modNum = num % 26;
   
   // Convert the number to the corresponding character 'a' to 'z'
   char alphaChar = 'a' + modNum;
   
   return std::string(1,alphaChar);
}

/**
   Get a value from an array of objects
   
   example usage in template: 

   lookupArray(datasource, fieldsSource, keyValue )
   where:
   datasource   : json array of object 
                  
                  [
                     { id: 1, code: "XX", name: "xx name"},
                     { id: 2, code: "YY", name: "YY name"},
                     { id: 3, code: "ZZ", name: "ZZ name"},
                  ]

   fieldsSource : "ID_CODE" or "CODE_NAME"
                  ID_CODE:  get value from field "code" with key from field "id"
                  CODE_NAME: get value from field "name" with key from field "code"
   keyValue     : the variable containing the key we want to search in the datasource
                  key: string or numeric(int,long)

 */
std::string lookupArray(inja::Arguments& args)
{
   auto datasource   = args.at(0)->get<Json>(); 
   auto fieldsSource = args.at(1)->get<std::string>(); 
   
   for (auto x: datasource.items()) 
   {
      //auto key = x.key();
      auto obj  = x.value();
      auto id   = obj["id"].get<long>(); 
      auto code = obj["code"].get<std::string>(); 
      auto name = obj["name"].get<std::string>(); 

      if (fieldsSource == "ID_CODE")
      {
         auto keyValue = args.at(2)->get<long>(); 
         if (id == keyValue)
            return code;
      }
      else if (fieldsSource == "CODE_NAME")
      {
         auto keyValue = args.at(2)->get<std::string>(); 
         if (code == keyValue)
            return name;
      }
   }   

   return "";
}

/// @brief render a combo box
/// @param args , first object of args must be a valid Json object which has
/// minimal three members: id (int), name (string), code (string)
/// @return 
std::string comboBox(inja::Arguments& args)
{
   std::string htmlCode;

   auto datasource   = args.at(0)->get<Json>(); 
   auto comboId      = args.at(1)->get<std::string>(); 
   auto comboName    = args.at(2)->get<std::string>(); 
   auto comboClass   = args.at(3)->get<std::string>(); 
   auto initialValue = args.at(4)->get<std::string>(); 
   auto fieldsSource = args.at(5)->get<std::string>(); // "ID_CODE" or "CODE_NAME" or ""
   
   htmlCode = tbsfmt::format("<select class=\"{}\" name=\"{}\" id=\"{}\">\n", comboClass, comboName, comboId);

   for (auto x: datasource.items()) 
   {
      auto obj  = x.value();
      auto code = obj["code"].get<std::string>();
      auto name = obj["name"].get<std::string>();

      std::string id;

      if (obj["id"].is_number())
      {
         auto id_ = obj["id"].get<long>(); 
         id = std::to_string(id_);
      }
      else if (obj["id"].is_string()) {
         id = obj["id"].get<std::string>(); 
      }

      std::string selected;

      if (fieldsSource == "ID_CODE" && initialValue == id)
         selected = "selected=\"selected\"";
      else if (fieldsSource == "CODE_NAME" && initialValue == code)
         selected = "selected=\"selected\"";

      if (fieldsSource == "CODE_NAME")
      {
         // use code and name
         htmlCode += tbsfmt::format("   <option value=\"{}\" {}>{}</option>\n", code, selected, name); 
      }
      else
      {
         // "ID_CODE"
         // use id and code
         htmlCode += tbsfmt::format("   <option value=\"{}\" {}>{}</option>\n", id, selected, code); 
      }
   }

   htmlCode += "</select>";

   return htmlCode;   
}

std::string appResourceUrl(inja::Arguments& args, const std::string& baseUrl)
{
   auto resourceLocation = args.at(0)->get<std::string>(); 
   auto fileName = args.at(1)->get<std::string>(); 
   
   std::string url;

   if      (resourceLocation == "DATA")
      url = baseUrl + "/resource/data/" + fileName;
   else if (resourceLocation == "IMAGE")
      url = baseUrl + "/resource/image/" + fileName;
   else if (resourceLocation == "UPLOAD")
      url = baseUrl + "/resource/upload/" + fileName;
   else if (resourceLocation == "REPORT")
      url = baseUrl + "/resource/report/" + fileName;
   else
   {
      if (resourceLocation == "images_user" && fileName.empty())
         url = baseUrl + "/assets/images/_default_person.jpg"; 
      else if (resourceLocation == "images_carousel" && fileName.empty())
         url = baseUrl + "/assets/images_carousel/default_slide1.jpg"; 
      else if (resourceLocation == "images_doctor" && fileName.empty())
         url = baseUrl + "/assets/images/_default_doctor.jpg";
      else
         url = baseUrl + "/resource/" + resourceLocation + "/" + fileName; 
   }

   return url;
}

std::string gender(inja::Arguments& args)
{
   auto gender = args.at(0)->get<std::string>(); 
   if (gender == "F")
      return "Female";
   else if (gender == "M")
      return "Male";
   else if (gender == "L")
      return "Laki-laki";
   else if (gender == "P")
      return "Perempuan";
   else
      return gender;
}

} // namespace injautil
} // namespace tbs