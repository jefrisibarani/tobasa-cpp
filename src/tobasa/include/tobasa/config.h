#pragma once

#include <fstream>
#include <iostream>
#include <shared_mutex>
#include <unordered_map>
#include "tobasa/json.h"
#include "tobasa/span.h"
#include "tobasa/exception.h"
#include "tobasa/util_string.h"

namespace tbs {

/**
 * @ingroup TBS
 * @brief Application Configuration class managing settings via a JSON configuration file.
 */
class Config
{
private:
   
   // protects config data
   mutable std::shared_mutex _mutex; 
   
   bool        _valid {false};
   std::string _configFile;
   Json        _jsonConf;

   std::unordered_map<std::string, std::string> _configVariables;

   Config();
   ~Config();

   enum class ParserCallbackState
   {
      NONE,
      GOT_DEFINED_VARIABLE,
      DONE
   };
   ParserCallbackState _parserState = ParserCallbackState::NONE;
   static bool jsonPparserCallback(int depth, Json::parse_event_t event, Json& parsed, const std::string& configFileName="");

public:
   Config(const Config&) = delete;
   const Config& operator=(const Config&) = delete;
   Config(Config&&) = delete;
   const Config& operator=(Config&&) = delete;

   /**
    * @brief Retrieves the singleton instance of this class.
    * @return Pointer to the singleton instance.
    */
   static Config& get();

   /**
    * @brief Loads configuration settings from a JSON configuration file.
    * @param path The path to the JSON configuration file.
    * @param charData Alternative data source if parsing file failed.
    * @throw Json::exception and std::exception
    */
   void load(const std::string& path, nonstd::span<const unsigned char> charData={});

   /**
    * @brief Checks if the loaded configuration is valid.
    * @return True if the configuration is valid, false otherwise.
    */
   bool valid() { return _valid; }

   /**
    * @brief Retrieves the path of the configuration file.
    * @return The path to the configuration file.
    */
   std::string filePath() const { return _configFile; }

   /**
    * @brief Retrieves a configuration object from the JSON configuration based on the provided option name.
    * @tparam OptionType The type of the option object to retrieve.
    * @param optionName The name of the option to retrieve from the JSON configuration.
    * @return OptionType object corresponding to the specified option name.
    * @throws std::runtime_error if the configuration state is invalid.
    */   
   template <typename OptionType>
   static OptionType getOption(const std::string& optionName)
   {
      auto& instance = get();
      std::shared_lock lock(instance._mutex);

      if ( !instance.valid() ) 
         throw std::runtime_error("Invalid Configuration state");

      if ( instance._jsonConf.contains(optionName))
         return instance._jsonConf[optionName].get<OptionType>();
      else
      {
         std::cerr << "Config item " << optionName << "not found\n";
         return OptionType();
      }
   }


   /**
    * @brief Adds an option to the configuration.
    * @tparam OptionType The type of the option to add.
    * @param optionName The name of the option to be added.
    * @param path Path to the configuration file.
    */
   template <typename OptionType>
   static void addOption(const std::string& optionName, const std::string& path, nonstd::span<const unsigned char> charData={})
   {
      auto& instance = get();                 // get singleton instance
      std::shared_lock lock(instance._mutex); // lock its mutex

      if ( instance.valid() )
      {
         try
         {
            std::ifstream is(path, std::ifstream::in);
            if (is.good())
            {
               auto config = Json::parse(is, 
                                 [](int depth, Json::parse_event_t event, Json& parsed)
                                 {
                                    return jsonPparserCallback(depth, event, parsed);
                                 },
                                 /*allow exceptions*/ true, /*ignore_comments*/ true);
               // Okay with MSVC
               //auto configObj = config.get<OptionType>();
               // In GCC 11, we need to qualify OptionType with typename 
               // to explicitly tell the compiler that it's a type:
               auto configObj = config.template get<OptionType>();
               std::string keyName = optionName;
               instance._jsonConf[keyName] = configObj;
            }
            else if (!charData.empty())
            {
               auto config = Json::parse(charData.begin(), charData.end(),
                                 [](int depth, Json::parse_event_t event, Json& parsed)
                                 {
                                    return jsonPparserCallback(depth, event, parsed);
                                 },
                                 /*allow exceptions*/ true, /*ignore_comments*/ true);

               auto configObj = config.template get<OptionType>();
               std::string keyName = optionName;
               instance._jsonConf[keyName] = configObj;
            }
            else 
               throw std::runtime_error("could not parse json data");
         }
         catch (const Json::exception& ex)
         {
            throw AppException("Configuration file error " + cleanJsonException(ex) );
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
      else
         throw std::runtime_error("Invalid Configuration state");
   }

   /// @brief Get nested option by dotted path ("a.b.c")
   template <typename OptionType>
   static OptionType getNestedOption(const std::string& dottedPath)
   {
      auto& instance = get();
      std::shared_lock lock(instance._mutex);

      const Json* current = &instance._jsonConf;
      for (auto& key : util::split(dottedPath, '.') ) 
      {
         if (!current->contains(key)) 
         {
            throw std::runtime_error("Config key not found: " + dottedPath);
         }
         current = &((*current)[key]);
      }

      return current->get<OptionType>();
   }

   /// @brief Try get nested option by dotted path ("a.b.c") with default fallback
   template <typename OptionType>
   static OptionType tryGetNestedOption(const std::string& dottedPath, const OptionType& defaultValue)
   {
      auto& instance = get();
      std::shared_lock lock(instance._mutex);

      const Json* current = &instance._jsonConf;
      for (auto& key : util::split(dottedPath, '.') ) 
      {
         if (!current->contains(key)) {
            return defaultValue;  // key missing → fallback
         }
         current = &((*current)[key]);
      }

      try {
         return current->get<OptionType>();
      } catch (...) {
         return defaultValue;  // type mismatch → fallback
      }
   }

   /// @brief Set nested option by dotted path ("a.b.c")
   static void setNestedOption(const std::string& dottedPath, const Json& value);

   const Json& getConfiguration()
   {
      std::shared_lock lock(_mutex);
      return _jsonConf;
   }
};

} // namespace tbs
