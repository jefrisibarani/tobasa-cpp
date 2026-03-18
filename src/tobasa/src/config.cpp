#include <fstream>
#include <exception>
#include <iostream>
#include <mutex>
#include "tobasa/path.h"
#include "tobasa/exception.h"
#include "tobasa/config.h"

namespace tbs {

Config::Config() 
   : _valid(false)
{}

Config::~Config() 
{
   std::cout << "Config destroyed\n";
}

Config& Config::get()
{
   static Config instance; // Meyers' singleton, thread-safe (C++11+)
   return instance;
}

void Config::load(const std::string& path, nonstd::span<const unsigned char> charData)
{
   std::unique_lock lock(_mutex);

   _configFile = path;
   try
   {
      auto fileName = path::fileNameWithExtension(_configFile);

      std::ifstream is(path, std::ifstream::in);
      if (is.good())
      {
         _jsonConf =  Json::parse( is, 
                        [fileName](int depth, Json::parse_event_t event, Json& parsed)
                        {
                           return jsonPparserCallback(depth, event, parsed, fileName);
                        },
                        /*allow exceptions*/ true, /*ignore_comments*/ true);
         
         
         _valid = true;
      }
      else if (!charData.empty())
      {
         _jsonConf = Json::parse(charData.begin(), charData.end(),
                        [fileName](int depth, Json::parse_event_t event, Json& parsed)
                        {
                           return jsonPparserCallback(depth, event, parsed, fileName);
                        },
                        /*allow exceptions*/ true, /*ignore_comments*/ true);
         _valid = true;
      }

      if (!_valid)
         throw std::runtime_error("could not parse json data");

   }
   catch (const Json::exception& ex)
   {
      throw AppException( "Configuration file error " + cleanJsonException(ex) );
   }
   catch(const AppException& ex)
   {
      throw ex;
   }   
   catch (const std::exception& ex)
   {
      throw ex;
   }
}

bool Config::jsonPparserCallback(int depth, Json::parse_event_t event, Json& parsed, const std::string& configFileName)
{
   if (event == Json::parse_event_t::key && configFileName == "appsettings.json")
   {
      // skip object elements with key "text_to_be_skipped"
      //if (parsed == Json("text_to_be_skipped")) { return false;}
      if (depth == 1)
      {
         if (parsed == Json("configVariables")) {
            get()._parserState = ParserCallbackState::GOT_DEFINED_VARIABLE;
         }
      }
   }
   else if (event == Json::parse_event_t::object_end && configFileName == "appsettings.json")
   {
      if (depth == 1)
      {
         if (get()._parserState == ParserCallbackState::GOT_DEFINED_VARIABLE)
         {
            // Initialize config variables
            for (auto& element : parsed.items()) 
            {
               auto key = element.key();
               auto val = element.value().is_string() ? element.value().get<std::string>() : "";
               get()._configVariables[element.key()] =  val;
            }

            get()._parserState = ParserCallbackState::DONE;
         }
      }
   }   
   else if (event == Json::parse_event_t::value )
   {  
      if (parsed.is_string()) 
      {
         std::string value = parsed.get<std::string>();
         
         while (true)
         {
            std::size_t startPos = value.find("${");
            std::size_t endPos   = value.find("}", startPos);

            if (startPos != std::string::npos && endPos != std::string::npos && endPos > startPos) 
            {
               std::string variableKey = value.substr(startPos, endPos - startPos + 1);
               auto it = get()._configVariables.find(variableKey);
               if (it == get()._configVariables.end()) 
                  throw AppException("Undefined configuration file variable " + variableKey);

               std::string variableValue = get()._configVariables[variableKey];
               value.replace(startPos, endPos - startPos + 1, variableValue);
            }
            else
               break;
         }

         parsed = value;
      } 
   }

   return true;
}

/// Set nested option by dotted path ("a.b.c")
void Config::setNestedOption(const std::string& dottedPath, const Json& value)
{
   auto& instance = get();
   std::unique_lock lock(instance._mutex);

   Json* current = &instance._jsonConf;
   auto parts = util::split(dottedPath, '.');

   for (size_t i = 0; i < parts.size() - 1; ++i) {
      current = &((*current)[parts[i]]);
   }

   (*current)[parts.back()] = value;
}



} // namespace tbs
