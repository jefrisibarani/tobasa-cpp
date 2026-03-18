#pragma once

#include "inja.hpp"

namespace tbs {
namespace injautil {

std::string intToAlphaChar(int val);

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
std::string lookupArray(inja::Arguments& args);

std::string comboBox(inja::Arguments& args);

std::string appResourceUrl(inja::Arguments& args, const std::string& baseUrl);

std::string gender(inja::Arguments& args);

} // namespace injautil
} // namespace tbs