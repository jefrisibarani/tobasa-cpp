{% extends "../layouts/layout_vertical.tpl" %}

{% block pageContent %}
            <!-- Main content -->
            <div class="col-md-12">
              <div class="card">
                <div class="card-header">
                  <h3 class="card-title" style="font-size : 1.4em;">Order List</h3>
                  <span style="float:right;font-size : 1.2em;">
                     <table border="0" width="550">
                       <tr>
                         <td width="20%" align="center">Start</td>
                         <td width="20%" align="center"><input type="date" class="form-control" id="tb_start_date" name="f_start_date"></td>
                         <td width="20%" align="center">End</td>
                         <td width="20%" align="center"><input type="date" class="form-control" id="tb_end_date" name="f_end_date"></td>
                         <td width="3%">&nbsp;</td>
                         <td width="17%"><input type="button" class="form-control" id="btn_getentries" value="Process" onclick="onGetEntries()"></td>
                       </tr>
                     </table>
                  </span>
                </div>
                <!-- /.card-header -->
                <div class="card-body p-2 table-scrollable-x">
                  <table id="{{ default(datatableId, "") }}" class="table table-bordered table-sm table-stripped table-hover" cellspacing="0" width="100%">
                    <thead>
                      <tr>
                        <th style="width: 80px">Action</th>
                        <th style="width: 10px">No</th>
                        <th>Identifier</th> <!-- Identifier -->
                        <th>Universal Id</th>
                        <th>Universal Name</th>
                        <th>Test Datetime</th>
                        <th>Patient Name</th>
                        <th>Patient Gender</th>
                      </tr>
                    </thead>
                    <tbody>
                      {% set idx=0 %}
                      {% for obr in lisorders %}
                      {% set idx=idx+1 %}
                      <tr tbs_data_obr_id="{{obr.obrId}}"
                          tbx_data_obr_msh_id="{{obr.dbMshId}}"
                          tbx_data_obr_pid_id="{{obr.dbPidId}}">
                        <td>
                          {% if 0>1 %}
                          <!--a href="#" onclick="showFormDetail('{{obr.obrId}}');" data-bs-toggle="tooltip" data-bs-placement="top" title="View details"><i class="fas fa-table"></i></a-->
                          <!--a href="#" onclick="showFormDelete('{{obr.obrId}}');"  data-bs-toggle="tooltip" data-bs-placement="top" title="Delete data"><i class="fas fa-trash"></i></a-->
                          {% endif %}
                        </td>
                        <th scope="row">{{ idx }}</th>
                        <td>{{ obr.obrIdentifier }}</td>
                        <td>{{ obr.obrUniversalId }}</td>
                        <td>{{ obr.obrUniversalName }}</td>
                        <td>{{ obr.obrObservationDatetime }}</td>
                        <td>{{ obr.pidName }}</td>
                        <td>{{ obr.pidGender }}</td>
                      </tr>
                      {% endfor %}
                    </tbody>
                  </table>
                </div> <!-- /.card-body -->
              </div> <!-- /.card -->
            </div>
            <!-- /Main content -->

            <!-- Modal for histogram zoom-->
            <div class="modal fade" id="imageModal" tabindex="-1" aria-labelledby="imageModalLabel" aria-hidden="true">
               <div class="modal-dialog">
               <div class="modal-content">
                  <div class="modal-header">
                     <!--h5 class="modal-title" id="exampleModalLabel">Modal title</h5-->
                     <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                  </div>
                  <div class="modal-body">
                     <img id="modalImage" class="img-fluid" src="" alt="Zoomed Image">
                  </div>
                  <div class="modal-footer">
                     <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">Close</button>
                  </div>
               </div>
               </div>
            </div>

{% endblock %}


{% block pageVendorCSS %}
    {% if datatable %}
      {% include "../layouts/common/page_css_datatable.tpl" %}
    {% endif %}
{% endblock %}


{% block pageCSS %}
    <style>
      #imageModal .modal-dialog { width: 100%; height: auto; }
      #imageModal .modal-content {/*overflow: hidden;*/ }
      #modalImage { width: 200%; height: auto; /*transform: scale(0.5); */}
    </style>
{% endblock %}


{% block pageVendorJS %}
    {# Required for bootbox dialog, datatable, summernote #}
    <script src="{{ pageBaseUrl }}/vendor/jquery/jquery.min.js" crossorigin="anonymous"></script>
    {% if datatable %}
      {% include "../layouts/common/page_js_datatable.tpl" %}
    {% endif %}
    <script src="{{ pageBaseUrl }}/vendor/bootbox_5.5.2/bootbox.min.js" crossorigin="anonymous"></script>
{% endblock %}


{% block pageJS %}
    <script>
      function getObxList(obrId, patientId, handler) {
         let url = TBS.baseUrl + '/api/lis/hl7_obxlist/' + obrId + '/' + patientId;
         TBS.apiSend(url, 'GET', '', function(data) {
            if (data.code != 200) {
               TBS.alert.error(data.message, 'Toast', '');
            } else {
               handler(data);
            }
         });
      }

      function onGetEntries(filterVal='') {
         let startDate = document.getElementById('tb_start_date').value;
         let endDate   = document.getElementById('tb_end_date').value;

         if (filterVal.length > 0)
            location.href = '{{ pageBaseUrl }}/lis/testdev_hl7?startdate=' + startDate + '&enddate=' + endDate  + '&filter=' + filterVal;
         else
            location.href = '{{ pageBaseUrl }}/lis/testdev_hl7?startdate=' + startDate + '&enddate=' + endDate;
      }

      function isObxImageField(obx)
      {
         if (  obx.name == "WBC Histogram. Total"
            || obx.name == "WBC Histogram. BMP"
            || obx.name == "RBC Histogram. Left Line"
            || obx.name == "RBC Histogram. Right Line"
            || obx.name == "RBC Histogram. Total"
            || obx.name == "RBC Histogram. BMP"
            || obx.name == "PLT Histogram. Left Line"
            || obx.name == "PLT Histogram. Right Line"
            || obx.name == "PLT Histogram. Total"
            || obx.name == "PLT Histogram. BMP"
            || obx.name == "WBC DIFF Scattergram. Fsc dimension"
            || obx.name == "WBC DIFF Scattergram. Ssc dimension"
            || obx.name == "WBC DIFF Scattergram. BMP"
            || obx.name == "Baso Scattergram. Fsc dimension"
            || obx.name == "Baso Scattergram. Ssc dimension"
            || obx.name == "Baso Scattergram. BMP" )
         {
            return true;
         }
         return false;
      }

      function openModal(img) {
         var src = img.src;
         $('#modalImage').attr('src', src);
         $('#imageModal').modal('show');
      }

      window.addEventListener('DOMContentLoaded', event => {

         document.getElementById('tb_start_date').value = '{{ default(dataStartDate, "") }}';
         document.getElementById('tb_end_date').value = '{{ default(dataEndDate, "") }}';

         {% if length(lisorders) > 0 %}
         {% if datatableId %}
            let table = new DataTable('#{{ datatableId }}', {
               columnDefs: [
                  { targets: [0], searchable: false, orderable: false, width: "5%"}
               ]
            });


            table.on('click', 'tbody td:not(:first-child):not(:last-child)', function (e) {
               var currentRow = this.parentElement;
               let obrIdAttrib = currentRow.getAttribute("tbs_data_obr_id");
               let patIdAttrib = currentRow.getAttribute("tbx_data_obr_pid_id");
               let processedAttrib = currentRow.getAttribute("tbs-processed");
               if (processedAttrib != null) {
                  let nextSibling = currentRow.nextSibling;
                  let isRowDetail = nextSibling.getAttribute("tbs-row-details");
                  if (isRowDetail != null)
                  {
                     if ( nextSibling.style.display !== 'none' && nextSibling.visibility !== 'hidden')
                         nextSibling.style.setProperty("display","none");
                     else
                         nextSibling.style.removeProperty('display');
                  }
               }

               if (processedAttrib == null) {
                  if (obrIdAttrib != null) {
                     getObxList(obrIdAttrib, patIdAttrib, function (data) {
                        let rowContent = document.createElement("tr");
                        rowContent.setAttribute("tbs-row-details",true);
                        let colContent = document.createElement("td");
                        colContent.setAttribute('colspan', currentRow.children.length);

                        let colInnerHtml =
                        `<div>
                        <table class="table table-bordered table-sm table-stripped table-hover" cellspacing="0" border="0" width="100%">
                           <thead>
                           <tr>
                              <th>Name</th>
                              <th>Value</th>
                              <th>Unit</th>
                              <th>Reference Range</th>
                              <th>Abnormal Flag</th>
                              <th>Result Status</th>
                           </tr>
                           </thead>
                           <tbody>`;

                        var hasImageField = false;
                        for (let obx of data.result)
                        {
                           if ( ! isObxImageField(obx) )
                           {
                              colInnerHtml +=
                              `<tr>
                                 <td>${obx.name}</td>
                                 <td>${obx.value}</td>
                                 <td>${obx.unit}</td>
                                 <td>${obx.referenceRange}</td>
                                 <td>${obx.abnormalFlag}</td>
                                 <td>${obx.resultStatus}</td>
                              </tr>`;
                           }
                           else {
                              // one time check only
                              if (!hasImageField)
                                 hasImageField = (obx.binaryValue == 1);
                           }
                        }

                        if (hasImageField)
                        {
                           colInnerHtml +=
                                    `<tr>
                                       <td colspan="6">
                                          <div class="row" id="obx_image_row">`;

                           for (let obx of data.result)
                           {
                              if (obx.binaryValue == 1 && obx.binaryEncoding == "Base64")
                              {
                                 var obxImageName = obx.name;

                                 colInnerHtml +=
                                    `       <div class="col">
                                                <div class="card">
                                                   <div class="card-header">${obxImageName}</div>
                                                   <div card-body>
                                                      <div class="d-flex justify-content-center align-items-center p-2">
                                                         <img src="data:image/png;base64, ${obx.binaryData}" alt="${obxImageName}" onclick="openModal(this)">
                                                      </div>
                                                   </div>
                                                </div> <!-- /card -->
                                             </div>`;
                              }
                           }

                           colInnerHtml +=
                                    `      </div> <!-- /obx_image_row -->
                                       </td>
                                    </tr>`;
                        }

                        colInnerHtml +=
                        `   </tbody>
                        </table>
                        </div>
                        `;

                        colContent.innerHTML = colInnerHtml;

                        rowContent.append(colContent);
                        rowContent.className = "background_soft_green";
                        currentRow.parentNode.insertBefore(rowContent, currentRow.nextSibling);
                        currentRow.setAttribute("tbs-processed",true);
                     });
                  }
               }
            });

         {% endif %}
         {% endif %}
      });

    </script>
{% endblock %}