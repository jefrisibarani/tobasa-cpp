{% extends "layouts/layout_vertical.tpl" %}

{% block pageContent %}
            <!-- Main content -->
            <div class="col-md-12">
              <div class="card">
                <div class="card-header">
                  <h3 class="card-title" style="font-size : 1.4em;">Menus</h3>
                </div>
                <!-- /.card-header -->
                <div class="card-body p-2 table-scrollable-x">
                  <table id="{{ default(datatableId, "") }}" class="table table-sm table-bordered table-stripped table-hover">
                    <thead>
                      <tr>
                        <th style="width: 80px">Action</th>
                        <th style="width: 10px">No</th>
                        <th>Name</th>
                        <th>Type</th>
                        <th>Group</th>
                        <th>Url</th>
                        <th>Sidebar</th>
                        <th>Methods</th>
                      </tr>
                    </thead>
                    <tbody>
                      {% set idx=0 %}

                      {% if length(menus) == 0 %}
                      <tr>
                        <td colspan="8">
                          <a href="#" onclick="showFormAdd('0');"  data-bs-toggle="tooltip" data-bs-placement="top" title="New data"><i class="fas fa-plus"></i></a>
                        </td>
                      </tr>
                      {% endif %}

                      {% for item in menus %}
                      {% set idx=idx+1 %}
                      <tr>
                        <td>
                          <a href="#" onclick="showFormEdit('{{idx}}');" data-bs-toggle="tooltip" data-bs-placement="top" title="Edit data"><i class="fas fa-edit"></i></a>
                          <a href="#" onclick="showFormDelete('{{idx}}');"  data-bs-toggle="tooltip" data-bs-placement="top" title="Delete data"><i class="fas fa-trash"></i></a>
                          <a href="#" onclick="showFormAdd('0');"  data-bs-toggle="tooltip" data-bs-placement="top" title="New data"><i class="fas fa-plus"></i></a>
                        </td>
                        <th scope="row">{{ idx }}</th>
                        <input type="hidden" id="id_item_id_{{idx}}" value="{{ item.id}}"/>
                        <input type="hidden" id="id_item_menuType_{{idx}}" value="{{ item.typeId}}"/>
                        <input type="hidden" id="id_item_menuGroup_{{idx}}" value="{{ item.groupId}}"/>
                        <td id='id_item_name_{{idx}}'>{{ item.name }}</td>
                        
                        <td>
                           {{ lookupArray(menuTypes, "ID_CODE", item.typeId) }}
                        </td>
                        <td>
                           {{ lookupArray(menuGroups, "ID_CODE", item.groupId ) }}
                        </td>
                        
                        <td id='id_item_pageurl_{{idx}}'>{{ item.pageurl }}</td>
                        <td id='id_item_sidebar_{{idx}}'>{{ item.sidebar }}</td>
                        <td id='id_item_methods_{{idx}}'>{{ item.methods }}</td>
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
{% endblock %}


{% block pageJS %}
<script>
   function showForm(idx, task) {
      let dialogCaption = '';
      if (task === 'INSERT')
         dialogCaption = "Add new item";

      if (task === 'UPDATE')
         dialogCaption = "Edit item";

      if (task === 'DELETE')
         dialogCaption = "Delete item?";

      var dialog = bootbox.dialog({
         title: dialogCaption,
         size: 'medium',
         message: "Form Menus",
         onEscape: true,
         backdrop: true,
         buttons: {
            cancel: {label: '<i class="fas fa-times"></i> Cancel', className: 'btn-secondary' },
            ok: {
               label: '<i class="fas fa-check"></i> Submit',
               className: 'btn-primary',
               callback: function () {

                  let itemName = itemMenuType = itemMenuGroup = itemPageurl = itemMethods = "";
                  let itemSidebar = false;
                  let itemId = 0;

                  itemId        = document.querySelector('#f_id').value * 1;
                  itemName      = document.querySelector('#f_name').value;
                  itemMenuType  = document.querySelector('#f_menuType').value * 1;
                  itemMenuGroup = document.querySelector('#f_menuGroup').value * 1;
                  itemPageurl   = document.querySelector('#f_pageurl').value;
                  itemSidebar   = document.querySelector('#f_sidebar').value;
                  itemMethods   = document.querySelector('#f_methods').value;

                  let payload = {
                     id:      (itemId        == null) ? 0   : itemId,
                     name:    (itemName      == null) ? ''  : itemName,
                     typeId:  (itemMenuType  == null) ? 0   : itemMenuType,
                     groupId: (itemMenuGroup == null) ? 0   : itemMenuGroup,
                     pageurl: (itemPageurl   == null) ? ''  : itemPageurl,
                     sidebar: (itemSidebar   == '1') ? true : false,
                     methods: (itemMethods   == null) ? ''  : itemMethods
                  };

                  let httpMethod = 'POST';
                  if (task === 'DELETE') {
                     httpMethod = 'DELETE';
                     payload = { id : itemId };
                  }

                  let apiCallOk = false;
                  TBS.apiSend(TBS.baseUrl + '/api/admin/menus/', httpMethod, JSON.stringify(payload), function(data) {
                     if (data.code == 200) {
                        window.location.href = TBS.baseUrl + '/admin/menus';
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
            let valItemName = valItemMenuType = valItemMenuGroup = valItemPageurl = valItemMethods = "";
            let valItemId = valItemSidebar = "";

            if (task === 'INSERT') {
               valItemId = 0;
            } else {
               valItemId        = document.querySelector('#id_item_id_'+ idx ).value;
               valItemName      = document.querySelector('#id_item_name_'+ idx ).innerHTML;
               valItemMenuType  = document.querySelector('#id_item_menuType_'+ idx ).value;
               valItemMenuGroup = document.querySelector('#id_item_menuGroup_'+ idx ).value;
               valItemPageurl   = document.querySelector('#id_item_pageurl_'+ idx ).innerHTML;
               valItemSidebar   = document.querySelector('#id_item_sidebar_'+ idx ).innerHTML;
               valItemMethods   = document.querySelector('#id_item_methods_'+ idx ).innerHTML;
            }

            let readonlyMark = "";
            if (task === 'DELETE')
               readonlyMark = "readonly";

            dialog.find('.bootbox-body').html(
            `
            <div class="card-body">
               <input type="hidden" id="f_id" value="${valItemId}">
               <div class="form-group row mb-1">
                  <label for="f_name" class="col-sm-2 col-form-label">Name</label>
                  <div class="col-sm-10">
                     <input type="text" class="form-control" id="f_name" ${readonlyMark} placeholder="" value="${valItemName}">
                  </div>
               </div>
               <div class="form-group row mb-1">
                  <label for="f_menuType" class="col-sm-2 col-form-label">Type</label>
                  <div class="col-sm-10">
                     {{ comboBox(menuTypes, "f_menuType", "f_menuType", "form-select", "", "ID_CODE") }}
                  </div>
               </div>
               <div class="form-group row mb-1">
                  <label for="f_menuGroup" class="col-sm-2 col-form-label">Group</label>
                  <div class="col-sm-10">
                     {{ comboBox(menuGroups, "f_menuGroup", "f_menuGroup", "form-select", "", "ID_CODE") }}
                  </div>
               </div>
               <div class="form-group row mb-1">
                  <label for="f_pageurl" class="col-sm-2 col-form-label">Url</label>
                  <div class="col-sm-10">
                  <input type="text" class="form-control" id="f_pageurl" ${readonlyMark} placeholder="" value="${valItemPageurl}">
                  </div>
               </div>
               <div class="form-group row mb-1">
                  <label for="f_sidebar" class="col-sm-2 col-form-label">Sidebar</label>
                  <div class="col-sm-10">
                     <select class="form-select" ${readonlyMark} name="f_sidebar" id="f_sidebar">
                        <option value="1" ${valItemSidebar=='true' ? 'selected="selected"':''}>TRUE</option>
                        <option value="0" ${valItemSidebar=='false' ? 'selected="selected"':''}>FALSE</option>
                     </select>
                  </div>
               </div>
               <div class="form-group row mb-1">
                  <label for="f_methods" class="col-sm-2 col-form-label">Methods</label>
                  <div class="col-sm-10">
                     <input type="text" class="form-control" id="f_methods" ${readonlyMark} placeholder="" value="${valItemMethods}">
                  </div>
               </div>
            </div>`
            );

            if (task !== 'INSERT') {
               document.querySelector('#f_menuType').value = valItemMenuType;
               document.querySelector('#f_menuGroup').value = valItemMenuGroup;
            }

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


   window.addEventListener('DOMContentLoaded', event => {
      {% if length(menus) > 0 %}
      {% if datatableId %}
      let table = new DataTable('#{{ datatableId }}', {
            columnDefs: [
                  { targets: [0], searchable: false, orderable: false}
               ]
         });
      {% endif %}
      {% endif %}
   });

</script>
{% endblock %}