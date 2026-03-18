    <script type="text/javascript"> <!-- JS TBS -->
      var TBS = { 
        debug: true,
        baseUrl: "{{ pageBaseUrl }}",
        baseUrlMCU: "",
        runInTbsServer: true,
        refreshToTbsServer: true,

        alertData: {{ default(pageAlerts, "[]") }},
        sessionExpNotice: {{ default(sessExpNotice, -1) }}, // seconds
        sessionExpires: {{ default(sessExpTime, -1) }},     // seconds        

        // this can be stored in cookie
        refreshToken: null,
        
        // these values must stay in memory
        accessToken: null,

        PROJECT_ID: '',
        CLIENT_APP_ID: '',
        WEBSERVICE_VERSION: '',
        DEVICE_TOKEN: '',
        CHAT_PROTOCOL_VERSION: '',
        USER_AGENT: ''
      };
    </script> <!-- JS TBS -->
    