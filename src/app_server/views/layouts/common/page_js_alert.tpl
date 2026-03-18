    <script type="text/javascript"> <!-- JS ALERT -->
      // https://dev.to/bmsvieira/vanilla-js-fadein-out-2a6o
      function fadeOut(el) {
         el.style.opacity = 1;
         (function fade() {
            if ((el.style.opacity -= .1) < 0) {
               el.style.display = "none";
            } else {
               requestAnimationFrame(fade);
            }
         })();
      };

      function fadeIn(el, display) {
         el.style.opacity = 0;
         el.style.display = display || "block";
         (function fade() {
            var val = parseFloat(el.style.opacity);
            if (!((val += .1) > 1)) {
               el.style.opacity = val;
               requestAnimationFrame(fade);
            }
         })();
      };

      var showAlert = function (options) {
         var alertType = options.alertType;
         var message   = options.message;
         var location  = options.location;
         var alertId   = options.alertId;
         var autoClose = options.autoClose;

         var iconClass = "d-flex justify-content-between";
         var iconId    = "#info-fill";
         var iconLabel = "";

         if (alertType == "alert-success") {
            iconId = "check-circle-fill";
            iconLabel = "Success:";
         }
         if (alertType == "alert-info") {
            iconId = "info-fill";   
            iconLabel = "Info:";
         }
         if (alertType == "alert-warning") {
            iconId = "exclamation-triangle-fill";  
            iconLabel = "Warning:";
         }
         if (alertType == "alert-danger") {
            iconId = "exclamation-triangle-fill";
            iconLabel = "Danger:";
         }
         
         let alertClass = `alert ${alertType} ${iconClass} alert-dismissable fade show`;
         let svgIcon    = `<svg class="bi flex-shrink-0 me-2" width="24" height="24" role="img" aria-label="${iconLabel}:"><use xlink:href="#${iconId}"/></svg>`;
         /*
         let myAlert = `
            <div id="${alertId}" class="alert ${alertType} ${iconClass} alert-dismissable fade show" role="alert">
               ${svgIcon}
               <span>${message}</span>
               <button type="button" class="btn-close align-right" data-bs-dismiss="alert" aria-label="Close"></button>
            </div>`;
         */         
         
         let myAlert = document.createElement("div");
         myAlert.id = alertId;
         myAlert.setAttribute("class", alertClass);
         myAlert.setAttribute("role", "alert");
         myAlert.innerHTML += svgIcon;
         myAlert.innerHTML += `<span>${message}</span>`; 
         myAlert.innerHTML += `<button type="button" class="btn-close align-right" data-bs-dismiss="alert" aria-label="Close"></button>`; 
         
         var alertElement = null;
         var alertPlaceHolderId = "";
         
         if (location === 'Toast')
            alertPlaceHolderId = '#alertsInToast';
         if(location ==='Page')
            alertPlaceHolderId = '#alertsInPage';
         if (location === 'Form')
            alertPlaceHolderId = '#alertsInForm';
         
         let alertPlaceHolder = document.querySelector(alertPlaceHolderId); 
         if (alertPlaceHolder) {
            //alertPlaceHolder.innerHTML += myAlert;
            alertPlaceHolder.append(myAlert);
            alertElement = alertPlaceHolder.lastChild;
         }

         if (autoClose) {
            fadeIn(alertElement,'');
            setTimeout(function() {
               fadeOut(alertElement,'');
               var bsAlert = new bootstrap.Alert(alertElement);
               bsAlert.close();
         }, 8000);
         }
         else {
            fadeIn(alertElement,'');
         }
      };

      window.addEventListener('DOMContentLoaded', event => {
         TBS.alert = {
            show:    showAlert,
            success: function (message, location = 'Toast', alertId = '', autoClose = true) { showAlert({ alertType: "alert-success",  message: message, location: location, alertId: alertId, autoClose: autoClose }); },
            info:    function (message, location = 'Toast', alertId = '', autoClose = true) { showAlert({ alertType: "alert-info",     message: message, location: location, alertId: alertId, autoClose: autoClose }); },
            warning: function (message, location = 'Toast', alertId = '', autoClose = true) { showAlert({ alertType: "alert-warning",  message: message, location: location, alertId: alertId, autoClose: autoClose }); },
            error:   function (message, location = 'Toast', alertId = '', autoClose = true) { showAlert({ alertType: "alert-danger",   message: message, location: location, alertId: alertId, autoClose: autoClose }); },
         };

         let alertsInForm = document.querySelector('#alertsInForm');
         if (alertsInForm) {
            alertsInForm.classList.remove('d-none');
         }
         
         let alertsInPage = document.querySelector('#alertsInPage'); 
         if (alertsInPage) {
            alertsInPage.classList.remove('d-none');
         }

         for (let i = 0; i < TBS.alertData.length; i++) 
         {
            let alert = TBS.alertData[i];

            TBS.alert.show({
               message: alert.message,
               alertType: alert.type,
               location: alert.location,
               alertId: alert.id,
               autoClose: alert.autoClose
            });
         }
      
      });
    </script> <!-- JS ALERT -->