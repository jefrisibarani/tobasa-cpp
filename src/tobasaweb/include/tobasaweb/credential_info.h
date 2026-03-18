#pragma once

#include "tobasaweb/entity/user.h"

namespace tbs {
namespace web {

/** \addtogroup WEB
 * @{
 */


/**
 * \brief Contains identification information for a user.
 */
struct Identity
{
   static constexpr long invalidUser = -1;
   long            id             { invalidUser };
   int             selectedSiteId { 0 };
   std::string     selectedLangId {};
   entity::UserPtr pUser;
};

using IdentityPtr = std::shared_ptr<Identity>;

/**
 * \brief Contains information about a user after an authentication attempt.
 */
struct AuthResult
{
   bool        credentialsProvided { false };
   Identity    identity;
   std::string errorMessage;
   bool        credentialsValid    { false };
   
   /**
    * @brief Represents the expiration time of an authenticated user in UTC seconds.
    */   
   long long   expirationTime {0} ;

   bool        loggedIn()  { return credentialsProvided && credentialsValid; }
};

/** @}*/

} // namespace web
} // namespace tbs