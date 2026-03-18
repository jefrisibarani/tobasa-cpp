{% extends "layouts/layout_vertical.tpl" %}

{% block pageContent %}
            <!-- Main content -->
            <div class="col-md-12">
              <div class="card">
                <div class="card-header">
                  <h3 class="card-title" style="font-size : 1.4em;">Roles</h3>
                </div>
                <!-- /.card-header -->
                <div class="card-body p-2 table-scrollable-x">
                  <table id="{{ default(datatableId, "") }}" class="table table-sm table-bordered table-stripped table-hover">
                    <thead>
                      <tr>
                        <th style="width: 100px">Action</th>
                        <th style="width: 10px">No</th>
                        <th>Role name</th>
                        <th>Alias</th>
                        <th>Enabled</th>
                        <th>Sys Role</th>
                      </tr>
                    </thead>
                    <tbody>
                      {% set idx=0 %}

                      {% if length(roles) == 0 %}
                      <tr>
                        <td colspan="5">
                          <a href="#" onclick="showFormAdd('0');"  data-bs-toggle="tooltip" data-bs-placement="top" title="New data"><i class="fas fa-plus"></i></a>
                        </td>
                      </tr>
                      {% endif %}

                      {% for item in roles %}
                      {% set idx=idx+1 %}
                      <tr id="id_row_{{idx}}">
                        <td>
                          <a href="#" onclick="showFormEdit('{{idx}}');" data-bs-toggle="tooltip" data-bs-placement="top" title="Edit data"><i class="fas fa-edit"></i></a>
                          <a href="#" onclick="showFormDelete('{{idx}}');"  data-bs-toggle="tooltip" data-bs-placement="top" title="Delete data"><i class="fas fa-trash"></i></a>
                          <a href="#" onclick="showFormAdd('0');"  data-bs-toggle="tooltip" data-bs-placement="top" title="New data"><i class="fas fa-plus"></i></a>
                          <a href="#" onclick="showFormAddMember('{{idx}}');" data-bs-toggle="tooltip" data-bs-placement="top" title="Add member"><i class="fas fa-user-plus"></i></a>
                          <a href="#" onclick="viewGroupMembers('{{idx}}');" data-bs-toggle="tooltip" data-bs-placement="top" title="View members"><i class="fas fa-list"></i></a>
                        </td>
                        <th scope="row">{{ idx }}</th>
                        <input type="hidden" id="id_item_id_{{idx}}" value="{{ item.id}}"/>
                        <td id='id_item_name_{{idx}}'>{{ item.name }}</td>
                        <td id='id_item_alias_{{idx}}'>{{ item.alias }}</td>
                        <td id='id_item_enabled_{{idx}}'>{{ item.enabled }}</td>
                        <td id='id_item_sysrole_{{idx}}'>{{ item.sysRole }}</td>
                      </tr>
                      {% endfor %}
                    </tbody>
                  </table>
                </div> <!-- /.card-body -->
              </div> <!-- /.card -->
            </div>
            <!-- /Main content -->
{% endblock %}


{% block pageVendorCSS %}
    {% if datatable %}
      {% include "layouts/common/page_css_datatable.tpl" %}
    {% endif %}

    <link rel="stylesheet" type="text/css" href="{{ pageBaseUrl }}/vendor/selectize/css/selectize.bootstrap5.css"/>
{% endblock %}


{% block pageCSS %}
{% endblock %}


{% block pageVendorJS %}
    {# Required for bootbox dialog, datatable, summernote #}
    <script src="{{ pageBaseUrl }}/vendor/jquery/jquery.min.js" crossorigin="anonymous"></script>
    {% if datatable %}
      {% include "layouts/common/page_js_datatable.tpl" %}
    {% endif %}
    <script src="{{ pageBaseUrl }}/vendor/bootbox_5.5.2/bootbox.min.js" crossorigin="anonymous"></script>
    <script src="{{ pageBaseUrl }}/vendor/selectize/js/selectize.min.js" crossorigin="anonymous"></script>
{% endblock %}


{% block pageJS %}
<script>
   function showForm(idx, task) {
      let dialogCaption = '';
      if (task === 'INSERT')
         dialogCaption = "Add new role";

      if (task === 'UPDATE')
         dialogCaption = "Edit role";

      if (task === 'DELETE')
         dialogCaption = "Delete role?";

      var dialog = bootbox.dialog({
         title: dialogCaption,
         size: 'medium',
         message: "Form Roles",
         onEscape: true,
         backdrop: true,
         buttons: {
            cancel: {label: '<i class="fas fa-times"></i> Cancel', className: 'btn-secondary' },
            ok: {
               label: '<i class="fas fa-check"></i> Submit',
               className: 'btn-primary',
               callback: function () {

                  let itemName = itemAlias = itemEnabled = itemsysRole = "";
                  let itemId = 0;

                  itemId       = document.querySelector('#f_id').value * 1;
                  itemName     = document.querySelector('#f_name').value;
                  itemAlias    = document.querySelector('#f_alias').value;
                  itemEnabled  = document.querySelector('#f_enabled').value;
                  itemsysRole  = document.querySelector('#f_sysrole').value;

                  let payload = {
                     id:      (itemId   == null) ? 0  : itemId,
                     name:    (itemName == null) ? '' : itemName,
                     alias:   (itemAlias == null) ? '' : itemAlias,
                     enabled: (itemEnabled=='1') ? true : false,
                     sysRole: (itemsysRole=='1') ? true : false
                  };

                  let httpMethod = 'POST';
                  if (task === 'DELETE') {
                     httpMethod = 'DELETE';
                     payload = { id : itemId };
                  }

                  let apiCallOk = false;
                  TBS.apiSend(TBS.baseUrl + '/api/admin/roles/', httpMethod, JSON.stringify(payload), function(data) {
                     if (data.code == 200) {
                        window.location.href = TBS.baseUrl + '/admin/roles';
                        TBS.alert.info(data.message, 'Toast', '');
                     } else {
                        TBS.alert.error(data.message, 'Toast', '');
                        apiCallOk = false;
                     }
                  });

                  if (apiCallOk)
                     return true;
                  else
                     return false;
               }
            }
         }
      });

      dialog.init(function() {
            let valItemId = valItemName = valItemAlias = valItemEnabled = valItemSysRole = "";
            if (task === 'INSERT') {
               valItemId = 0;
            } else {
               valItemId      = document.querySelector('#id_item_id_'+ idx ).value;
               valItemName    = document.querySelector('#id_item_name_'+ idx ).innerHTML;
               valItemAlias   = document.querySelector('#id_item_alias_'+ idx ).innerHTML;
               valItemEnabled = document.querySelector('#id_item_enabled_'+ idx ).innerHTML;
               valItemSysRole = document.querySelector('#id_item_sysrole_'+ idx ).innerHTML;
            }

            let readonlyMark = "";
            if (task === 'DELETE')
               readonlyMark = "readonly" ;

            dialog.find('.bootbox-body').html(
            `
            <div class="card-body">
               <input type="hidden" id="f_id" value="${valItemId}">
               <div class="form-group row mb-1">
                  <label for="f_name" class="col-sm-3 col-form-label">Role name</label>
                  <div class="col-sm-9">
                  <input type="text" class="form-control" id="f_name" ${readonlyMark} placeholder="" value="${valItemName}">
                  </div>
               </div>
               <div class="form-group row mb-1">
                  <label for="f_alias" class="col-sm-3 col-form-label">Alias</label>
                  <div class="col-sm-9">
                     <input type="text" class="form-control" id="f_alias" ${readonlyMark} placeholder="" value="${valItemAlias}">
                  </div>
               </div>
               <div class="form-group row mb-1">
                  <label for="f_enabled" class="col-sm-3 col-form-label">Enabled</label>
                  <div class="col-sm-9">
                     <select class="form-select" name="f_enabled" id="f_enabled" ${readonlyMark} >
                        <option value="1" ${valItemEnabled=='true' ? 'selected="selected"':''}>TRUE</option>
                        <option value="0" ${valItemEnabled=='false' ? 'selected="selected"':''}>FALSE</option>
                     </select>
                  </div>
               </div>
               <div class="form-group row mb-1">
                  <label for="f_sysrole" class="col-sm-3 col-form-label">SYS Role</label>
                  <div class="col-sm-9">
                     <select class="form-select" name="f_sysrole" id="f_sysrole" ${readonlyMark}>
                        <option value="1" ${valItemSysRole=='true' ? 'selected="selected"':''}>TRUE</option>
                        <option value="0" ${valItemSysRole=='false' ? 'selected="selected"':''}>FALSE</option>
                     </select>
                  </div>
               </div>
            </div>`
            );
      });  /// end dialog.init

      dialog.modal('show');
   }

   function showFormAdd(idx) {
      showForm(idx, 'INSERT');
   }

   function showFormEdit(idx) {
      showForm(idx, 'UPDATE');
   }

   function showFormDelete(idx) {
      showForm(idx, 'DELETE');
   }

   function showFormAddMember(idx) {

      let valItemId   = document.querySelector('#id_item_id_'+ idx ).value;
      let valItemName = document.querySelector('#id_item_name_'+ idx ).innerHTML;
      let roleNameVal = (valItemName == null) ? '' : valItemName;
      let roleIdVal   = (valItemId == null) ? 0 : valItemId*1;

      var dialog = bootbox.dialog({
         show: false,
         size: 'medium',
         message: "Add user",
         title: 'Add user to group ' + roleNameVal,
         onEscape: true,
         backdrop: true,
         buttons: {
            cancel: {label: '<i class="fas fa-times"></i> Cancel', className: 'btn-secondary' },
            ok: {
               label: '<i class="fas fa-check"></i> Submit',
               className: 'btn-primary',
               callback: function () {
                  let selectElement  = document.querySelector('#select_users');
                  let payloadRoleId  = document.querySelector('#roleId').value;
                  let payloadUserIds = [];
                  Array.from(selectElement.selectedOptions).map(function(option) {
                        option.selected ? payloadUserIds.push(option.value *1 ) : null
                     });

                  let payload = {
                     roleId:  roleIdVal,
                     userIds: payloadUserIds
                  };

                  var apiuri = TBS.baseUrl + "/api/admin/roles/addusers";
                  TBS.apiSend(apiuri, 'POST', JSON.stringify(payload), function(data) {
                     if (data.code == 200) {
                        TBS.alert.info(data.message, 'Toast', '');
                     } else {
                        TBS.alert.error(data.message, 'Toast', '');
                     }
                  });

                  // $('#form_add_user').submit();
                  TBS.alerts.info("User telah ditambah");
               }
            }
         }
      });

      dialog.init(function () {
         var roleId = roleIdVal;
         var urlAddUsers = TBS.baseUrl + "/api/admin/roles/addusers";
         var content = `
            <div class="form-group row">
               <!--form id="form_add_user" action="${urlAddUsers}" method="post"-->
                  <input type="hidden" data-val="true" data-val-required="The Role Id field is required." id="roleId" name="roleId" value="${roleId}">
                  <!--label for="select_users">Select a user...</label-->
                  <select name="userId[]" id="select_users" class="form-select" multiple placeholder="pick a user..."></select>
               <!--/form-->
            </div>`;

         dialog.find('.bootbox-body').html(content);

         var formatName = function (item) {
            //return $.trim((item.firstName || '') + ' ' + (item.lastName || ''));
            return $.trim((item.userName));
         };

         $('#select_users').selectize({
            plugins: ["restore_on_backspace", "clear_button", "remove_button"],
            create: false,
            maxItems: null,
            valueField: 'id',
            labelField: 'userName',
            searchField: ['userName', 'email'],
            sortField: [
               {field: 'userName', direction: 'asc'},
               {field: 'email', direction: 'asc'}
            ],
            selectOnTab: true,
            preload: true,
            render: {
               item: function(item, escape) {
                  var name = formatName(item);
                  return '<div>' +
                        (name ?       '<span class="name">'  + escape(name) + '</span>' : '') +
                        (item.email ? ' <span class="email">' + escape(item.email) + '</span>' : '') +
                        '</div>';
               },
               option: function(item, escape) {
                  var name    = formatName(item);
                  var label   = name || item.email;
                  var caption = name ? item.email : null;
                  return '<div>' +
                           '<span class="label">' + escape(label) + '</span>' +
                           (caption ? ' <span class="caption">' + escape(caption) + '</span>' : '') +
                         '</div>';
               }
            },
            load: function (query, callback) {
               var apiuri = TBS.baseUrl + "/api/admin/roles/get_non_member/?roleid=" + roleId;
               TBS.apiSend(apiuri, 'GET', '', function(data) {
                     if (data.code == 200) {
                        if (data.result.length > 0)
                           callback(data.result);
                        else {
                           TBS.alert.warning("No more users to add", 'Toast', '');
                           dialog.modal('hide');
                           //bootbox.hideAll();
                        }

                     } else {
                        TBS.alert.error(data.message, 'Toast', '');
                        callback(data.message);
                     }
                  });
            },
         });
      });


      dialog.on('shown.bs.modal', function (e) {
         });

      dialog.modal('show');
   }

   function viewGroupMembers(idx)
   {
      let valItemId   = document.querySelector('#id_item_id_'+ idx ).value;
      let valItemName = document.querySelector('#id_item_name_'+ idx ).innerHTML;
      let roleNameVal = (valItemName == null) ? '' : valItemName;
      let roleIdVal   = (valItemId == null) ? 0 : valItemId*1;

      let currentRow  = document.querySelector('#id_row_'+ idx );
      let processedAttrib = currentRow.getAttribute("tbs-processed");
      if (processedAttrib === 'true') {
         let subRow   = document.querySelector('#id_row_sub_'+ idx);
         if (subRow) {
            subRow.remove();
            currentRow.setAttribute("tbs-processed", false);
            return;
         }
      }
      else if (processedAttrib == null || processedAttrib === 'false') {

         var apiuri = TBS.baseUrl + "/api/admin/roles/get_member/?roleid=" + roleIdVal;
         TBS.apiSend(apiuri, 'GET', '', function(data) {
            if (data.code == 200) {
               if (data.result.length > 0) {
                  let rowContent = document.createElement("tr");
                  rowContent.id = 'id_row_sub_'+ idx;

                  let colContent = document.createElement("td");
                  colContent.setAttribute('colspan', currentRow.children.length);

                  let rowHtml = '';
                  for (let i in data.result) {
                     rowHtml += '<tr id="id_row_user_' + data.result[i].id + '">';
                     rowHtml += ` <td width="6%"><a class="elNeedTooltip" href="#" onclick="removeUserFromGroup(${roleIdVal},${data.result[i].id},${idx});" data-bs-toggle="tooltip" data-bs-placement="top" title="Remove from group"><i class="fas fa-user-times"></i></a></td>`;
                     rowHtml += ' <td>'+ data.result[i].userName +'</td>';
                     rowHtml += ' <td>'+ data.result[i].email +'</td>';
                     rowHtml += ' <td>'+ data.result[i].firstName + ' ' + data.result[i].lastName + '</td>';
                     rowHtml += '</tr>';
                  }

                  colContent.innerHTML =
                  `
                  <div>
                     <table class="ms-3 contentTable" cellspacing="0" border="0" width="50%">
                        ${rowHtml}
                     </table>
                  </div>
                  `;

                  rowContent.append(colContent);
                  //rowContent.style.setProperty("background-color","rgb(234, 255, 240)");
                  rowContent.className = "background_soft_green";
                  currentRow.parentNode.insertBefore(rowContent, currentRow.nextSibling);
                  currentRow.setAttribute("tbs-processed", true);

                  // enable tooltip on newly created buttons
                  var tooltipTriggerList = [].slice.call(document.getElementsByClassName('elNeedTooltip'));
                  var tooltipList = tooltipTriggerList.map(function (tooltipTriggerEl) {
                     return new bootstrap.Tooltip(tooltipTriggerEl)
                  });


               }
               else
                  TBS.alert.info("Role has no members", 'Toast', '');
            } else {
               TBS.alert.error(data.message, 'Toast', '');
            }
         });
      }
   }

   function removeUserFromGroup(roleId, userId, rowIndex)
   {
      let payload =
      {
         roleId: roleId,
         userId: userId
      };

      var apiuri = TBS.baseUrl + "/api/admin/roles/remove_user";
      TBS.apiSend(apiuri, 'POST', JSON.stringify(payload), function(data) {
         if (data.code == 200) {
            let myrow = document.querySelector('#id_row_user_'+ userId);
            if (myrow) {
               let siblings = TBS.getSiblings(myrow);
               if (siblings.length == 0) // myrow is the last user row
               {
                  let parentContentRow  = document.querySelector('#id_row_sub_'+ rowIndex );
                  if (parentContentRow)
                     parentContentRow.remove();

                  let parentDatatableRow  = document.querySelector('#id_row_'+ rowIndex );
                  if (parentDatatableRow)
                     parentDatatableRow.setAttribute("tbs-processed", false);
               }
               else
                  myrow.remove();
            }

            TBS.alert.info("user removed", 'Toast', '');
         }
         else
            TBS.alert.error(data.message, 'Toast', '');
      });
   }

   window.addEventListener('DOMContentLoaded', event => {
      {% if length(roles) > 0 and datatableId %}
      let table = new DataTable('#{{ datatableId }}', {
            columnDefs: [
                  { targets: [0], searchable: false, orderable: false}
               ]
         });
      {% endif %}
   });

</script>
{% endblock %}