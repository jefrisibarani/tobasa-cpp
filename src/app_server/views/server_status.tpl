{% extends "layouts/layout_basic.tpl" %}

{% block pageContent %}
          <div class="container">
            <div class="row justify-content-center">
              <div class="col-lg-12 mt-4">
                <div id="alertsInToast" class="alertContainer"></div>
                <div id="alertsInPage" class="alertContainer d-none"></div>
                <ul class="nav nav-tabs" id="myTab" role="tablist">
                  <li class="nav-item" role="presentation">
                     <button class="nav-link" id="dashboard-tab" type="button" role="tab" aria-selected="false" onclick="window.location.href='{{ pageBaseUrl }}/dashboard'"> Dashboard </button>
                   </li>
                  <li class="nav-item" role="presentation">
                    <button class="nav-link active" id="status-tab" data-bs-toggle="tab" data-bs-target="#status" type="button" role="tab" aria-controls="home" aria-selected="true"> Status </button>
                  </li>
                  <li class="nav-item" role="presentation">
                     <button class="nav-link" id="connections-tab" data-bs-toggle="tab" data-bs-target="#connections" type="button" role="tab" aria-controls="connections" aria-selected="false"> HTTP Connections </button>
                   </li>
                  <li class="nav-item" role="presentation">
                    <button class="nav-link" id="control-tab" data-bs-toggle="tab" data-bs-target="#control" type="button" role="tab" aria-controls="control" aria-selected="false"> Control </button>
                  </li>
                  <li class="nav-item" role="presentation">
                    <button class="nav-link" id="log-tab" data-bs-toggle="tab" data-bs-target="#log" type="button" role="tab" aria-controls="log" aria-selected="false"> Log </button>
                  </li>
                  <li class="nav-item" role="presentation">
                     <button class="nav-link" id="m-error-tab" data-bs-toggle="tab" data-bs-target="#m-error" type="button" role="tab" aria-controls="m-error" aria-selected="false"> Error </button>
                  </li>

                </ul>
                <div class="tab-content" id="myTabContent">

                  <div class="tab-pane fade show active" id="status" role="tabpanel" aria-labelledby="status-tab">
                    <div class="card shadow-lg border-0 rounded-lg mt-3">
                      <div id="cardEngineState" class="card-header">
                        <h3 class="text-center font-weight-light my-2"> Web service State </h3>
                      </div>
                      <div class="card-body">
                        <table id="engineStatsTbl" class="table">
                        </table>
                      </div>
                    </div>
                  </div>

                  <div class="tab-pane fade" id="connections" role="tabpanel" aria-labelledby="connections-tab">
                     <div class="card shadow-lg border-0 rounded-lg mt-3">
                       <div id="cardConnections" class="card-header">
                         <h3 class="text-center font-weight-light my-2">HTTP Connections</h3>
                       </div>
                       <div class="card-body">
                         <table id="connectionsContentTable" class="table table-dark table-sm">
                         </table>
                       </div>
                     </div>
                   </div>

                  <div class="tab-pane fade" id="control" role="tabpanel" aria-labelledby="control-tab">
                    <div class="card shadow-lg border-0 rounded-lg mt-3">
                     <div id="cardBtnControl" class="card-header">
                        <h3 class="text-center font-weight-light my-2">Engine Control</h3>
                      </div>
                      <div class="card-body">
                        <!--
                        <button id="btnStartEngine" type="button" class="btn btn-primary btn-lg">Start Engine</button>
                        <button id="btnStopEngine" type="button" class="btn btn-danger btn-lg">Stop Engine</button>
                        -->
                        <div id="alertsInForm" class="mt-5 alertContainer d-none"></div>
                      </div>
                    </div>
                  </div>

                  <div class="tab-pane fade" id="log" role="tabpanel" aria-labelledby="log-tab">
                    <div class="card shadow-lg border-0 rounded-lg mt-3">
                      <div id="cardEngineLog" class="card-header">
                        <h3 class="text-center font-weight-light my-2">Web service Log</h3>
                      </div>
                      <div class="card-body">
                        <div>
                          <label for="cbSelectLogSource">Source</label>
                          <select id="cbLogSource" name="cbSelectLogSource">
                            <option value="INFO">INFO</option>
                            <option value="ERROR">ERROR</option>
                            <option value="DEBUG">DEBUG</option>
                            <option value="TRACE">TRACE</option>
                            <option value="WARN">WARN</option>
                            <option value="SQL">SQL</option>
                            <option value="LIS">LIS</option>
                            <option value="ALL">ALL</option>
                          </select>

                          <label for="cbSelectLineCount">Maximum last lines to read:</label>
                          <select id="cbLineCount" name="cbSelectLineCount">
                            <option value="10">10</option>
                            <option value="20">20</option>
                            <option value="50">50</option>
                            <option value="100">100</option>
                            <option value="200">200</option>
                            <option value="400">400</option>
                            <option value="0">All</option>
                          </select>
                          <button id="btnGetLogFile" type="button" class="btn btn-primary btn-sm">Read</button>
                          <button id="btnClearLogArea" type="button" class="btn btn-primary btn-sm">Clear</button>
                        </div>
                        <div>
                          <label for="logTextArea" class="form-label"></label>
                          <textarea wrap="off" class="form-control" id="logTextArea" rows="20"></textarea>
                        </div>
                      </div>
                    </div>
                  </div>

                  <div class="tab-pane fade" id="m-error" role="tabpanel" aria-labelledby="m-error-tab">
                     <div class="card shadow-lg border-0 rounded-lg mt-3">
                       <div id="cardLisError" class="card-header">
                         <h3 class="text-center font-weight-light my-2">Error</h3>
                       </div>
                       <div class="card-body">
                         <table id="errorContentTable" class="table table-dark table-sm">
                         </table>
                       </div>
                     </div>
                   </div>

                </div>
              </div>
            </div>
          </div>
{% endblock %}

{% block pageVendorJS %}
    {# Required for bootbox dialog, datatable #}
    <script src="{{ pageBaseUrl }}/vendor/jquery/jquery.min.js" crossorigin="anonymous"></script>
    <script src="{{ pageBaseUrl }}/vendor/bootbox_5.5.2/bootbox.min.js" crossorigin="anonymous"></script>
{% endblock %}

{% block pageJS %}
    <script>
      /// read log file content
      /// if size equals 0, read all content
      /// if size > 0, get last file content as much as size
      function getLogFileContent(size,source)
      {
         var logTextArea = document.querySelector('#logTextArea');
         let url = TBS.baseUrl + '/api/read_log/' + size + '/' + source;
         TBS.showModalLoader();

         TBS.apiSend(url, 'GET', '', function(data) {
            TBS.hideModalLoader();
            if (data.code != 200) {
               TBS.alert.error(data.message, 'Toast', '');
            } else {
               // accumulate into buffer then append once to update textarea.value
               let buffer = '\n';
               for (let i = 0; i < data.result.length; i++)
               {
                  let line = data.result[i];
                  buffer += line + '\n';
               }
               logTextArea.value += buffer;
               // scroll to bottom to show appended text immediately
               logTextArea.scrollTop = logTextArea.scrollHeight;
            }
         });
     }

      function getEngineStatus(targetTab)
      {
         let url = TBS.baseUrl + '/api/server_status';
         TBS.showModalLoader();
         TBS.apiSend(url, 'GET', '', function(data) {
            TBS.hideModalLoader();
            if (data.code != 200) {
               TBS.alert.error(data.message, 'Toast', '');
            } else {
               let table = document.querySelector('#engineStatsTbl');
               table.innerHTML = '';
               let tbody = document.createElement("tbody");

               for (let i = 0; i < data.result.length; i++)
               {
                  let tRow  = document.createElement("tr");
                  let tdKey = document.createElement("td");
                  let tdVal = document.createElement("td");

                  let key = data.result[i][1];
                  let val = data.result[i][2];

                  if (key === "Status") {
                     let cardEngineState = document.querySelector('#cardEngineState');
                     if (val.indexOf('Stopped')===0)
                        tdVal.style.backgroundColor = '#f9adb4';
                     else {
                        tdVal.style.backgroundColor = '#dffde6';
                        tdVal.style.borderColor = '#9ee2ae';
                     }
                  }

                  if (key === "Errors") {
                     let errorObjs = val;
                     if (val.length > 0)
                        val = val.length + " errors occured";
                     else
                        val = "0";

                     let errContentTable = document.querySelector('#errorContentTable');
                     errContentTable.innerHTML = '';

                     let xhead = document.createElement("thead");
                     let xRow  = document.createElement("tr");

                     let xtlbl0 = document.createElement("td");
                     xtlbl0.textContent = 'No.';
                     let xtlbl1 = document.createElement("td");
                     xtlbl1.textContent = 'Source';
                     let xtlbl2 = document.createElement("td");
                     xtlbl2.textContent = 'Timestamp';
                     let xtlbl3 = document.createElement("td");
                     xtlbl3.textContent = 'Task';
                     let xtlbl4 = document.createElement("td");
                     xtlbl4.textContent = 'Message';
                     xRow.append(xtlbl0,xtlbl1,xtlbl2,xtlbl3,xtlbl4);
                     xhead.append(xRow);
                     errContentTable.append(xhead);

                     let xbody = document.createElement("tbody");
                     for (let i = 0; i < errorObjs.length; i++)
                     {
                        let er = errorObjs[i];
                        let xRow  = document.createElement("tr");

                        let xNo = document.createElement("td");
                        xNo.textContent = (i+1).toString();
                        let xSource = document.createElement("td");
                        xSource.textContent = er.source;
                        let xTime = document.createElement("td");
                        xTime.textContent = TBS.formatTime(er.timestamp);
                        let xTask = document.createElement("td");
                        xTask.textContent = er.task;
                        let xMsg = document.createElement("td");
                        xMsg.textContent = er.message;
                        xRow.append(xNo,xSource,xTime,xTask,xMsg);
                        xbody.append(xRow);
                     }
                     errContentTable.append(xbody);
                  }

                  if (key === "Connections") {
                     let connInfos = val;
                     if (val.length > 0)
                        val = val.length + " connections ";
                     else
                        val = "0";

                     let connContentTable = document.querySelector('#connectionsContentTable');
                     connContentTable.innerHTML = '';

                     let xhead = document.createElement("thead");
                     let xRow  = document.createElement("tr");

                     let xtlbl0 = document.createElement("td");
                     xtlbl0.textContent = 'No.';
                     let xtlbl1 = document.createElement("td");
                     xtlbl1.textContent = 'Conn Id';
                     let xtlbl2 = document.createElement("td");
                     xtlbl2.textContent = 'Remote endpoint';
                     let xtlbl3 = document.createElement("td");
                     xtlbl3.textContent = 'Closed';
                     let xtlbl4 = document.createElement("td");
                     xtlbl4.textContent = 'TLS';
                     let xtlbl5 = document.createElement("td");
                     xtlbl5.textContent = 'Websocket';
                     let xtlbl6 = document.createElement("td");
                     xtlbl6.textContent = 'Identifier';
                     let xtlbl7 = document.createElement("td");
                     xtlbl7.textContent = 'Start time';
                     let xtlbl8 = document.createElement("td");
                     xtlbl8.textContent = 'Duration';

                     xRow.append(xtlbl0, xtlbl1, xtlbl2, xtlbl3, xtlbl4, xtlbl5, xtlbl6, xtlbl7, xtlbl8);
                     xhead.append(xRow);
                     connContentTable.append(xhead);

                     let xbody = document.createElement("tbody");
                     for (let i = 0; i < connInfos.length; i++)
                     {
                        let info = connInfos[i];
                        let xRow  = document.createElement("tr");

                        let xNo = document.createElement("td");
                        xNo.textContent = (i+1).toString();
                        let xConnId = document.createElement("td");
                        xConnId.textContent = info.connId;
                        let xRemote = document.createElement("td");
                        xRemote.textContent = info.remoteEndpoint;
                        let xClosed = document.createElement("td");
                        xClosed.textContent = info.closed ? "True" : "False";
                        let xTls = document.createElement("td");
                        xTls.textContent = info.tls ? "TLS" : "-";
                        let xWebsocket = document.createElement("td");
                        xWebsocket.textContent = info.websocket ? "WS" : "-";
                        let xIdentifier = document.createElement("td");
                        xIdentifier.textContent = info.identifier;

                        let xStartTime = document.createElement("td");
                        xStartTime.textContent = info.startTime;
                        let xDuration = document.createElement("td");
                        xDuration.textContent = info.duration;

                        xRow.append(xNo, xConnId, xRemote, xClosed, xTls, xWebsocket, xIdentifier, xStartTime, xDuration);
                        xbody.append(xRow);
                     }
                     connContentTable.append(xbody);
                  }


                  tdKey.textContent = key;
                  tdVal.textContent = val;
                  tRow.append(tdKey,tdVal);
                  tbody.append(tRow);
               }

               table.append(tbody);
               showSelectedTab(targetTab);
            }
         });
      }

      function showSelectedTab(targetTab) {
         let targetTabId = targetTab;
         if (targetTab==null) {
            const savedTabId = TBS.getCookie('tbs_server_status_tab');
            targetTabId = savedTabId;
         }

         if (targetTabId) {
            const tabToActivate = document.getElementById(targetTabId);
            if (tabToActivate) {
               const bootstrapTab = new bootstrap.Tab(tabToActivate);
               bootstrapTab.show();
            }
         }
      }

      // Save selected tab to cookie on tab change
      const tabs = document.querySelectorAll('.nav-link');
      tabs.forEach(tab => {
         tab.addEventListener('click', function (event) {
            const selectedTabId = event.target.id;
            getEngineStatus(selectedTabId);
         });

         tab.addEventListener('shown.bs.tab', function (event) {
            const selectedTabId = event.target.id;                    // Get the ID of the selected tab
            if (selectedTabId == "dashboard-tab")
               selectedTabId = "status-tab";
            TBS.setCookie('tbs_server_status_tab', selectedTabId, 2); // Save it to a cookie for 2 days
         });
      });

      const logTextArea = document.querySelector('#logTextArea');
      const cbLineCount = document.querySelector('#cbLineCount');
      const cbLogSource = document.querySelector('#cbLogSource');

      const btnGetLogFile   = document.querySelector('#btnGetLogFile');
      if (btnGetLogFile) {
         btnGetLogFile.addEventListener('click', event => {
            event.preventDefault();
            let size   = cbLineCount.value;
            let source = cbLogSource.value;

            if (size==0 || size>20)
               logTextArea.value = '';

            getLogFileContent(size,source);
         });
      }

      const btnClearLogArea = document.querySelector('#btnClearLogArea');
      if (btnClearLogArea) {
         btnClearLogArea.addEventListener('click', event => {
            event.preventDefault();
            logTextArea.value = '';
         });
      }

      window.addEventListener('DOMContentLoaded', event => {
         getEngineStatus();
      });

    </script>
{% endblock %}