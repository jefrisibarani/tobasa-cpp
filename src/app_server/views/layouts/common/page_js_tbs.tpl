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
        refreshToken: null,
        accessToken: null,
        wsConnIdentity: null,
        sseConnIdentity: null,
        PROJECT_ID: '',
        CLIENT_APP_ID: '',
        WEBSERVICE_VERSION: '',
        DEVICE_TOKEN: '',
        CHAT_PROTOCOL_VERSION: '',
        USER_AGENT: ''
      };
    </script> <!-- JS TBS -->
