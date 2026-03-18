    <script type="text/javascript"> <!-- JS SITE -->
      const REGEX_EMAIL = '([a-z0-9!#$%&\'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&\'*+/=?^_`{|}~-]+)*@@' + 
                          '(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?)';


      TBS.getBaseUrl = function() {
        const port = window.location.port ? `:${window.location.port}` : '';
        const baseUrl = `${window.location.protocol}//${window.location.hostname}${port}`;
        return baseUrl;
      }

      TBS.urlLogout = function() {
        return `${TBS.baseUrl}/logout?redirect=false`;
      }

      TBS.urlGetRefreshToken = function() {
      if (TBS.runInTbsServer || TBS.refreshToTbsServer) {
          // get refresh token from TBS Web server
          return `${TBS.baseUrl}/api/refresh_auth_token`;
        } 
        else {
          // get refresh token from MCU Web server
          return `${TBS.baseUrlMCU}/api/tbs_refresh_auth_token`;
        }
      } 

      TBS.urlChatEndpoint = function() {
        return `${TBS.baseUrl}/chat_app_socket`;
      }

      TBS.urlDownloadAttachment = function(attachmentId, filename) {
        return `${TBS.baseUrl}/api/chat/get_file/${attachmentId}/${filename}`;
      }

      TBS.urlChatUploadFile = function() {
        return `${TBS.baseUrl}/api/chat/upload_file`;
      }

      TBS.log = function(message) {
        if (TBS.debug)
          console.log("DBG : " + message);
      }

      TBS.isLoggedIn = function() {
        let userUuid = TBS.getCookie("tbs_user_uuid");
        let userName = TBS.getCookie("tbs_user_name");
        let siteId   = TBS.getCookie("tbs_selected_site_id");
        let langId   = TBS.getCookie("tbs_selected_lang_id");
        
        if (userUuid && userName && siteId && langId) {
          if (userUuid.length > 0 && userName.length > 0 && siteId.length > 0 && langId.length > 0) {
            return true;
          }
        }

        bootbox.confirm({
                title: "Error",
                message: "Not logged in",
                closeButton: true,
                buttons: {
                    confirm: {label: '<i class="fas fa-check"></i> Re-login'}
                },
                callback: function (result) {
                    if (result)
                      TBS.logout();
                }
              });

        return false;
      }

      TBS.clearAuthCookie = function() {
        // //let sessioncookie = TBS.getCookie("tbs_session");
        // const cookies = document.cookie.split("; ");
        // for (const cookie of cookies) {
        //   const eqPos = cookie.indexOf("=");
        //   const name = eqPos > -1 ? cookie.substring(0, eqPos) : cookie;
        //   document.cookie = name + "=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;";
        // }
        const cookies = document.cookie.split(';');
        const paths = ['/', window.location.pathname];
        const domainParts = window.location.hostname.split('.');

        // Generate all possible domain levels (e.g., example.com, www.example.com)
        const domains = [];
        for (let i = 0; i < domainParts.length - 1; i++) {
          domains.push(domainParts.slice(i).join('.'));
        }

        cookies.forEach(cookie => {
          const key = cookie.trim().split('=')[0];
          
          // Clear for each path and domain combination
          paths.forEach(path => {
              domains.forEach(domain => {
                document.cookie = `${key}=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=${path}; domain=.${domain};`;
              });
              
              // Also try without the domain for good measure
              document.cookie = `${key}=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=${path};`;
          });
        });

        console.log("All cookies cleared");  
      }

      TBS.logout = function() {
        TBS.clearAuthCookie();
        window.location.href = TBS.baseUrl + "/logout";
        /*
        fetch(TBS.urlLogout(), {
          method: 'GET',
          //credentials: 'include',
        })
        .then(response => {
          if (!response.ok) {
            throw new Error('Failed to log out from the server');
          }
          return response.json();
        })
        .then(data => {
          TBS.log('Logged out successfully');
          //alert('You have been logged out.');
          //window.location.reload();
        })
        .catch(error => {
          console.error('Logout error:', error);
          //alert('An error occurred during logout.');
        });
        */
      }

      // Format Unix timestamp (miliseconds)
      TBS.formatTime = function(s) {
        const dtFormat = new Intl.DateTimeFormat('sv-SE', {
          timeZone: 'Asia/Jakarta',
          year: 'numeric', 
          month: '2-digit', 
          day: '2-digit', 
          hour: '2-digit', 
          minute: '2-digit', 
          second: '2-digit',
          timeZoneName: 'short'
          /*dateStyle: 'full',*/
          /*timeStyle: 'long',*/
        });

        return dtFormat.format(new Date(s/* * 1e3*/));
      }

      // transform YYYY-MM-DDThh:mm or YYYY-MM-DDThh:mm:ss into YYYY-MM-DD hh:mm:ss
      TBS.reformatDatetime = function(val) {
        if (val && (val.length == 16 || val.length == 19))
        {
          let x = val.replace("T", " ");
          if (val.length == 16)
            x = x + ":00";
          
          return x;
        }
        return null;
      }

      TBS.getCookie = function(name) {
        let value = '; ' + document.cookie;
        let parts = value.split(`; ${name}=`);
        if (parts.length == 2) 
          return parts.pop().split(';').shift();
        
        return "";
      }

      TBS.setCookie = function(name, value, days) {
        const date = new Date();
        date.setTime(date.getTime() + (days * 24 * 60 * 60 * 1000));
        document.cookie = `${name}=${value};expires=${date.toUTCString()};path=/`;
      }

      TBS.refreshAccessToken = function( callback=null) {
      // Note: TBS.runInTbsServer, will affect how CORS will be applied

        let requestBody = null;
        let credentials = null; 
        if (TBS.runInTbsServer) {
          // Send the refresh token in HTTP Cookie request header
          // If the "tbs_api_refresh_token" cookie has not expired, its value will be sent here.
          credentials = "include"; 
          requestBody = null;
        } 
        else {
          let jwtToken = TBS.getCookie("tbs_api_refresh_token");
          if (! TBS.isValidJwtToken(jwtToken) ) {
            TBS.logout();
            alert("Your session has expired. Please log in again.");
            TBS.hideChatUi();
            return;
          }

          const jsonBody = {
            // Note: The "tbs_api_refresh_token" cookie must not have the HttpOnly attribute.
            refreshToken: jwtToken
          }
          credentials = "same-origin";
          requestBody = JSON.stringify(jsonBody); // Send refresh token in json body
        }

        return fetch( TBS.urlGetRefreshToken(), {
          method: 'POST',
          credentials: credentials,
          headers: {
            'Content-Type': 'application/json',
          },
          // Send refresh token in json body
          body: requestBody,
        })
        .then(response => {
          if (!response.ok) {
            throw new Error('Failed to refresh access token');
          }
          return response.json();
        })
        .then(data => {

          TBS.accessToken           = data.result.accessToken;
          TBS.PROJECT_ID            = data.result.projectId;
          TBS.CLIENT_APP_ID         = data.result.clientAppId;
          TBS.WEBSERVICE_VERSION    = data.result.webserviceVersion;
          TBS.DEVICE_TOKEN          = data.result.deviceToken;
          TBS.CHAT_PROTOCOL_VERSION = data.result.chatProtocolVersion;
          TBS.USER_AGENT            = data.result.userAgent;

          if (callback != null)
            callback(data);
        })
        .catch(error => {
          console.error(error);
          TBS.hideChatUi();
          alert("Authentication to chat server unsuccessful");
        });
      }

      TBS.isJWTExpired = function(token) {
        if (!token) {
          return true; // No token means it's expired or invalid
        }

        const payloadBase64 = token.split('.')[1];
        if (!payloadBase64) {
          return true; // Invalid token format
        }

        try {
          const payload = JSON.parse(atob(payloadBase64));
          const currentTime = Math.floor(Date.now() / 1000); // Current time in seconds
          return payload.exp < currentTime; // Check if the token is expired
        } catch (error) {
          console.error('Failed to parse JWT:', error);
          return true; // Assume expired if parsing fails
        }
      }

      TBS.isValidJwtToken = function(token) {
        if (!token) {
          return false;
        }
        if (TBS.isJWTExpired(token)) {
          return false;
        }
        return true;
      }

      TBS.downloadFromResponse = function(response)
      {
        const contentDisposition = response.headers.get('Content-Disposition');
        const parts = contentDisposition.split(';');
        let filename = null;
        for (let part of parts) {
          part = part.trim();
          if (part.startsWith('filename=')) {
            filename = part.substring('filename='.length).trim().replace(/^"(.*)"$/, '$1');
            break;
          }
        }

        if (filename) {
          // Content-Disposition has filename
          return response.blob().then(blob => {
            const url = window.URL.createObjectURL(blob);
            const link = document.createElement('a');
            link.href = url;
            link.download = filename;
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
            window.URL.revokeObjectURL(url);
          });

        } else {
          // Content-Disposition is missing filename
          return null;
        }
      }

      TBS.doApiSend = function(url, method, body=null, callback=null, useContentType=true, download=false) {

        let headers = {
          "Authorization":        `Bearer ${TBS.accessToken}`,
          "User-Agent":           TBS.USER_AGENT,
          "X-Project-Id":         TBS.PROJECT_ID,
          "X-Client-App-Id":      TBS.CLIENT_APP_ID,
          "X-WebService-Version": TBS.WEBSERVICE_VERSION,
          "X-Device-Token":       TBS.DEVICE_TOKEN,
          "X-ChatProto-Version":  TBS.CHAT_PROTOCOL_VERSION,
        }

        if (useContentType) {
          headers['Content-Type'] = 'application/json';
        }

        fetch(url, {
          method: method,
          headers: headers,
          referrer: 'no-referrer',
          body: body
        }).then(function (response) {
          // Note.
          // Cross-Origin Request: The request was made to a different origin than the page making the request.
          // if this is cors request, make sure the server allow it by sending correct CORS responses
          // and the server set respond header Access-Control-Expose-Headers:Content-Disposition
          // because we are using it
          if (response.headers.has('Content-Disposition')) {
            // we are downloading file
            if (download)
              return TBS.downloadFromResponse(response);
            else {
              return response;
            }
          } else {
            // Content-Disposition header is not present
            // we got JSON response
            return response.json();
          }
        }).then(function (data) {
          if (data == null)   
            return;

          if (callback != null) {
            return callback(data);
          } 
          else {
            //TBS.log(data.message);
            //return;
            if (data.code == 200) {
                TBS.alert.info(data.message, 'Toast', '');
                return;
            } else {
                TBS.alert.error(data.message, 'Toast', '');
                return;
            }
          }
        }).catch(function (err) {
          TBS.hideModalLoader();
          TBS.alert.error(err, 'Toast', '');
          //TBS.log(err);
        });
      }

      TBS.apiSend = function(url, method, body=null, callback=null, useContentType=true, download=false) {
        if (body == '')
          body = null;
        
        if (! TBS.isLoggedIn()) {
          TBS.log('User not logged in');
          TBS.alert.error('User not logged in', 'Toast', '');
          return;
        }

        if (! TBS.isValidJwtToken(TBS.accessToken)) {
          TBS.refreshAccessToken( function(data){
            TBS.doApiSend(url, method, body, callback, useContentType, download);
          });
        }
        else
          TBS.doApiSend(url, method, body, callback, useContentType, download);
      }       
      
      // -------------------------------------------------------
      
      TBS.hideModalLoader = function() {
         let loader = document.querySelector('#modal_loader');
         loader.style.display = "none";
      }

      TBS.showModalLoader = function() {
         let loader = document.querySelector('#modal_loader');
         loader.style.display = "block";
      }
      
      TBS.getSiblings = function (e) {
         // https://www.javascripttutorial.net/javascript-dom/javascript-siblings/
         let siblings = []; 
         if(!e.parentNode) {
            return siblings;
         }

         let sibling  = e.parentNode.firstChild;
         while (sibling) {
            if (sibling.nodeType === 1 && sibling !== e) {
                  siblings.push(sibling);
            }
            sibling = sibling.nextSibling;
         }
         return siblings;
      };

    </script> <!-- JS SITE -->