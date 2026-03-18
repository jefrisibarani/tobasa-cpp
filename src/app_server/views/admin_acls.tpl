{% extends "layouts/layout_vertical.tpl" %}

{% block pageContent %}
            <!-- Main content -->
            <div class="col-md-12">
              <div class="card">
                <div class="card-header">
                  <h3 class="card-title" style="font-size : 1.4em;">Access control list</h3>
                </div>
                <!-- /.card-header -->
                <div class="card-body p-2 table-scrollable-x">
                  <table id="{{ default(datatableId, "") }}" class="table table-sm table-bordered table-stripped table-hover">
                    <thead>
                      <tr>
                        <th style="width: 80px">Action</th>
                        <th style="width: 10px">No</th>
                        <th>User/role</th>
                        <th>Acl Item</th>
                        <th>Acl Group</th>
                        <th>Acl Type</th>
                        <th>Page url</th>
                        <th>All</th>
                        <th>Index</th>
                        <th>Add</th>
                        <th>Delete</th>
                        <th>Update</th>
                        <th>Print</th>
                        <th>Other</th>
                      </tr>
                    </thead>
                    <tbody>
                      {% set idx=0 %}

                      {% if length(dataAcls) == 0 %}
                      <tr>
                        <td colspan="14">
                          <a href="#" onclick="showFormAdd('0');"  data-bs-toggle="tooltip" data-bs-placement="top" title="New data"><i class="fas fa-plus"></i></a>
                        </td>
                      </tr>
                      {% endif %}

                      {% for item in dataAcls %}
                      {% set idx=idx+1 %}
                      <tr>
                        <td>
                          <a href="#" onclick="showFormEdit('{{idx}}');" data-bs-toggle="tooltip" data-bs-placement="top" title="Edit data"><i class="fas fa-edit"></i></a>
                          <a href="#" onclick="showFormDelete('{{idx}}');"  data-bs-toggle="tooltip" data-bs-placement="top" title="Delete data"><i class="fas fa-trash"></i></a>
                          <a href="#" onclick="showFormAdd('0');"  data-bs-toggle="tooltip" data-bs-placement="top" title="New data"><i class="fas fa-plus"></i></a>
                        </td>
                        <th scope="row">{{ idx }}</th>
                        <input type="hidden" id="id_id_{{idx}}" value="{{ item.id}}"/>
                        <input type="hidden" id="id_ugId_{{idx}}" value="{{ item.ugId}}"/>
                        <input type="hidden" id="id_ugType_{{idx}}" value="{{ item.ugType}}"/>
                        <input type="hidden" id="id_menuId_{{idx}}" value="{{ item.menuId}}"/>

                        <td id='id_ugName_{{idx}}'>{{ item.ugName }}</td>
                        <td id='id_menuName_{{idx}}'>{{ item.menuName }}</td>
                        <td id='id_menuGroup_{{idx}}'>{{ item.menuGroup }}</td>
                        <td id='id_menuType_{{idx}}'>{{ item.menuType }}</td>

                        <td id='id_pageurl_{{idx}}'>{{ item.pageUrl }}</td>
                        <td id='id_aAll_{{idx}}'>{{ item.aAll }}</td>
                        <td id='id_aIndex_{{idx}}'>{{ item.aIndex }}</td>
                        <td id='id_aAdd_{{idx}}'>{{ item.aAdd }}</td>

                        <td id='id_aDelete_{{idx}}'>{{ item.aDelete }}</td>
                        <td id='id_aUpdate_{{idx}}'>{{ item.aUpdate }}</td>
                        <td id='id_aPrint_{{idx}}'>{{ item.aPrint }}</td>
                        <td id='id_aOther_{{idx}}'>{{ item.aOther }}</td>
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
         dialogCaption = "Add new acl";

      if (task === 'UPDATE')
         dialogCaption = "Edit acl";

      if (task === 'DELETE')
         dialogCaption = "Delete acl?";

      var dialog = bootbox.dialog({
         title: dialogCaption,
         size: 'medium',
         message: "Form Acl",
         onEscape: true,
         backdrop: true,
         buttons: {
            cancel: {label: '<i class="fas fa-times"></i> Cancel', className: 'btn-secondary' },
            ok: {
               label: '<i class="fas fa-check"></i> Submit',
               className: 'btn-primary',
               callback: function () {
                  let _id  = document.querySelector('#f_id').value * 1;
                  let _ugId = _ugName = _ugType = "";

                  if (task === 'INSERT') {
                     let rawUgId = document.querySelector('#f_ugId').value; // G_1  or U_1
                     _ugId   = rawUgId.substring(2);
                     _ugType = rawUgId.substring(0,1);
                     _ugName = "";
                  }
                  else {
                     _ugId   = document.querySelector('#f_ugId').value;
                     _ugName = document.querySelector('#f_ugName').value;
                     _ugType = document.querySelector('#f_ugType').value;
                     _ugType = (_ugType=="User") ? 'U' : 'G';
                  }

                  let _menuId    = document.querySelector('#f_menuId').value * 1;
                  let _aAll      = document.querySelector('#f_aAll').value=='1'    ? true : false;
                  let _aIndex    = document.querySelector('#f_aIndex').value=='1'  ? true : false;
                  let _aAdd      = document.querySelector('#f_aAdd').value=='1'    ? true : false;
                  let _aDelete   = document.querySelector('#f_aDelete').value=='1' ? true : false;
                  let _aUpdate   = document.querySelector('#f_aUpdate').value=='1' ? true : false;
                  let _aPrint    = document.querySelector('#f_aPrint').value=='1'  ? true : false;
                  let _aOther    = document.querySelector('#f_aOther').value;

                  let payload = {
                     id:      (_id     == null) ? 0  : _id,
                     ugId:    (_ugId   == null) ? 0  : (_ugId*1),
                     ugType:  (_ugType == null) ? '' : _ugType,
                     menuId:  (_menuId == null) ? 0  : _menuId,
                     aAll:    _aAll,
                     aIndex:  _aIndex,
                     aAdd:    _aAdd,
                     aDelete: _aDelete,
                     aUpdate: _aUpdate,
                     aPrint:  _aPrint,
                     aOther:  (_aOther == null) ? '' : _aOther
                  };

                  let httpMethod = 'POST';
                  if (task === 'DELETE') {
                     httpMethod = 'DELETE';
                     payload = { id : _id };
                  }

                  let apiCallOk = false;
                  TBS.apiSend(TBS.baseUrl + '/api/admin/acl/', httpMethod, JSON.stringify(payload), function(data) {
                     if (data.code == 200) {
                        window.location.href = TBS.baseUrl + '/admin/acl';
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
            let valId = valUgId = valUgName = valUgType = valMenuId = valMenuName = valMenuGroup = valMenuType = "";
            let valPageurl = valAll = valIndex = valAdd = valDelete = valUpdate = valPrint = valOther = "";
            if (task === 'INSERT') {
               valItemId = 0;
            } else {
               valId        = document.querySelector('#id_id_'+ idx ).value;
               valUgId      = document.querySelector('#id_ugId_'+ idx ).value;
               valUgName    = document.querySelector('#id_ugName_'+ idx ).innerHTML;
               valUgType    = document.querySelector('#id_ugType_'+ idx ).value;
               valMenuId    = document.querySelector('#id_menuId_'+ idx ).value;
               valMenuName  = document.querySelector('#id_menuName_'+ idx ).innerHTML;
               valMenuGroup = document.querySelector('#id_menuGroup_'+ idx ).innerHTML;
               valMenuType  = document.querySelector('#id_menuType_'+ idx ).innerHTML;
               valPageurl   = document.querySelector('#id_pageurl_'+ idx ).innerHTML;
               valAll       = document.querySelector('#id_aAll_'+ idx ).innerHTML;
               valIndex     = document.querySelector('#id_aIndex_'+ idx ).innerHTML;
               valAdd       = document.querySelector('#id_aAdd_'+ idx ).innerHTML;
               valDelete    = document.querySelector('#id_aDelete_'+ idx ).innerHTML;
               valUpdate    = document.querySelector('#id_aUpdate_'+ idx ).innerHTML;
               valPrint     = document.querySelector('#id_aPrint_'+ idx ).innerHTML;
               valOther     = document.querySelector('#id_aOther_'+ idx ).innerHTML;
            }

            let readonlyMark = "";
            if (task === 'DELETE')
               readonlyMark = "readonly" ;

            let userRoleCaption = '';
            if (valUgType=="User")
               userRoleCaption = "User name";
            else
               userRoleCaption = "Role name";

            let userRoleElements = '';
            let menuElements = '';
            let detailElements = '';

            if (task === 'INSERT') {
               menuElements =
               `<div class="form-group row mb-1">
                  <label for="f_menuId" class="col-sm-3 col-form-label">ACL Item</label>
                  <div class="col-sm-9">
                     {{ comboBox(dataMenuViews, "f_menuId", "f_menuId", "form-select", "", "ID_CODE") }}
                  </div>
               </div>
               `;

               userRoleElements =
               `<input type="hidden" id="f_ugType" value="">
               <div class="form-group row mb-1">
                  <label for="f_ugId" class="col-sm-3 col-form-label">User/Role</label>
                  <div class="col-sm-9">
                     {{ comboBox(dataRolesUAndsers, "f_ugId", "f_ugId", "form-select", "", "ID_CODE") }}
                  </div>
               </div>
               `;
            }
            else {
               menuElements =
               `<input type="hidden" id="f_ugId" value="${valUgId}">
               <input type="hidden" id="f_ugType" value="${valUgType}">
               <input type="hidden" id="f_menuId" value="${valMenuId}">
               <div class="form-group row mb-1">
                  <label for="f_menuName" class="col-sm-3 col-form-label">Acl Item</label>
                  <div class="col-sm-9">
                     <input type="text" class="form-control" id="f_menuName" readonly value="${valMenuName}">
                  </div>
               </div>
               <div class="form-group row mb-1">
                  <label for="f_menuGroup" class="col-sm-3 col-form-label">Acl Group</label>
                  <div class="col-sm-9">
                     <input type="text" class="form-control" id="f_menuGroup" readonly value="${valMenuGroup}">
                  </div>
               </div>
               <div class="form-group row mb-1">
                  <label for="f_menuType" class="col-sm-3 col-form-label">Acl Type</label>
                  <div class="col-sm-9">
                     <input type="text" class="form-control" id="f_menuType" readonly value="${valMenuType}">
                  </div>
               </div>
               <div class="form-group row mb-1">
                  <label for="f_pageurl" class="col-sm-3 col-form-label">Page Url</label>
                  <div class="col-sm-9">
                     <input type="text" class="form-control" id="f_pageurl" readonly value="${valPageurl}">
                  </div>
               </div>
               `;

               userRoleElements =
               `<div class="form-group row mb-1">
                  <label for="f_ugName" class="col-sm-3 col-form-label">${userRoleCaption}</label>
                  <div class="col-sm-9">
                  <input type="text" class="form-control" id="f_ugName" readonly value="${valUgName}">
                  </div>
               </div>
               `;
            }

            if (task !== 'DELETE') {
               detailElements = 
               `  <div class="form-group row mb-1">
                     <label for="f_aAll" class="col-sm-3 col-form-label">All</label>
                     <div class="col-sm-9">
                        <select class="form-select" name="f_aAll" id="f_aAll" ${readonlyMark}>
                           <option value="1" ${valAll=='true' ? 'selected="selected"':''}>TRUE</option>
                           <option value="0" ${valAll=='false' ? 'selected="selected"':''}>FALSE</option>
                        </select>
                     </div>
                  </div>
                  <div class="form-group row mb-1">
                     <label for="f_aIndex" class="col-sm-3 col-form-label">Index</label>
                     <div class="col-sm-9">
                        <select class="form-select" name="f_aIndex" id="f_aIndex" ${readonlyMark}>
                           <option value="1" ${valIndex=='true' ? 'selected="selected"':''}>TRUE</option>
                           <option value="0" ${valIndex=='false' ? 'selected="selected"':''}>FALSE</option>
                        </select>
                     </div>
                  </div>
                  <div class="form-group row mb-1">
                     <label for="f_aAdd" class="col-sm-3 col-form-label">Add</label>
                     <div class="col-sm-9">
                        <select class="form-select" name="f_aAdd" id="f_aAdd" ${readonlyMark}>
                           <option value="1" ${valAdd=='true' ? 'selected="selected"':''}>TRUE</option>
                           <option value="0" ${valAdd=='false' ? 'selected="selected"':''}>FALSE</option>
                        </select>
                     </div>
                  </div>
                  <div class="form-group row mb-1">
                     <label for="f_aDelete" class="col-sm-3 col-form-label">Delete</label>
                     <div class="col-sm-9">
                        <select class="form-select" name="f_aDelete" id="f_aDelete" ${readonlyMark}>
                           <option value="1" ${valDelete=='true' ? 'selected="selected"':''}>TRUE</option>
                           <option value="0" ${valDelete=='false' ? 'selected="selected"':''}>FALSE</option>
                        </select>
                     </div>
                  </div>
                  <div class="form-group row mb-1">
                     <label for="f_aUpdate" class="col-sm-3 col-form-label">Update</label>
                     <div class="col-sm-9">
                        <select class="form-select" name="f_aUpdate" id="f_aUpdate" ${readonlyMark}>
                           <option value="1" ${valUpdate=='true' ? 'selected="selected"':''}>TRUE</option>
                           <option value="0" ${valUpdate=='false' ? 'selected="selected"':''}>FALSE</option>
                        </select>
                     </div>
                  </div>
                  <div class="form-group row mb-1">
                     <label for="f_aPrint" class="col-sm-3 col-form-label">Print</label>
                     <div class="col-sm-9">
                        <select class="form-select" name="f_aPrint" id="f_aPrint" ${readonlyMark} >
                           <option value="1" ${valPrint=='true' ? 'selected="selected"':''}>TRUE</option>
                           <option value="0" ${valPrint=='false' ? 'selected="selected"':''}>FALSE</option>
                        </select>
                     </div>
                  </div>
                  <div class="form-group row mb-1">
                     <label for="f_aOther" class="col-sm-3 col-form-label">Other</label>
                     <div class="col-sm-9">
                        <input type="text" class="form-control" id="f_aOther" placeholder="" value="${valOther}" ${readonlyMark}>
                     </div>
                  </div>
               `;
            }

            dialog.find('.bootbox-body').html(
            `
            <div class="card-body">
               <input type="hidden" id="f_id" value="${valId}">
               ${userRoleElements}
               ${menuElements}
               ${detailElements}
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

   window.addEventListener('DOMContentLoaded', event => {
      {% if length(dataAcls) > 0 %}
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