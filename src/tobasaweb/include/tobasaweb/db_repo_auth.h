#pragma once

#include <tobasa/logger.h>
#include <tobasa/config.h>
#include <tobasa/crypt.h>
#include <tobasa/util.h>
#include <tobasasql/sql_connection.h>
#include <tobasasql/sql_query.h>
#include "tobasaweb/util.h"
#include "tobasaweb/jwt.h"
#include "tobasaweb/db_repo_auth_base.h"
#include "tobasaweb/entity/user_role.h"
#include "tobasaweb/exception.h"


namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

/** 
 * AuthDbRepo
 * \tparam SqlDriverType
 */
template < typename SqlDriverType >
class AuthDbRepo : public AuthDbRepoBase
{
public:
   using SqlResult     = sql::SqlResult<SqlDriverType>;
   using SqlConnection = sql::SqlConnection<SqlDriverType>;
   using SqlQuery      = sql::SqlQuery<SqlDriverType>;
   using Helper        = typename SqlDriverType::HelperImpl;

private:
   SqlConnection& _sqlConn;

public:
   AuthDbRepo() = default;
   virtual ~AuthDbRepo() = default;
   AuthDbRepo(SqlConnection& conn) : _sqlConn{ conn } {}

   // -------------------------------------------------------

   UserPtr authenticate(const std::string& userName, const std::string& password, const int siteId=1)
   {
      dto::LoginUser userDto;
      userDto.userName = userName;
      userDto.password = password;

      return authenticate(userDto);
   }

   UserPtr authenticate(const web::dto::LoginUser& userDto)
   {
      if (userDto.userName.empty() || userDto.password.empty())
         throw ValidationException("Username or password is incorrect");

      auto user = getUserByName(userDto.userName);

      // check if userName exists
      if (user == nullptr)
         throw ValidationException("Username not found");

      if (user->userName.empty())
         throw AppException("Internal error occured when getting user data");

      // before steppin further, check if password is correct.
      if (!tbs::util::verifySecureHash(userDto.password, user->passwordHash, user->passwordSalt))
         throw ValidationException("Username or password is incorrect");

      // check if user locked
      if (user->enabled == false)
         throw  ValidationException("Account disabled/not activated");

      // check if user can login
      if (user->allowLogin == false)
         throw ValidationException("Account is not allowed to logon");

      // check if user expired
      if (user->expired.timePoint() < DateTime().timePoint())
         throw ValidationException("Account has expired");

      // Global checks done, and password verified
      // give access if super user or site admin
      // Global Administrator, allow login
      if (!isSuperUser(user->id))
      {
         // Site Administrator, allow login
         if (!isSiteAdministrator(user->id, userDto.selectedSiteId))
         {
            // check if user allowed to login to specified hospital(site)
            if (!canLoginToSite(user->id, userDto.selectedSiteId))
               throw ValidationException("You are not allowed to login to specified site");
         }
      }

      // authentication successful
      updateLastLogin(user);

      user->selectedSiteId = userDto.selectedSiteId;
      user->selectedLangId = userDto.selectedLangId;

      return user;
   }

   /// @brief Add user into database
   /// @note this method use SQL Transaction
   UserPtr enrollNewUSer(const User& user, const std::string& password, bool checkAll)
   {
      if (user.userName.empty())
         throw ValidationException("User name is required");

      if (user.userName.length() < 4)
         throw ValidationException("User name minimum length is 4 characters");

      if (user.userName.length() > 20)
         throw ValidationException("User name maxumum length is 20 characters");

      if (password.empty())
         throw ValidationException("Password is required");

      if (password.length() < 5 )
         throw ValidationException("Password minimum length is 5 characters");

      if (password.length() > 100 )
         throw ValidationException("Password maximum length is 100 characters");

      if (user.firstName.empty() && checkAll)
         throw ValidationException("Bad parameter, user first name");

      if (user.lastName.empty() && checkAll)
         throw ValidationException("Bad parameter, user last name");

      if (user.email.empty())
         throw ValidationException("Bad parameter, email Address");

      if (exists(user.userName))
         throw ValidationException("User name " + user.userName + " is already taken");

      if (uniqueCodeExists(user.uniqueCode) && checkAll)
         throw ValidationException("Unique code " + user.uniqueCode + " is already taken");

      if (nikExists(user.nik) && checkAll)
         throw ValidationException("NIK " + user.nik + " is already taken");

      
      UserPtr pNewUSer = nullptr;

      this->beginTransaction();

      if (! create(user, password))
      {
         this->rollbackTransaction();
         return nullptr;
      }
      
      pNewUSer = getUserByName(user.userName);
      if (pNewUSer == nullptr)
      {
         this->rollbackTransaction();
         return nullptr;
      }
      
      if (pNewUSer->id == -1)
      {
         this->rollbackTransaction();
         return nullptr;
      }

      bool isFirstAccount = noRegisteredUsersAndRoles();
      if (isFirstAccount)
      {
         if (! createDefaultRolesAndAddFirstUserAsAdmin(pNewUSer) )
         {
            return nullptr;
            this->rollbackTransaction();
         }
      }
      else
      {
         if (! insertIntoBaseUserRole(pNewUSer))
         {
            this->rollbackTransaction();
            return nullptr;
         }

         long selectedSiteId = user.selectedSiteId;
         if (! insertIntoBaseUserSite(pNewUSer, selectedSiteId) )
         {
            this->rollbackTransaction();
            return nullptr;
         }
      }

      this->commitTransaction();
      return  pNewUSer;
   }

   UserPtr updateUserProfile(const User& user)
   {
      if (user.userName.empty())
         throw ValidationException("User name is required");

      if (user.userName.length() < 4)
         throw ValidationException("User name minimum length is 4 characters");

      if (user.userName.length() > 20)
         throw ValidationException("User name maximum length is 20 characters");

      if (user.firstName.empty())
         throw ValidationException("Bad parameter, user first name");

      if (user.lastName.empty())
         throw ValidationException("Bad parameter, user last name");

      if (user.email.empty())
         throw ValidationException("Bad parameter, email Address");

      if (user.uniqueCode.empty())
         throw ValidationException("Bad parameter, unique code");

      if (user.phone.empty())
         throw ValidationException("Bad parameter, phone number");

      //if (user.gender.empty())
      //   throw ValidationException("Bad parameter, gender");

      if (user.address.empty())
         throw ValidationException("Bad parameter, address");

      if (user.nik.empty())
         throw ValidationException("Bad parameter, nik");

      // get user data from DB
      auto pUser = getUserById(user.id);
      if (pUser==nullptr)
         throw ValidationException("User not found");

      if (pUser->userName != user.userName)
      {
         // username is about to be changed, therefore check if the new username is already taken
         if (exists(user.userName))
            throw ValidationException("User name " + user.userName + " is already taken");
      }

      if ( !user.uniqueCode.empty() && pUser->uniqueCode != user.uniqueCode )
      {
         if (existsByUniqueCode(user.uniqueCode))
            throw ValidationException("Unique code " + user.uniqueCode + " is already taken");
      }

      pUser->userName   = user.userName;
      pUser->firstName  = user.firstName;
      pUser->lastName   = user.lastName;
      pUser->email      = user.email;
      pUser->image      = user.image;
      pUser->uniqueCode = user.uniqueCode;
      pUser->birthDate  = user.birthDate;
      pUser->phone      = user.phone;
      pUser->gender     = user.gender;
      pUser->address    = user.address;
      pUser->nik        = user.nik;

      if (user.image != "DO_NOT_UPDATE")
         pUser->image = user.image;

      if (update(pUser))
         return getUserByName(user.userName);
      else
         return nullptr;
   }

   std::vector<entity::UserDto> getAllUserDto()
   {
      std::vector<entity::UserDto> usersVector;
      std::string sql = "SELECT * FROM base_users ORDER BY id ASC";
      SqlQuery query(_sqlConn, sql);
      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);

            entity::UserDto user;
            user.id           = sqlResult->getLongValue("id");
            user.uuid         = sqlResult->getStringValue("uuid"); 
            user.userName     = sqlResult->getStringValue("user_name");
            user.firstName    = sqlResult->getStringValue("first_name");
            user.lastName     = sqlResult->getStringValue("last_name");
            user.email        = sqlResult->getStringValue("email");
            user.image        = sqlResult->getStringValue("image");
            user.enabled      = sqlResult->getBoolValue("enabled");
            user.passwordSalt = sqlResult->getStringValue("password_salt");
            user.passwordHash = sqlResult->getStringValue("password_hash");
            user.allowLogin   = sqlResult->getBoolValue("allow_login");
            user.created      = sqlResult->getDateTimeValue("created").isoDateTimeString();
            user.updated      = sqlResult->getDateTimeValue("updated").isoDateTimeString();
            user.expired      = sqlResult->getDateTimeValue("expired").isoDateTimeString();
            user.lastLogin    = sqlResult->getDateTimeValue("last_login").isoDateTimeString();
            user.uniqueCode   = sqlResult->getStringValue("unique_code");
            user.birthDate    = sqlResult->getDateTimeValue("birth_date").isoDateString();
            user.phone        = sqlResult->getStringValue("phone");
            user.gender       = sqlResult->getStringValue("gender");
            user.address      = sqlResult->getStringValue("address");
            user.nik          = sqlResult->getStringValue("nik");

            usersVector.push_back(user);
         }
      }

      return usersVector;
   }

   std::vector<entity::UserPtr> getAllUserPtr()
   {
      std::vector<entity::UserPtr> usersVector;
      SqlQuery query(_sqlConn, "SELECT * FROM base_users ORDER BY id ASC");
      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);

            entity::UserPtr user = std::make_shared<User>();
            user->id           = sqlResult->getLongValue("id");
            user->uuid         = sqlResult->getStringValue("uuid"); 
            user->userName     = sqlResult->getStringValue("user_name");
            user->firstName    = sqlResult->getStringValue("first_name");
            user->lastName     = sqlResult->getStringValue("last_name");
            user->email        = sqlResult->getStringValue("email");
            user->image        = sqlResult->getStringValue("image");
            user->enabled      = sqlResult->getBoolValue("enabled");
            user->passwordSalt = sqlResult->getStringValue("password_salt");
            user->passwordHash = sqlResult->getStringValue("password_hash");
            user->allowLogin   = sqlResult->getBoolValue("allow_login");
            user->created      = sqlResult->getDateTimeValue("created");
            user->updated      = sqlResult->getDateTimeValue("updated");
            user->expired      = sqlResult->getDateTimeValue("expired");
            user->lastLogin    = sqlResult->getDateTimeValue("last_login");
            user->uniqueCode   = sqlResult->getStringValue("unique_code");
            user->birthDate    = sqlResult->getDateTimeValue("birth_date");
            user->phone        = sqlResult->getStringValue("phone");
            user->gender       = sqlResult->getStringValue("gender");
            user->address      = sqlResult->getStringValue("address");
            user->nik          = sqlResult->getStringValue("nik");

            usersVector.push_back(user);
         }
      }

      return usersVector;
   }

   UserPtr getUserById(long id)
   {
      SqlQuery query(_sqlConn,"SELECT * FROM base_users WHERE id=:id");
      query.addParam("id", sql::DataType::integer, id);
      auto result = query.executeResult();
      return getUser(result);
   }

   UserPtr getUserByUuid(const std::string& uuid)
   {
      SqlQuery query(_sqlConn,"SELECT * FROM base_users WHERE uuid=:uuid");
      query.addParam("uuid", sql::DataType::varchar, uuid);
      auto result = query.executeResult();
      return getUser(result);
   }

   UserPtr getUserByName(const std::string& name)
   {
      std::string sql = "SELECT * FROM base_users WHERE user_name=:name";
      SqlQuery query(_sqlConn, sql);
      query.addParam("name", sql::DataType::varchar, name);
      auto result = query.executeResult();
      return getUser(result);
   }

   bool create(const User& user, const std::string& password)
   {
      try
      {
         // create hex encoded hash
         std::string passwordHash, passwordSalt;
         tbs::util::createSecureHash(password, passwordHash, passwordSalt);

         auto currentTime = DateTime();
         currentTime.timePoint() += tbsdate::years{2};
         DateTime expiredDate(currentTime.timePoint());
         std::string expiredDateString = expiredDate.isoDateTimeString();

         std::string sql = 
         R"-( INSERT INTO base_users (
                uuid, user_name, first_name, last_name, email, image, enabled, password_salt,
                password_hash, allow_login, expired, unique_code, birth_date, phone, gender, address, nik )
              VALUES (
                :uuid, :usrname, :firstName, :lastName, :email, :image, :enabled, :salt,
                :hash, :allow, :expired, :unqCode, :dob, :phone, :gender, :address, :nik ) )-";

         SqlQuery query(_sqlConn, sql);
         DateTime birthDate = user.birthDate;

         query.addParam("uuid",      sql::DataType::varchar,   util::generateUniqueId() );
         query.addParam("usrname",   sql::DataType::varchar,   user.userName);
         query.addParam("firstName", sql::DataType::varchar,   user.firstName);
         query.addParam("lastName",  sql::DataType::varchar,   user.lastName);
         query.addParam("email",     sql::DataType::varchar,   user.email);
         query.addParam("image",     sql::DataType::varchar,   user.image);
         query.addParam("enabled",   sql::DataType::boolean,   user.enabled);
         query.addParam("salt",      sql::DataType::varbinary, passwordSalt, static_cast<long>(passwordSalt.length()/2));
         query.addParam("hash",      sql::DataType::varbinary, passwordHash, static_cast<long>(passwordHash.length()/2));
         query.addParam("allow",     sql::DataType::boolean,   user.allowLogin);
         query.addParam("expired",   sql::DataType::timestamp, expiredDateString);
         query.addParam("unqCode",   sql::DataType::varchar,   user.uniqueCode);
         query.addParam("dob",       sql::DataType::date,      birthDate.isoDateString() );
         query.addParam("phone",     sql::DataType::varchar,   user.phone);
         query.addParam("gender",    sql::DataType::varchar,   user.gender);
         query.addParam("address",   sql::DataType::varchar,   user.address);
         query.addParam("nik",       sql::DataType::varchar,   user.nik);

         bool success = query.executeVoid();
         return success;
      }
      catch(std::exception& ex)
      {
         Logger::logE("[webapp] AuthDbRepo create, {}", ex.what());
      }

      return false;
   }

   bool update(UserPtr pUser)
   {
      try
      {
         std::string sql = 
         R"-( UPDATE base_users SET
                  uuid          = :uuid,      user_name     = :uname,
                  first_name    = :fname,     last_name     = :lname,
                  email         = :email,     image         = :image,
                  enabled       = :enabled,   allow_login   = :allow,
                  updated       = :updated,   expired       = :expired,
                  unique_code   = :ucode,     birth_date    = :dob,
                  phone         = :phone,     gender        = :gender,
                  address       = :address,   nik           = :nik
               WHERE id= :id )-";

         SqlQuery query(_sqlConn, sql);
         
         query.addParam("uuid",    sql::DataType::varchar,   pUser->uuid);
         query.addParam("uname",   sql::DataType::varchar,   pUser->userName);
         query.addParam("fname",   sql::DataType::varchar,   pUser->firstName);
         query.addParam("lname",   sql::DataType::varchar,   pUser->lastName);
         query.addParam("email",   sql::DataType::varchar,   pUser->email);
         query.addParam("image",   sql::DataType::varchar,   pUser->image);
         query.addParam("enabled", sql::DataType::boolean,   pUser->enabled);
         query.addParam("allow",   sql::DataType::boolean,   pUser->allowLogin);
         query.addParam("updated", sql::DataType::timestamp, DateTime().isoDateTimeString());
         query.addParam("expired", sql::DataType::timestamp, pUser->expired.isoDateTimeString());
         //query.addParam("llogin",  sql::DataType::timestamp, pUser->lastLogin.isoDateTimeString());
         query.addParam("ucode",   sql::DataType::varchar,   pUser->uniqueCode);
         query.addParam("dob",     sql::DataType::date,      pUser->birthDate.isoDateString());
         query.addParam("phone",   sql::DataType::varchar,   pUser->phone);
         query.addParam("gender",  sql::DataType::varchar,   pUser->gender);
         query.addParam("address", sql::DataType::varchar,   pUser->address);
         query.addParam("nik",     sql::DataType::varchar,   pUser->nik);
         query.addParam("id",      sql::DataType::integer,   pUser->id);

         bool success = query.executeVoid();
         return success;
      }
      catch(std::exception& ex)
      {
         Logger::logE("[webapp] AuthDbRepo update, {}", ex.what());
      }

      return false;
   }

   bool remove(long id)
   {
      try
      {
         std::string sql = "DELETE FROM base_users WHERE id=:id";
         SqlQuery query(_sqlConn, sql);
         query.addParam("id", sql::DataType::integer, id);
         bool success = query.executeVoid();
         return success;
      }
      catch(std::exception& ex)
      {
         Logger::logE("[webapp] AuthDbRepo remove, {}", ex.what());
      }

      return false;
   }

   bool exists(const std::string& name)
   {
      std::string sql = "SELECT COUNT(id) FROM base_users WHERE user_name=:name";
      SqlQuery query(_sqlConn, sql);
      query.addParam("name", sql::DataType::varchar, name);

      std::string result = query.executeScalar();
      if (util::isNumber(result))
         return (std::stoi(result) == 1);
      else
         return false;
   }

   bool exists(long id)
   {
      std::string sql = "SELECT COUNT(id) FROM base_users WHERE id=:id";
      SqlQuery query(_sqlConn, sql);
      query.addParam("id", sql::DataType::integer, id);

      std::string result = query.executeScalar();
      if (util::isNumber(result))
         return (std::stoi(result) == 1);
      else
         return false;
   }

   bool existsByUniqueCode(const std::string& code)
   {
      std::string sql = "SELECT COUNT(id) FROM base_users WHERE unique_code=:code";
      SqlQuery query(_sqlConn, sql);
      query.addParam("code", sql::DataType::varchar, code);

      std::string result = query.executeScalar();
      if (util::isNumber(result))
         return (std::stoi(result) == 1);
      else
         return false;
   }

   bool canLogin(long id)
   {
      std::string sql = R"-(
         SELECT COUNT(u.id) FROM base_users u
         WHERE u.id=:id AND u.enabled=:enabled AND u.allow_login=:allow )-";
      SqlQuery query(_sqlConn, sql);

      query.addParam("id",      sql::DataType::integer, id);
      query.addParam("enabled", sql::DataType::boolean, true);
      query.addParam("allow",   sql::DataType::boolean, true);

      std::string result = query.executeScalar();
      if (util::isNumber(result))
         return (std::stoi(result) == 1);
      else
         return false;
   }

   bool canLoginToSite(long userId, long siteId)
   {
      std::string sql = R"-(
         SELECT COUNT(s.id) FROM base_user_site s
         WHERE s.user_id=:uid AND s.site_id=:sid AND s.allow_login=:allow )-";
      
      SqlQuery query(_sqlConn, sql);
      query.addParam("uid",   sql::DataType::integer, userId);
      query.addParam("sid",   sql::DataType::integer, siteId);
      query.addParam("allow", sql::DataType::boolean, true);

      std::string result = query.executeScalar();
      if (util::isNumber(result))
         return (std::stoi(result) == 1);
      else
         return false;
   }

   bool isSuperUser(long userId)
   {
      std::string sql = R"-(
         SELECT u.id FROM base_users u WHERE u.id=:id AND u.id IN
            (SELECT ur.user_id FROM base_user_role ur
            JOIN base_roles r ON ur.role_id = r.id
            WHERE r.name=:name AND r.sysrole=:sysrole AND r.enabled=:enabled) )-";

      SqlQuery query(_sqlConn, sql);
      query.addParam("id",      sql::DataType::integer, userId);
      query.addParam("name",    sql::DataType::varchar, std::string("role_admin"));
      query.addParam("sysrole", sql::DataType::boolean, true);
      query.addParam("enabled", sql::DataType::boolean, true);

      std::string result = query.executeScalar();
      if (util::isNumber(result))
         return (std::stol(result) == userId);
      else
         return false;
   }

   bool isSiteAdministrator(long userId, long siteId)
   {
      std::string sql = R"-( 
         SELECT COUNT(us.id) FROM base_user_site us
         WHERE us.user_id=:userid AND us.site_id=:siteid AND us.is_admin=:isadmin AND us.allow_login=:allow )-";

      SqlQuery query(_sqlConn, sql);
      query.addParam("userid", sql::DataType::integer, userId);
      query.addParam("siteid", sql::DataType::integer, siteId);
      query.addParam("iadmin", sql::DataType::boolean, true);
      query.addParam("allow", sql::DataType::boolean, true);

      std::string result = query.executeScalar();
      if (util::isNumber(result))
         return (std::stoi(result) == 1);
      else
         return false;
   }

   int getAdminCount(long siteId)
   {
      std::string sql = R"-(
         SELECT COUNT(ur.id) FROM base_user_role ur
         JOIN base_roles r ON ur.role_id=r.id
         WHERE r.name=:name AND r.sysrole=:sysrole )-";

      SqlQuery query(_sqlConn, sql);
      query.addParam("name",    sql::DataType::varchar, std::string("role_admin"));
      query.addParam("sysrole", sql::DataType::boolean, true);

      std::string result = query.executeScalar();
      if (util::isNumber(result))
         return std::stoi(result);
      else
         return 0;
   }

   bool activate(long userId)
   {
      std::string sql = "UPDATE base_users SET enabled=:enabled WHERE id=:userId";
      SqlQuery query(_sqlConn, sql);
      query.addParam("enabled", sql::DataType::boolean, true);
      query.addParam("userId",  sql::DataType::integer, userId);

      return query.executeVoid();
   }

   bool changePassword(const std::string& userName,
      const std::string& currentPassword, const std::string& newPassword)
   {
      if (currentPassword.empty() || newPassword.empty())
         throw ValidationException("Password cannot be empty");

      if (currentPassword == newPassword)
         throw ValidationException("Current password and new password cannot be same");

      if (newPassword.length() < 5)
         throw ValidationException("Password must be at least 5 characters long");

      if (newPassword.length() > 100 )
         throw ValidationException("Password maximum length is 100 characters");

      bool passOk = checkPassword(userName, currentPassword);
      if (passOk)
      {
         // create hex encoded hash
         std::string passwordHash, passwordSalt;
         tbs::util::createSecureHash(newPassword, passwordHash, passwordSalt);

         std::string sql = "UPDATE base_users SET password_salt=:salt, password_hash=:hash WHERE user_name=:name";
         SqlQuery query(_sqlConn, sql);
         query.addParam("salt", sql::DataType::varbinary, passwordSalt, static_cast<long>(passwordSalt.length()/2));
         query.addParam("hash", sql::DataType::varbinary, passwordHash, static_cast<long>(passwordHash.length()/2));
         query.addParam("name", sql::DataType::varchar,   userName);

         return query.executeVoid();
      }
      else {
         throw ValidationException("Invalid user name or current password");
      }

      return false;
   }

   bool checkPassword(const std::string& userName, const std::string& password)
   {
      if (password.empty())
         throw ValidationException("Password cannot be empty");

      if (userName.empty())
         throw ValidationException("User name cannot be empty");

      auto user = getUserByName(userName);
      // check if userName exists
      if (user == nullptr)
         throw ValidationException("User name not found");

      // check if user locked
      if (user->enabled == false)
         throw ValidationException("Account disabled or not activated");

      // check if user can login
      if (user->allowLogin == false)
         throw ValidationException("Account is not allowed to logon");

      // check if user expired
      if (user->expired.timePoint() < DateTime().timePoint())
         throw ValidationException("Account has expired");

      // check if password is correct
      if (tbs::util::verifySecureHash(password, user->passwordHash, user->passwordSalt)) {
         return true;
      }

      return false;
   }

   bool resetPassword(const std::string& userName,
      const std::string& newPassword, const std::string& resetCode)
   {
      if (newPassword.empty())
         throw ValidationException("Password cannot be empty");

      if (userName.empty())
         throw ValidationException("User name cannot be empty");

      if (resetCode.empty())
         throw ValidationException("Reset code cannot be empty");

      if (newPassword.length() < 5)
         throw ValidationException("Password must be at least 5 characters long");

      if (newPassword.length() > 100 )
         throw ValidationException("Password maximum length is 100 characters");

      auto user = getUserByName(userName);
      // check if userName exists
      if (user == nullptr)
         throw ValidationException("User name not found");

      // get reset code HMAC
      auto securitySalt = Config::getOption<std::string>("securitySalt");
      auto hmacResetCode = crypt::hmacSHA(crypt::ShaType::SHA256, resetCode, securitySalt, false);

      // Get base_users_reset_password record from database
      std::string sql = R"-(
         SELECT * FROM base_users_reset_password
         WHERE user_id=:uid AND success=:success AND reset_code=:code )-";

      SqlQuery query(_sqlConn, sql);
      query.addParam("uid",     sql::DataType::integer, user->id);
      query.addParam("success", sql::DataType::boolean, false);
      query.addParam("code",    sql::DataType::varchar, hmacResetCode);

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();

      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         // we have base_users_reset_password
         // reset password only if reset code has not expired
         int64_t expiredTime = sqlResult->getLongLongValue("expired_time");
         if (expiredTime > DateTime().toUnixTimeMiliSeconds())
         {
            // create hex encoded password HMAC hash
            std::string passwordHash, passwordSalt;
            tbs::util::createSecureHash(newPassword, passwordHash, passwordSalt);

            // reset stored password in database
            std::string sql = "UPDATE base_users SET password_salt=:salt, password_hash=:hash WHERE user_name=:name";

            SqlQuery query0(_sqlConn, sql);
            query0.addParam("salt", sql::DataType::varbinary, passwordSalt, static_cast<long>(passwordSalt.length()/2));
            query0.addParam("hash", sql::DataType::varbinary, passwordHash, static_cast<long>(passwordHash.length()/2));
            query0.addParam("name", sql::DataType::varchar,   userName);

            if (query0.executeVoid())
            {
               long id_ = sqlResult->getLongValue("id");

               // Update password reset history
               std::string successTime = DateTime().isoDateTimeString();
               std::string sql ="UPDATE base_users_reset_password SET success=:success, success_time=:stime WHERE id=:id";

               SqlQuery query1(_sqlConn, sql);
               query1.addParam("success", sql::DataType::boolean,   true);
               query1.addParam("stime",   sql::DataType::timestamp, successTime );
               query1.addParam("id",      sql::DataType::integer,   id_);

               return query1.executeVoid();
            }
            else
               throw ValidationException("Reset password failed");
         }
         else
            throw ValidationException("Reset code has expired");
      }
      else
         throw ValidationException("Invalid password reset request data");

      return false;
   }

   bool forgotPassword(const std::string& userName, const std::string& emailAddress, DateTime& outExpiredTime)
   {
      if (userName.empty())
         throw ValidationException("User name cannot be empty");

      auto pUser = getUserByName(userName);
      // check if userName exists
      if (pUser == nullptr)
         throw ValidationException("User name not found");

      if (pUser->email != emailAddress)
         throw ValidationException("Email address incorrect");

      Logger::logI("[webapp] Sending password reset code email for API User {} to {}", userName, emailAddress);
      std::string resetCode = util::getRandomNumber(6);

      // get reset code HMAC
      auto securitySalt = Config::getOption<std::string>("securitySalt");
      auto hmacResetCode = crypt::hmacSHA(crypt::ShaType::SHA256, resetCode, securitySalt, false);

      std::string sql = "INSERT INTO base_users_reset_password (user_id, expired_time, reset_code) VALUES (:uid, :exp, :code)";

      SqlQuery query(_sqlConn, sql);
      query.addParam("uid",  sql::DataType::integer, pUser->id);
      query.addParam("exp",  sql::DataType::bigint,  outExpiredTime.toUnixTimeMiliSeconds());
      query.addParam("code", sql::DataType::varchar, hmacResetCode);
      if (query.executeVoid())
      {
         sendPasswordResetCodeEmail(pUser, outExpiredTime, resetCode);
         return true;
      }

      return false;
   }

   std::vector<dto::UserRoleDto> getUserRoleDto(long userId)
   {
      // get user's roles
      std::string sql = R"-(
         SELECT ur.id, ur.role_id AS r_id, ur.is_primary,
               r.name AS r_name, r.alias AS r_alias,
               r.enabled AS r_enabled, r.sysrole AS sysrole
         FROM base_user_role ur
         JOIN base_roles r ON ur.role_id = r.id
         WHERE ur.user_id = :uid )-";

      SqlQuery query(_sqlConn, sql);
      query.addParam("uid", sql::DataType::integer, userId);

      std::vector<dto::UserRoleDto> rolesVector;

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);

            dto::UserRoleDto urdto;
            urdto.id        = sqlResult->getLongValue("id");
            urdto.roleId    = sqlResult->getLongValue("r_id");
            urdto.isPrimary = sqlResult->getBoolValue("is_primary");
            urdto.name      = sqlResult->getStringValue("r_name");
            urdto.alias     = sqlResult->getStringValue("r_alias");
            urdto.enabled   = sqlResult->getBoolValue("r_enabled");
            urdto.sysRole   = sqlResult->getBoolValue("sysrole");

            rolesVector.push_back(urdto);
         }
      }

      return rolesVector;
   }

   std::vector<std::string> getRoleNames(long userId)
   {
      // get user's roles
      std::string sql = "SELECT ur.role_name FROM v_base_user_roles ur WHERE ur.user_id=:uid";
      SqlQuery query(_sqlConn, sql);
      query.addParam("uid", sql::DataType::integer, userId);

      std::vector<std::string> rolesVector;

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            rolesVector.push_back(sqlResult->getStringValue("role_name"));
         }
      }

      return rolesVector;
   }

   std::vector<long> getRoleIds(long userId)
   {
      // get user's roles
      std::string sql = "SELECT ur.role_id FROM base_user_roles ur WHERE ur.user_id=:uid";
      SqlQuery query(_sqlConn, sql);
      query.addParam("uid", sql::DataType::integer, userId);

      std::vector<long> rolesVector;

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            rolesVector.push_back(sqlResult->getLongValue("role_id"));
         }
      }

      return rolesVector;
   }

   std::vector<Site> getSites(long userId)
   {
      return {};
   }

   std::vector<long> getSiteIds(long userId)
   {
      // get user's sites
      std::string sql = "SELECT us.site_id FROM base_user_site us WHERE us.user_id=:uid";
      SqlQuery query(_sqlConn, sql);
      query.addParam("uid", sql::DataType::integer, userId);

      std::vector<long> siteVector;

      std::shared_ptr<SqlResult> sqlResult = query.executeResult();
      if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
      {
         for (int i = 0; i < sqlResult->totalRows(); i++)
         {
            sqlResult->locate(i);
            siteVector.push_back(sqlResult->getLongValue("site_id"));
         }
      }

      return siteVector;
   }

   bool userHasRole(long userId, const std::string& roleName)
   {
      std::string sql = "SELECT COUNT(id) FROM v_base_user_roles WHERE user_id=:uid AND role_name=:role";
      SqlQuery query(_sqlConn, sql);
      query.addParam("uid",  sql::DataType::integer, userId);
      query.addParam("role", sql::DataType::varchar, roleName);
      
      std::string result = query.executeScalar();
      if (util::isNumber(result))
         return ( std::stoi(result) > 0 );
      else
         return false;
   }

   bool userHasSite(long userId, long siteId)
   {
      std::string sql = "SELECT COUNT(id) FROM base_user_site WHERE user_id=:uid AND site_id=:sid";
      SqlQuery query(_sqlConn, sql);
      query.addParam("uid", sql::DataType::integer, userId);
      query.addParam("sid", sql::DataType::integer, siteId);
      
      std::string result = query.executeScalar();
      if (util::isNumber(result))
         return ( std::stoi(result) > 0 );
      else
         return false;
   }

   bool uniqueCodeExists(const std::string& uniqueCode)
   {
      std::string sql = "SELECT COUNT(id) FROM base_users WHERE unique_code=:code";
      SqlQuery query(_sqlConn, sql);
      query.addParam("code", sql::DataType::varchar, uniqueCode);

      std::string result = query.executeScalar();
      if (util::isNumber(result))
         return (std::stoi(result) == 1);
      else
         return false;
   }

   bool nikExists(const std::string& nik)
   {
      std::string sql = "SELECT COUNT(id) FROM base_users WHERE nik=:nik";
      SqlQuery query(_sqlConn, sql);
      query.addParam("nik", sql::DataType::varchar, nik);
      std::string result = query.executeScalar();
      if (util::isNumber(result))
         return (std::stoi(result) == 1);
      else
         return false;
   }

   void updateLastLogin(UserPtr user)
   {
      std::string sql = "UPDATE base_users SET last_login=:last WHERE id=:uid";
      SqlQuery query(_sqlConn, sql);
      query.addParam("last", sql::DataType::timestamp, DateTime().isoDateTimeString());
      query.addParam("uid",  sql::DataType::integer, user->id);
      bool success = query.executeVoid();
   }

   bool logUserLogon(AuthLogPtr authLogData)
   {
      std::string sql = R"-(
            INSERT INTO base_auth_log (logon_time, usr_id, usr_name, text_note, src_ip, src_host, src_mac, auth_type, site_id)
            VALUES (:ltime, :uid, :uname, :tnote, :ip, :host, :mac, :auth, :sid) )-";
      SqlQuery query(_sqlConn, sql);

      query.addParam("ltime", sql::DataType::timestamp, authLogData->logonTime.isoDateTimeString(true));
      query.addParam("uid",   sql::DataType::integer,   authLogData->usrId);
      query.addParam("uname", sql::DataType::varchar,   authLogData->usrName);
      query.addParam("tnote", sql::DataType::varchar,   authLogData->textNote);
      query.addParam("ip",    sql::DataType::varchar,   authLogData->srcIp);
      query.addParam("host",  sql::DataType::varchar,   authLogData->srcHost);
      query.addParam("mac",   sql::DataType::varchar,   authLogData->srcMac );
      query.addParam("auth",  sql::DataType::varchar,   authLogData->authType);
      query.addParam("sid",   sql::DataType::integer,   authLogData->siteId);

      bool success = query.executeVoid();
      return success;
   }

   bool noRegisteredUsersAndRoles()
   {
      std::string sql = R"-(
         SELECT (SELECT COUNT(id) FROM base_users) + 
            (SELECT COUNT(id) FROM base_roles) )-";
            
      SqlQuery query(_sqlConn, sql);
      std::string result = query.executeScalar();

      if ( util::isNumber(result)) {
         return  (std::stoi(result) == 0);
      }

      return false;
   }

   bool createDefaultRolesAndAddFirstUserAsAdmin(UserPtr pUser)
   {
      using namespace web::entity;

      std::vector<Role> vroles;
      vroles.push_back( { -1, "role_admin", "Admin",    true, true, DateTime(), DateTime()} );
      vroles.push_back( { -1, "role_user",  "User",     true, true, DateTime(), DateTime()} );
      vroles.push_back( { -1, "role_app",   "App User", true, true, DateTime(), DateTime()} );

      for (auto role: vroles)
      {
         std::string sql ="INSERT INTO base_roles (name, alias, enabled, sysrole) VALUES (:name, :alias, :enabled, :sysrole)";
         SqlQuery query(_sqlConn, sql);
         query.addParam("name",    sql::DataType::varchar, role.name);
         query.addParam("alias",   sql::DataType::varchar, role.alias);
         query.addParam("enabled", sql::DataType::boolean, role.enabled);
         query.addParam("sysrole", sql::DataType::boolean, role.sysRole);

         bool success = query.executeVoid();
         if (! success)
            return false;
      }

      std::string sql = "SELECT id FROM base_roles WHERE name=:name AND enabled=:enabled AND sysrole=:sysrole";
      SqlQuery query(_sqlConn, sql);
      query.addParam("name",    sql::DataType::varchar, std::string("role_admin"));
      query.addParam("enabled", sql::DataType::boolean, true);
      query.addParam("sysrole", sql::DataType::boolean, true);

      std::string result = query.executeScalar();
      if (util::isNumber(result))
      {
         long adminRoleId = std::stoi(result);
         UserRole userRole {-1, pUser->id, adminRoleId, true};

         std::string sql = "INSERT INTO base_user_role (user_id, role_id, is_primary) VALUES (:uid, :rid, :isp)";
         SqlQuery query(_sqlConn, sql);
         query.addParam("uid", sql::DataType::integer, userRole.userId);
         query.addParam("rid", sql::DataType::integer, userRole.roleId);
         query.addParam("isp", sql::DataType::boolean, userRole.isPrimary);
         bool success = query.executeVoid();
         if (!success)
            return false;
      }

      return true;
   }

   bool insertIntoBaseUserRole(UserPtr user, const std::string& groupName="")
   {
      return insertIntoBaseUserRole(user->id, groupName);
   }

   bool insertIntoBaseUserRole(long userId, const std::string& groupName="")
   {
      std::string role = groupName;
      bool isPrimary = false;
      if (role.empty())
      {
         role = "role_user";
         isPrimary = true;
      }

      if (userHasRole(userId,role))
         return true;

      std::string sql = "SELECT id FROM base_roles WHERE name=:name";
      SqlQuery query(_sqlConn, sql);
      query.addParam("name", sql::DataType::varchar, role);
      std::string result = query.executeScalar();
      if (util::isNumber(result))
      {
         long roleId = std::stoi(result);

         std::string sql = "INSERT INTO base_user_role (user_id, role_id, is_primary) VALUES (:uid, :rid, :isp)";
         SqlQuery query(_sqlConn, sql);
         query.addParam("uid", sql::DataType::integer, userId);
         query.addParam("rid", sql::DataType::integer, roleId);
         query.addParam("isp", sql::DataType::boolean, isPrimary);
         
         return query.executeVoid();
      }

      return false;
   }

   bool insertIntoBaseUserSite(UserPtr user, long selectedSiteId)
   {
      return insertIntoBaseUserSite(user->id, selectedSiteId);
   }

   bool insertIntoBaseUserSite(long userId, long selectedSiteId)
   {
      if (userHasSite(userId, selectedSiteId))
         return true;

      std::string sql = "SELECT id FROM base_sites WHERE id=:id";
      SqlQuery query(_sqlConn, sql);
      query.addParam("id", sql::DataType::integer, selectedSiteId);

      std::string result = query.executeScalar();
      if (util::isNumber(result))
      {
         long siteId = std::stoi(result);
         std::string sql = "INSERT INTO base_user_site (user_id, site_id, allow_login, is_admin) VALUES (:uid, :sid, :allow, :admin)";
         SqlQuery query(_sqlConn, sql);

         query.addParam("uid",   sql::DataType::integer, userId);
         query.addParam("sid",   sql::DataType::integer, selectedSiteId);
         query.addParam("allow", sql::DataType::boolean, true);
         query.addParam("admin", sql::DataType::boolean, false);

         return  query.executeVoid();
      }
      return false;
   }

   void sendPasswordResetCodeEmail(UserPtr user, DateTime expired, const std::string& resetCode)
   {
      Logger::logD("AuthDbRepo sendPasswordResetCodeEmail, reset code={}", resetCode);
   }

   void sendActivationTokenEmail(long userId, const std::string& userName, const std::string& securitySalt)
   {
      std::string activationToken = createActivationToken(userId, userName, securitySalt);
      std::string xx = activationToken;
   }

   std::string createActivationToken(long userId, const std::string& userName, const std::string& securitySalt)
   {
      auto expiredTime = DateTime();
      expiredTime.timePoint() += tbsdate::days{1};

      int64_t expiredTimeEpoch = DateTime().toUnixTimeMiliSeconds();
      std::string activationMessage = tbsfmt::format("{};{};{}", userId, userName, expiredTimeEpoch);
      auto token = crypt::passwordEncrypt(activationMessage,securitySalt);

      return token;
   }

   bool verifyActivationToken(const std::string& token, const std::string& securitySalt)
   {
      try
      {
         auto activationMessage = crypt::passwordDecrypt(token, securitySalt);
         auto payload           = util::split(activationMessage,';');
         std::string userId     = payload[0];
         std::string userName   = payload[1];
         std::string expiredStr = payload[2];

         if (!util::isNumber(userId))
            throw ValidationException("Invalid activation token format");

         if (!util::isNumber(expiredStr))
            throw ValidationException("Invalid activation token format");

         long uid         = std::stol(userId);
         int64_t expired  = std::stoll(expiredStr);
         auto currentTime = DateTime();

         if (currentTime.toUnixTimeMiliSeconds() > expired)
            throw ValidationException("Activation token has expired");

         auto pUser = getUserById(uid);
         if (pUser!=nullptr && pUser->userName == userName)
         {
            if (pUser->enabled)
               throw ValidationException("Already activated");

            return activate(pUser->id);
         }
         else
            throw ValidationException("User not found");
      }
      catch(const ValidationException& e)
      {
         throw e;
      }
      catch(const std::exception& e)
      {
         throw AppException(e.what());
      }

      return false;

   }

   std::string generateAccessToken(UserPtr pUser)
   {
      auto appOption = Config::getOption<web::conf::Webapp>("webapp");
      auto wsOption  = appOption.webService;

      std::string accessTokenSecret  = wsOption.authJwtSecret;
      std::string jwtIssuer          = wsOption.authJwtIssuer;
      int jwtExpTimeSpanMinutes      = wsOption.authJwtExpireTimeSpanMinutes;

      std::vector<std::string> roleNames = getRoleNames(pUser->id);
      std::vector<long> siteIds          = getSiteIds(pUser->id);

      Json jRole(roleNames);
      Json jSiteId(siteIds);

      std::string fullName = util::trim(pUser->firstName) + " " + util::trim(pUser->lastName);

      const auto notUtc = std::chrono::system_clock::now();
      const auto token = jwt::create()
            .set_type("JWT")
            .set_issuer(jwtIssuer)
            .set_issued_at(notUtc)
            .set_not_before(notUtc)
            .set_expires_at(notUtc + std::chrono::minutes{ jwtExpTimeSpanMinutes })
            .set_payload_claim("user_id",          std::to_string(pUser->id))
            .set_payload_claim("user_uuid",        pUser->uuid)
            .set_payload_claim("user_name",        pUser->userName)
            .set_payload_claim("given_name",       pUser->firstName)
            .set_payload_claim("family_name",      pUser->lastName)
            .set_payload_claim("full_name",        fullName)
            .set_payload_claim("image",            pUser->image)
            .set_payload_claim("selected_site_id", std::to_string(pUser->selectedSiteId))
            .set_payload_claim("selected_lang_id", pUser->selectedLangId)
            .set_payload_claim("role",             jRole)
            .set_payload_claim("site_id",          jSiteId)
            .set_payload_claim("unique_code",      "")
            .set_payload_claim("birthdate",        pUser->birthDate.isoDateString())
            .set_payload_claim("email",            pUser->email)
            .set_payload_claim("phone",            pUser->phone)
            .set_payload_claim("nik",              pUser->nik)

            .sign(jwt::algorithm::hs256{ accessTokenSecret });

      return token;
   }

   std::string generateRefreshToken(UserPtr pUser)
   {
      auto appOption = Config::getOption<web::conf::Webapp>("webapp");
      auto wsOption  = appOption.webService;

      std::string refreshTokenSecret = wsOption.authJwtSecretRefresh;
      std::string jwtIssuer          = wsOption.authJwtIssuer;
      int refreshExpTimeSpanMinutes  = wsOption.authJwtRefreshExpireTimeSpanMinutes;

      const auto notUtc = std::chrono::system_clock::now();
      const auto refreshToken = jwt::create()
            .set_type("JWT")
            .set_issuer(jwtIssuer)
            .set_issued_at(notUtc)
            .set_not_before(notUtc)
            .set_expires_at(notUtc + std::chrono::minutes{ refreshExpTimeSpanMinutes })
            .set_payload_claim("user_uuid", pUser->uuid)
            .set_payload_claim("user_name", pUser->userName)
            .sign(jwt::algorithm::hs256{ refreshTokenSecret });

      return refreshToken;
   }

   std::string validateUniqueCode(const std::string& uniqueCode, const std::string& fullName,
      const std::string& birthDate, const std::string& nik)
   {
      return {};
   }


   // -------------------------------------------------------

   AclPtr getAcl(long ugid, const std::string& ugType, long menuId)
   {
      try
      {
         std::string sql = R"-(
               SELECT * FROM base_acl
               WHERE ug_id=:ugid AND ug_type=:type AND menu_id=:menuid )-";

         SqlQuery query(_sqlConn, sql);
         query.addParam("ugid",   sql::DataType::integer, ugid);
         query.addParam("type",   sql::DataType::varchar, ugType);
         query.addParam("menuid", sql::DataType::integer, menuId);

         std::shared_ptr<SqlResult> sqlResult = query.executeResult();
         if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
         {
            auto acl = std::make_shared<Acl>();
            acl->id      = sqlResult->getLongValue("id");
            acl->ugId    = sqlResult->getLongValue("ug_id");
            acl->ugType  = sqlResult->getStringValue("ug_type");
            acl->menuId  = sqlResult->getLongValue("menu_id");
            acl->aAll    = sqlResult->getBoolValue("a_all");
            acl->aAdd    = sqlResult->getBoolValue("a_add");
            acl->aDelete = sqlResult->getBoolValue("a_delete");
            acl->aUpdate = sqlResult->getBoolValue("a_update");
            acl->aPrint  = sqlResult->getBoolValue("a_print");
            acl->aIndex  = sqlResult->getBoolValue("a_index");
            acl->aOther  = sqlResult->getStringValue("a_other");

            return acl;
         }
         else
            Logger::logT("[webapp] AclSevice getAcl, no acl data found for ug_id: {}, ug_type: {}, menu_id: {}", ugid, ugType, menuId);
      }
      catch(const std::exception & ex)
      {
         Logger::logE("[webapp] AclSevice getAcl, {}", ex.what());
      }

      return nullptr;
   }

   bool updateAcl(const web::entity::Acl& acl)
   {
      std::string sql = "SELECT COUNT(id) FROM base_acl WHERE ug_id=:ugid AND ug_type=:ugtype AND menu_id=:menuid";
      SqlQuery query(_sqlConn, sql);
      query.addParam("ugid",   sql::DataType::integer, acl.ugId);
      query.addParam("ugtype", sql::DataType::varchar, acl.ugType);
      query.addParam("menuid", sql::DataType::integer, acl.menuId);

      std::string total = query.executeScalar();
      if (util::isNumber(total) && std::stol(total) == 0)
      {
         sql = R"-(
            UPDATE base_acl
            SET a_all=:a_all, a_add=:a_add, a_delete=:a_delete, a_update=:a_update, a_print=:a_print, a_index=:a_index, a_other=:a_other
            WHERE ug_id=:ug_id AND ug_type=:ug_type AND menu_id=:menu_id)-";

         SqlQuery query(_sqlConn, sql);

         query.addParam("a_all",    sql::DataType::boolean, acl.aAll);
         query.addParam("a_add",    sql::DataType::boolean, acl.aAdd);
         query.addParam("a_delete", sql::DataType::boolean, acl.aDelete);
         query.addParam("a_update", sql::DataType::boolean, acl.aUpdate);
         query.addParam("a_print",  sql::DataType::boolean, acl.aPrint);
         query.addParam("a_index",  sql::DataType::boolean, acl.aIndex);
         query.addParam("a_other",  sql::DataType::varchar, acl.aOther);

         query.addParam("ug_id",    sql::DataType::integer, acl.ugId);
         query.addParam("ug_type",  sql::DataType::varchar, acl.ugType);
         query.addParam("menu_id",  sql::DataType::integer, acl.menuId);

         bool success = query.executeVoid();
         return success;
      }

      return false;
   }

   bool canAccessByMenuNameFullCheck(long userId, long siteId,
      const std::string& menuName, const std::string& method = "all")
   {
      try
      {
         std::string sql = "SELECT ur.role_id FROM base_user_role ur WHERE ur.user_id=:uid";
         SqlQuery query(_sqlConn, sql);
         query.addParam("uid", sql::DataType::integer, userId);

         std::shared_ptr<SqlResult> sqlResult = query.executeResult();
         if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
         {
            for (int i = 0; i < sqlResult->totalRows(); i++)
            {
               sqlResult->locate(i);
               long roleId = sqlResult->getLongValue("role_id");
               bool roleOk = canAccessByMenuName(roleId, "G", menuName, method);
               if (roleOk)
                  return roleOk;
            }
         }

         // user's role has no access, check user's privilege
         bool userOk = canAccessByMenuName(userId, "U", menuName, method);
         return userOk;
      }
      catch(std::exception& ex)
      {
         Logger::logE("[webapp] AclSevice canAccessByMenuNameFullCheck, {}", ex.what());
      }

      return false;
   }

   bool canAccessByRequestPathFullCheck(long userId, long siteId,
      const std::string& requestPath, const std::string& method = "all")
   {
      try
      {
         std::string sql = "SELECT ur.role_id FROM base_user_role ur WHERE ur.user_id=:uid";
         SqlQuery query(_sqlConn, sql);
         query.addParam("uid", sql::DataType::integer, userId);

         std::shared_ptr<SqlResult> sqlResult = query.executeResult();
         if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
         {
            for (int i = 0; i < sqlResult->totalRows(); i++)
            {
               sqlResult->locate(i);

               long roleId = sqlResult->getLongValue("role_id");
               bool roleOk = canAccessByRequestPath(roleId, "G", requestPath, method);

               if (roleOk)
                  return roleOk;
            }
         }

         // user's role has no access, check user's privilege
         bool userOk = canAccessByRequestPath(userId, "U", requestPath, method);
         return userOk;
      }
      catch(std::exception& ex)
      {
         Logger::logE("[webapp] AclSevice canAccessByRequestPathFullCheck, {}", ex.what());
      }

      return false;
   }

   bool canAccessByMenuId(long ugid, const std::string& ugType,
      long  menuId, const std::string& method = "all")
   {
      auto acl = getAcl(ugid, ugType, menuId);
      if (acl != nullptr)
      {
         if (acl->aAll)
            return true;
         else if (acl->aIndex  && method == "index")
            return true;
         else if (acl->aAdd    && method == "add")
            return true;
         else if (acl->aUpdate && method == "update")
            return true;
         else if (acl->aDelete && method == "delete")
            return true;
         else if (acl->aPrint  && method == "print")
            return true;
         else if (acl->aOther.length() > 0)
         {
            std::vector<std::string> methodList = util::split(acl->aOther, ',');

            if ( util::findPositionInVector(methodList, method) != tbs::NOT_FOUND ) {
               return true;
            }
         }
      }

      return false;
   }

   bool canAccessByMenuName(long  ugid, const std::string& ugType,
      const std::string& menuName, const std::string& method = "all")
   {
      try
      {
         std::string sql = "SELECT id FROM base_menu m WHERE m.name=:name";
         SqlQuery query(_sqlConn, sql);
         query.addParam("name", sql::DataType::varchar, menuName);

         std::string idStr = query.executeScalar();
         if(util::isNumber(idStr))
         {
            long menuId = std::stol(idStr);
            return canAccessByMenuId(ugid, ugType, menuId, method);
         }
      }
      catch(const std::exception& ex)
      {
         Logger::logE("[webapp] AclSevice canAccessByMenuName, {}", ex.what());
      }

      return false;
   }

   bool canAccessByRequestPath(long ugid, const std::string& ugType,
      const std::string& requestPath, const std::string& method = "all")
   {
      try
      {
         std::string sql = "SELECT id FROM base_menu m WHERE m.pageurl=:path";
         SqlQuery query(_sqlConn, sql);
         query.addParam("path", sql::DataType::varchar, requestPath);

         std::string idStr = query.executeScalar();
         if (util::isNumber(idStr))
         {
            long menuId = std::stol(idStr);
            return canAccessByMenuId(ugid, ugType, menuId, method);
         }
      }
      catch(const std::exception& ex)
      {
         Logger::logE("[webapp] AclSevice canAccessByRequestPath, {}", ex.what());
      }

      return false;
   }

   std::vector<std::string> getMenuMethods(long menuId)
   {
      try
      {
         std::string sql = "SELECT methods FROM base_menu m WHERE m.id=:id";
         SqlQuery query(_sqlConn, sql);
         query.addParam("id", sql::DataType::varchar, menuId);

         std::string methods = query.executeScalar();
         auto menuMethods = util::split(methods, ',');
         return menuMethods;
      }
      catch (const std::exception& ex)
      {
         Logger::logE("[webapp] AclDbRepo getMenuMethods, {}", ex.what());
      }

      return {};
   }

   std::vector<std::tuple<long, std::string, std::string>> getAclItemsForUserOrRole()
   {
      try
      {
         std::string sql = R"-( SELECT mnu.id AS menu_id, grp.name grp_name, mnu.name AS menu_name
                           FROM base_menu mnu
                           JOIN v_base_menu_group grp ON mnu.group_id = grp.id
                           JOIN v_base_menu_type typ ON mnu.type_id = typ.id
                           ORDER BY mnu.name ASC )-";
         
         SqlQuery query(_sqlConn, sql);

         std::vector<std::tuple<long, std::string, std::string>> aclVector;
         std::shared_ptr<SqlResult> sqlResult = query.executeResult();
         if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
         {
            for (int i = 0; i < sqlResult->totalRows(); i++)
            {
               long        mnuId   = sqlResult->getLongValue("menu_id");
               std::string grpName = sqlResult->getStringValue("grp_name");
               std::string mnuName = sqlResult->getStringValue("menu_name");
               aclVector.push_back(std::make_tuple(mnuId, grpName, mnuName));
            }
         }

         return aclVector;
      }
      catch(const std::exception& ex)
      {
         Logger::logE("[webapp] AclDbRepo getAclItemsForUserOrRole, {}", ex.what());
      }

      return {};
   }

   /**
      @param ugId    : user or group id
      @param ugType  : 'U' or 'G'
   */
  std::vector<std::tuple<long, std::string>> getAclItemsForUserOrRole(long ugId, const std::string& ugType)
  {
     try
     {
        std::string sql = R"-( SELECT a.menu_id, m.name AS menu_name
                            FROM base_acl a
                            JOIN base_menu m ON a.menu_id = m.id
                            WHERE a.ug_id=:ugid AND a.ug_type=:ugtype
                            ORDER BY a.menu_id ASC )-";
        SqlQuery query(_sqlConn, sql);
        query.addParam("ugid",   sql::DataType::integer, ugId);
        query.addParam("ugtype", sql::DataType::varchar, ugType);

        std::vector<std::tuple<long, std::string>> aclVector;
        std::shared_ptr<SqlResult> sqlResult = query.executeResult();
        if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
        {
           for (int i = 0; i < sqlResult->totalRows(); i++)
           {
              long        menuId   = sqlResult->getLongValue("menu_id");
              std::string menuName = sqlResult->getStringValue("menu_name");
              aclVector.push_back(std::make_tuple (menuId, menuName));
           }
        }

        return aclVector;
     }
     catch(const std::exception& ex)
     {
        Logger::logE("[webapp] AclDbRepo getAclItemsForUserOrRole, {}", ex.what());
     }

     return {};
  }

   bool updateAclForUserOrGroup(const std::vector<long>& newMenuIds,
      long ugId,const std::string& ugType)
   {
      if (newMenuIds.size() == 0) {
         return false;
      }

      // get current acl menu
      auto currentAcl = getAclItemsForUserOrRole(ugId, ugType);

      std::string csvNewMenuIds = util::vetorToCsvString(newMenuIds);
      if (csvNewMenuIds.empty()) {
         return false;
      }

     try
     {
         // remove current acl not listed in newMenuIds
         std::string sql = 
           tbsfmt::format("DELETE FROM base_acl WHERE ug_id=:ugid AND ug_type=:ugtype AND menu_id NOT IN ( {} )", csvNewMenuIds);

         SqlQuery query(_sqlConn, sql);
         query.addParam("ugid",   sql::DataType::integer, ugId);
         query.addParam("ugtype", sql::DataType::varchar, ugType);

         bool success = query.executeVoid();
         if (!success) {
            return false;
         }

         // Then insert new acl menu, not exist in newMenuIds
         for (auto menuId: newMenuIds)
         {
            std::string sql = "SELECT COUNT(id) FROM base_acl WHERE ug_id=:ugid AND ug_type=:ugtype AND menu_id=:menuid";
            SqlQuery query(_sqlConn, sql);
            query.addParam("ugid",   sql::DataType::integer, ugId);
            query.addParam("ugtype", sql::DataType::varchar, ugType);
            query.addParam("menuid", sql::DataType::integer, menuId);

            std::string total = query.executeScalar();
            if ( util::isNumber(total) &&  std::stol(total) == 0)
            {
               std::string sql = "INSERT INTO base_acl (ug_id, ug_type, menu_id, a_all) VALUES (:ugid, :ugtype, :menuid, :aall)";
               SqlQuery query3(_sqlConn, sql);

               query3.addParam("ugid",   sql::DataType::integer, ugId);
               query3.addParam("ugtype", sql::DataType::varchar, ugType);
               query3.addParam("menuid", sql::DataType::integer, menuId);
               query3.addParam("aall",   sql::DataType::boolean, true);

               bool success = query3.executeVoid();
               if (success) {
                  Logger::logI("[webapp] AclDbRepo, Add new Acl MenuId {}", menuId);
               }
            }
         }

         return true;
      }
      catch(const std::exception& ex)
      {
         Logger::logE("[webapp] AclDbRepo removeAllAclForUserOrGroup, {}", ex.what());
      }

      return false;
  }

   bool removeAllAclForUserOrGroup(long ugId, const std::string& ugType)
   {
      try
      {
         std::string sql = "DELETE FROM base_acl WHERE ug_id=:ugid AND ug_type=:ugtype";
         SqlQuery query(_sqlConn, sql);
         query.addParam("ugid",   sql::DataType::integer, ugId);
         query.addParam("ugtype", sql::DataType::varchar, ugType);

         bool success = query.executeVoid();
         return success;
      }
      catch (const std::exception& ex)
      {
         Logger::logE("[webapp] AclDbRepo removeAllAclForUserOrGroup, {}", ex.what());
      }

      return false;
   }

   std::vector<std::tuple<long, std::string>> getItemGroupList()
   {
      try
      {
         std::vector<std::tuple<long, std::string>> nameVector;

         std::string sql("SELECT item_id AS id, item_code AS code FROM base_class_code WHERE item_class='MENU_GROUP'");
         SqlQuery query(_sqlConn, sql);
         std::shared_ptr<SqlResult> sqlResult = query.executeResult();
         if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
         {
            for (int i = 0; i < sqlResult->totalRows(); i++)
            {
               sqlResult->locate(i);

               long        itemId   = sqlResult->getLongValue("id");
               std::string itemCode = sqlResult->getStringValue("code");
               nameVector.push_back(std::make_tuple (itemId, itemCode));
            }
         }

         return nameVector;
      }
      catch(const std::exception& ex)
      {
         Logger::logE("[webapp] AclDbRepo getItemGroupList, {}", ex.what());
      }

      return {};
   }

   std::vector<std::tuple<long, std::string>> getItemTypeList()
   {
      try
      {
         std::vector<std::tuple<long, std::string>> nameVector;

         std::string sql("SELECT item_id AS id, item_code AS code FROM base_class_code WHERE item_class='MENU_TYPE'");
         SqlQuery query(_sqlConn, sql);
         std::shared_ptr<SqlResult> sqlResult = query.executeResult();
         if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
         {
            for (int i = 0; i < sqlResult->totalRows(); i++)
            {
               sqlResult->locate(i);

               long        itemId   = sqlResult->getLongValue("id");
               std::string itemCode = sqlResult->getStringValue("code");
               nameVector.push_back(std::make_tuple (itemId, itemCode));
            }
         }

         return nameVector;
      }
      catch(const std::exception& ex)
      {
         Logger::logE("[webapp] AclDbRepo getItemTypeList, {}", ex.what());
      }

      return {};
   }

   std::vector<std::tuple<long, std::string>> getUserList()
   {
      try
      {
         std::vector<std::tuple<long, std::string>> nameVector;

         std::string sql("SELECT ug_id AS id, ug_name AS name FROM v_base_acl WHERE ug_type='User' ORDER BY id ASC");
         SqlQuery query(_sqlConn, sql);
         std::shared_ptr<SqlResult> sqlResult = query.executeResult();
         if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
         {
            for (int i = 0; i < sqlResult->totalRows(); i++)
            {
               sqlResult->locate(i);

               long        ugId   = sqlResult->getLongValue("id");
               std::string ugName = sqlResult->getStringValue("name");
               nameVector.push_back(std::make_tuple (ugId, ugName));
            }
         }

         return nameVector;
      }
      catch(const std::exception& ex)
      {
         Logger::logE("[webapp] AclDbRepo getUserList, {}", ex.what());
      }

      return {};
   }

   std::vector<std::tuple<long, std::string>> getRoleList()
   {
      try
      {
         std::vector<std::tuple<long, std::string>> rolesVector;
         
         std::string sql("SELECT ug_id AS id, ug_name AS name FROM v_base_acl WHERE ug_type='Group' ORDER BY id ASC");
         SqlQuery query(_sqlConn, sql);
         std::shared_ptr<SqlResult> sqlResult = query.executeResult();
         if (sqlResult != nullptr && sqlResult->isValid() && sqlResult->totalRows() > 0)
         {
            for (int i = 0; i < sqlResult->totalRows(); i++)
            {
               sqlResult->locate(i);
               long        ugId   = sqlResult->getLongValue("id");
               std::string ugName = sqlResult->getStringValue("name");
               rolesVector.push_back(std::make_tuple (ugId, ugName));
            }
         }

         return rolesVector;
      }
      catch(const std::exception& ex)
      {
         Logger::logE("[webapp] AclDbRepo getRoleList, {}", ex.what());
      }

      return {};
   }

protected:

   UserPtr getUser(std::shared_ptr<SqlResult> pResult)
   {
      if (pResult->isValid() && pResult->totalRows() > 0)
      {
         try
         {
            auto user = std::make_shared<User>();

            user->id           = pResult->getLongValue("id");
            user->uuid         = pResult->getStringValue("uuid");
            user->userName     = pResult->getStringValue("user_name");
            user->firstName    = pResult->getStringValue("first_name");
            user->lastName     = pResult->getStringValue("last_name");
            user->email        = pResult->getStringValue("email");
            user->image        = pResult->getStringValue("image");
            user->enabled      = pResult->getBoolValue("enabled");
            user->passwordSalt = pResult->getStringValue("password_salt");
            user->passwordHash = pResult->getStringValue("password_hash");
            user->allowLogin   = pResult->getBoolValue("allow_login");
            user->created      = pResult->getDateTimeValue("created");
            user->expired      = pResult->getDateTimeValue("expired");
            user->lastLogin    = pResult->getDateTimeValue("last_login");
            user->uniqueCode   = pResult->getStringValue("unique_code");
            user->birthDate    = pResult->getDateTimeValue("birth_date");
            user->phone        = pResult->getStringValue("phone");
            user->gender       = pResult->getStringValue("gender");
            user->address      = pResult->getStringValue("address");
            user->nik          = pResult->getStringValue("nik");

            return user;
         }
         catch(const std::exception& ex)
         {
            Logger::logE("[webapp] AuthDbRepo getUser, {}", ex.what());
         }
      }

      return nullptr;
   }

};

/** @}*/

} // namespace web
} // namespace tbs
