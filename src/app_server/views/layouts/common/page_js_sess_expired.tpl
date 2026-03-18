    <script type="text/javascript"> <!-- JS session expired -->
      "use strict";
      var sessionCountDownInterval;
      var confirmDialogInterval;
      var sessionTimeout = 0;
      TBS.sessioncookie  = TBS.getCookie("tbs_session");

      window.addEventListener('DOMContentLoaded', event => {

         let tsExpired  = TBS.sessionExpires;
         let tsNow      = Math.floor(Date.now()/1000);
         sessionTimeout = tsExpired - tsNow; 

         // every one seconds, call sessionCountDown
         if (sessionTimeout > 0)
            sessionCountDownInterval = setInterval(sessionCountDown, 1000);
      });

      function sessionCountDown() {
         sessionTimeout -= 1;

         if (TBS.debug) {
            var msg = transformSecondsToMS(sessionTimeout);
            document.querySelector('#session_counter').textContent = msg;
         }

         if (sessionTimeout == 0) {
            // session cookie might already expired, so we reset the value with previously saved.
            //document.cookie = 'tbs_session=' + TBS.sessioncookie + ';';
            //window.location.href = TBS.baseUrl + '/logout';
            TBS.logout();
         }

         if (TBS.sessionExpNotice == sessionTimeout) {
            var remaining = getSessionTimeOutFromKeepAlive();
            TBS.log("Expire Notification 1 : timeout: " + sessionTimeout + ", remaining: "+ remaining);

            if (remaining <= sessionTimeout)
               var dialog = showSessionConfirmationDialog();
            else
               sessionTimeout = remaining;
         }
      }

      function showSessionConfirmationDialog() {
         var content = `
            <div class="row">
               <div class="col-sm-12">
                  Sesi anda akan segera berakhir. Anda akan otomatis logout dalam
               </div>
               <div class="col-sm-12 session_count_down text-danger text-center">
               </div>
               <div class="col">
                  Data yang belum tersimpan akan hilang bila logout.</br>
                  Pilih <b>Tetap login</b> untuk melanjutkan sesi.
               </div>
            </div>`;

         var dialog = bootbox.confirm({
            title: "Sesi akan segera berakhir",
            message: content,
            buttons: {
               cancel: {
                  label: '<i class="fa fa-times"></i> Close',
                  callback: function () {}
               },
               confirm: {
                  label: '<i class="fa fa-check"></i> Tetap log in'
               }
            },
            callback: function (result) {
               if (result) {
                  // user ingin tetap logged in, set sessionTimeout menjadi remaining time terbaru
                  var remaining = getSessionTimeOutFromKeepAlive();
                  TBS.log("Expire Notification 2 : timeout: " + sessionTimeout + ", remaining: "+ remaining);
                  sessionTimeout = remaining;
               }
               else {
                  // reset counter
                  if (confirmDialogInterval != null)
                     clearInterval(confirmDialogInterval);
               }
            }
         });

         var dialogCountDown = TBS.sessionExpNotice;
         dialog.init(function () {
            confirmDialogInterval = setInterval(function () {
               dialogCountDown -= 1;

               var msg = transformSecondsToMS(dialogCountDown);
               dialog.find('.session_count_down').text(msg);
            }, 1000);
         });

         return dialog;
      }

      function getSessionTimeOutFromKeepAlive() {
         var rval = -1;
         const request = new XMLHttpRequest();
         request.open("GET", "{{ pageBaseUrl }}/keep_alive", false); // `false` makes the request synchronous
         request.send(null);

         if (request.status === 200) {
            try {
               let data = JSON.parse(request.responseText);
               if (data.code === 200)
                  rval = data.result;
            }
            catch(error) {
               console.error(error);
            }
         }

         return rval;
      }

      function transformSecondsToMS(seconds) {
         var m = Math.floor(seconds / 60);
         var s = Math.floor(seconds % 60);
         var mDisplay = m < 10 ? "0" + m : m;
         var sDisplay = s < 10 ? "0" + s : s;
         return mDisplay + ":" + sDisplay;
      }

    </script> <!-- JS session expired -->