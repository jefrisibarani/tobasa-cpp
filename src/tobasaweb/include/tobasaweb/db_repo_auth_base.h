#pragma once

#include <vector>
#include <tobasasql/sql_service_base.h>
#include "tobasaweb/entity/user.h"
#include "tobasaweb/entity/site.h"
#include "tobasaweb/entity/role.h"
#include "tobasaweb/dto/login_user.h"
#include "tobasaweb/entity/acl.h"
#include "tobasaweb/dto/role_dto.h"
#include "tobasaweb/entity/auth_log.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */

using namespace tbs::web::entity;

/** 
 * AuthDbRepoBase
 */
class AuthDbRepoBase : public sql::SqlServiceBase
{
public:
   AuthDbRepoBase() = default;
   virtual ~AuthDbRepoBase() = default;

   virtual UserPtr authenticate(const std::string& userName, const std::string& password, const int siteId=1) = 0;

   virtual UserPtr authenticate(const web::dto::LoginUser& userDto) = 0;

   /// @brief Add user into database
   /// @note this method use SQL Transaction
   virtual UserPtr enrollNewUSer(const User& user, const std::string& password, bool checkAll=true) = 0;

   virtual UserPtr updateUserProfile(const User& user) = 0;

   virtual std::vector<entity::UserDto> getAllUserDto() = 0;

   virtual std::vector<entity::UserPtr> getAllUserPtr() = 0;

   virtual UserPtr getUserById(long id) = 0;

   virtual UserPtr getUserByUuid(const std::string& uuid) = 0;

   virtual UserPtr getUserByName(const std::string& name) = 0;

   virtual bool create(const User& user, const std::string& password) = 0;

   virtual bool update(UserPtr pUser) = 0;

   virtual bool remove(long id) = 0;

   virtual bool exists(const std::string& name) = 0;

   virtual bool exists(long id) = 0;

   virtual bool existsByUniqueCode(const std::string& name) = 0;

   virtual bool canLogin(long id) = 0;

   virtual bool canLoginToSite(long userId, long siteId = 1) = 0;

   virtual bool isSuperUser(long id) = 0;

   virtual bool isSiteAdministrator(long userId, long siteId = 1) = 0;

   virtual int getAdminCount(long siteId = 1) = 0;

   virtual bool activate(long userId) = 0;

   virtual bool changePassword(const std::string& userName,
      const std::string& currentPassword, const std::string& newPassword) = 0;

   virtual bool checkPassword(const std::string& userName, const std::string& password) = 0;

   virtual bool resetPassword(const std::string& userName,
      const std::string& newPassword, const std::string& resetCode) = 0;

   virtual bool forgotPassword(const std::string& userName, const std::string& emailAddress, DateTime& outExpiredTime) = 0;

   virtual std::vector<dto::UserRoleDto> getUserRoleDto(long userId) = 0;

   virtual std::vector<std::string> getRoleNames(long userId) = 0;

   virtual std::vector<long> getRoleIds(long userId) = 0;

   virtual std::vector<Site> getSites(long userId) = 0;

   virtual std::vector<long> getSiteIds(long userId) = 0;

   virtual bool userHasRole(long userId, const std::string& roleName) = 0;

   virtual bool userHasSite(long userId, long siteId) = 0;

   virtual bool uniqueCodeExists(const std::string& uniqueCode) = 0;

   virtual bool nikExists(const std::string& nik) = 0;

   virtual void updateLastLogin(UserPtr user) = 0;

   virtual bool logUserLogon(AuthLogPtr authLogData) = 0;

   virtual bool noRegisteredUsersAndRoles() = 0;

   virtual bool createDefaultRolesAndAddFirstUserAsAdmin(UserPtr pUser) = 0;

   virtual bool insertIntoBaseUserRole(UserPtr user, const std::string& groupName = "") = 0;

   virtual bool insertIntoBaseUserRole(long userId, const std::string& groupName = "") = 0;

   virtual bool insertIntoBaseUserSite(UserPtr user, long selectedSiteId) = 0;

   virtual bool insertIntoBaseUserSite(long userId, long selectedSiteId) = 0;

   virtual void sendPasswordResetCodeEmail(UserPtr user, DateTime expired, const std::string& resetCode) = 0;

   virtual void sendActivationTokenEmail(long userId, const std::string& userName, const std::string& securitySalt) = 0;

   virtual std::string createActivationToken(long userId, const std::string& userName, const std::string& securitySalt) = 0;

   virtual bool verifyActivationToken(const std::string& token, const std::string& securitySalt) = 0;

   virtual std::string generateAccessToken(UserPtr pUser) = 0;

   virtual std::string generateRefreshToken(UserPtr pUser) = 0;

   virtual std::string validateUniqueCode(const std::string& uniqueCode, const std::string& fullName,
      const std::string& birthDate, const std::string& nik) = 0;


   // -------------------------------------------------------
   // A C L
   // -------------------------------------------------------
   
   virtual AclPtr getAcl(long ugid, const std::string& ugType, long menuId) = 0;

   virtual bool updateAcl(const web::entity::Acl& acl) = 0 ;

   virtual bool canAccessByMenuNameFullCheck(long userId, long siteId,
      const std::string& menuName, const std::string& method = "all") = 0;

   virtual bool canAccessByRequestPathFullCheck(long userId, long siteId,
      const std::string& requestPath, const std::string& method = "all") = 0;

   virtual bool canAccessByMenuId(long ugid, const std::string& ugType,
      long  menuId, const std::string& method = "all") = 0;

   virtual bool canAccessByMenuName(long  ugid, const std::string& ugType,
      const std::string& menuName, const std::string& method = "all") = 0;

   virtual bool canAccessByRequestPath(long ugid, const std::string& ugType,
      const std::string& requestPath, const std::string& method = "all") = 0;

   virtual std::vector<std::string> getMenuMethods(long menuId) = 0;

   virtual std::vector<std::tuple<long, std::string, std::string>> getAclItemsForUserOrRole() = 0;

   /**
      @param ugId    : user or group id
      @param ugType  : 'U' or 'G'
   */
   virtual std::vector<std::tuple<long, std::string>> getAclItemsForUserOrRole(long ugId, const std::string& ugType) = 0;

   virtual bool updateAclForUserOrGroup(const std::vector<long>& newMenuIds,
      long ugId,const std::string& ugType) = 0;

   virtual bool removeAllAclForUserOrGroup(long ugId, const std::string& ugType) = 0;

   virtual std::vector<std::tuple<long, std::string>> getItemGroupList() = 0;

   virtual std::vector<std::tuple<long, std::string>> getItemTypeList() = 0;

   virtual std::vector<std::tuple<long, std::string>> getUserList() = 0;

   virtual std::vector<std::tuple<long, std::string>> getRoleList()  = 0;
};

using AuthDbRepoPtr = std::shared_ptr<AuthDbRepoBase>;

/** @}*/

} // namespace web
} // namespace tbs