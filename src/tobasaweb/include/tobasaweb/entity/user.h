#pragma once

#include <string>
#include <memory>
#include <tobasa/datetime.h>
#include <tobasa/json.h>

namespace tbs {
namespace web {
namespace entity {

/** \ingroup WEB
 * @{
 */

struct User
{
   long        id = -1;
   std::string uuid;
   std::string userName;
   std::string firstName;
   std::string lastName;
   std::string email;
   std::string image;
   bool        enabled;
   std::string passwordSalt;
   std::string passwordHash;
   bool        allowLogin;
   DateTime    created;
   DateTime    updated;
   DateTime    expired;
   DateTime    lastLogin;
   std::string uniqueCode;
   DateTime    birthDate;
   std::string phone;
   std::string gender;
   std::string address;
   std::string nik;

   int         selectedSiteId;
   std::string selectedLangId;
};

using UserPtr  = std::shared_ptr<User>;
using UserList = std::vector<User>;


struct UserDto
{
   long        id;
   std::string uuid;
   std::string userName;
   std::string firstName;
   std::string lastName;
   std::string email;
   std::string image;
   bool        enabled;
   std::string passwordSalt;
   std::string passwordHash;
   bool        allowLogin;
   std::string created;
   std::string updated;
   std::string expired;
   std::string lastLogin;
   std::string uniqueCode;
   std::string birthDate;
   std::string phone;
   std::string gender;
   std::string address;
   std::string nik;

   UserDto() {}

   UserDto(UserPtr pUser)
   {
      id           = pUser->id;
      uuid         = pUser->uuid;
      userName     = pUser->userName;
      firstName    = pUser->firstName;
      lastName     = pUser->lastName;
      email        = pUser->email;
      image        = pUser->image;
      enabled      = pUser->enabled;
      passwordSalt = pUser->passwordSalt;
      passwordHash = pUser->passwordHash;
      allowLogin   = pUser->allowLogin;
      created      = pUser->created.isoDateTimeString();
      updated      = pUser->updated.isoDateTimeString();
      expired      = pUser->expired.isoDateTimeString();
      lastLogin    = pUser->lastLogin.isoDateTimeString();
      uniqueCode   = pUser->uniqueCode;
      birthDate    = pUser->birthDate.isoDateString();
      phone        = pUser->phone;
      gender       = pUser->gender;
      address      = pUser->address;
      nik          = pUser->nik;
   }


   static UserPtr toUser(UserDto& dto)
   {
      auto user = std::make_shared<User>();
      user->id           = dto.id;
      user->uuid         = dto.uuid;
      user->userName     = dto.userName;
      user->firstName    = dto.firstName;
      user->lastName     = dto.lastName;
      user->email        = dto.email;
      user->image        = dto.image;
      user->enabled      = dto.enabled;
      user->passwordSalt = dto.passwordSalt;
      user->passwordHash = dto.passwordHash;
      user->allowLogin   = dto.allowLogin;

      user->uniqueCode   = dto.uniqueCode;
      user->phone        = dto.phone;

      DateTime dt;
      if (dt.parse(dto.created))
         user->created   = dt;
      else return nullptr;

      if (dt.parse(dto.updated))
         user->updated   = dt;
      else return nullptr;

      if (dt.parse(dto.expired))
         user->expired   = dt;
      else return nullptr;

      if (dt.parse(dto.lastLogin))
         user->lastLogin = dt;
      else return nullptr;

      if (dt.parse(dto.birthDate, "%Y-%m-%d"))
         user->birthDate = dt;
      else return nullptr;

      return user;
   }

};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserDto, id, uuid, userName, firstName, lastName, email, 
   image, enabled, passwordSalt, passwordHash, allowLogin, created, updated, expired, 
   lastLogin, uniqueCode, birthDate, phone, gender, address, nik)

using UserDtoList = std::vector<UserDto>;


struct UserCreateDto
{
   std::string userName;
   std::string firstName;
   std::string lastName;
   std::string email;
   std::string password;
   std::string uniqueCode;
   std::string birthDate;
   std::string phone;
   std::string gender;
   std::string address;
   std::string nik;
   long        selectedSiteId;
   std::string selectedLangId;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserCreateDto, userName, firstName, lastName, email,
   password, uniqueCode, birthDate, phone, gender, address, nik, selectedSiteId, selectedLangId)


struct UserSimple
{
   long        id;
   std::string uuid;
   std::string userName;
   std::string firstName;
   std::string lastName;
   std::string email;
   bool        enabled;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UserSimple, id, uuid, userName, firstName, lastName, email, enabled)
using UserSimpleList = std::vector<UserSimple>;

/** @}*/

} // namespace entity
} // namespace web
} // namespace tbs