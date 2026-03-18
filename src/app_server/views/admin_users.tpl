{% extends "layouts/layout_vertical.tpl" %}

{% block pageContent %}
            <!-- Main content -->
            <div class="col-md-12">
              <div class="card">
                <div class="card-header">
                  <h3 class="card-title" style="font-size : 1.4em;">Users</h3>
                </div>
                <!-- /.card-header -->
                <div class="card-body p-2 table-scrollable-x">
                  <table id="{{ default(datatableId, "") }}" class="table table-sm table-bordered table-stripped table-hover">
                    <thead>
                      <tr>
                        <th style="width: 90px">Action</th>
                        <th style="width: 10px">No</th>
                        <th>User name</th>
                        <th>First name</th>
                        <th>Last name</th>
                        <th>Email</th>
                        <th>Enabled</th>
                        <th>Can Login</th>
                        <th>Birthdate</th>
                        <th>Phone</th>
                        <th>Gender</th>
                        <th>Address</th>
                        <th>Unique Code</th>
                        <th>NIK</th>
                        <th>Expired Date</th>
                      </tr>
                    </thead>
                    <tbody>
                      {% set idx=0 %}

                      {% if length(users) == 0 %}
                      <tr>
                        <td colspan="12">
                          <a href="#" onclick="showFormAdd('0');"  data-bs-toggle="tooltip" data-bs-placement="top" title="New data"><i class="fas fa-plus"></i></a>
                        </td>
                      </tr>
                      {% endif %}

                      {% for item in users %}
                      {% set idx=idx+1 %}
                      <tr>
                        <td>
                          <a href="#" onclick="showFormEdit('{{idx}}');" data-bs-toggle="tooltip" data-bs-placement="top" title="Edit user"><i class="fas fa-edit"></i></a>&nbsp;
                          <a href="#" onclick="showFormDelete('{{idx}}');"  data-bs-toggle="tooltip" data-bs-placement="top" title="Delete user"><i class="fas fa-trash"></i></a>&nbsp;
                          <a href="#" onclick="showFormChangePassword('{{idx}}');"  data-bs-toggle="tooltip" data-bs-placement="top" title="Reset password"><i class="fas fa-key"></i></a>&nbsp;
                          <a href="#" onclick="showFormAdd('0');"  data-bs-toggle="tooltip" data-bs-placement="top" title="New user"><i class="fas fa-plus"></i></a>&nbsp;
                        </td>
                        <th scope="row">{{ idx }}</th>
                        <input type="hidden" id="id_item_id_{{idx}}" value="{{ item.id}}"/>
                        <td id='id_item_username_{{idx}}'>{{ item.userName }}</td>
                        <td id='id_item_firstname_{{idx}}'>{{ item.firstName }}</td>
                        <td id='id_item_lastname_{{idx}}'>{{ item.lastName }}</td>
                        <td id='id_item_email_{{idx}}'>{{ item.email }}</td>
                        <td id='id_item_enabled_{{idx}}'>{{ item.enabled }}</td>
                        <td id='id_item_allowlogin_{{idx}}'>{{ item.allowLogin }}</td>
                        <td id='id_item_birthdate_{{idx}}'>{{ item.birthDate }}</td>
                        <td id='id_item_phone_{{idx}}'>{{ item.phone }}</td>
                        <td id='id_item_gender_{{idx}}'>{{ item.gender }}</td>
                        <td id='id_item_address_{{idx}}'>{{ item.address }}</td>
                        <td id='id_item_uniquecode_{{idx}}'>{{ item.uniqueCode }}</td>
                        <td id='id_item_nik_{{idx}}'>{{ item.nik }}</td>
                        <td id='id_item_expired_{{idx}}'>{{ item.expired }}</td>
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
         dialogCaption = "Add new user";

      if (task === 'UPDATE')
         dialogCaption = "Edit user";

      if (task === 'DELETE')
         dialogCaption = "Delete user?";

      var dialog = bootbox.dialog({
         title: dialogCaption,
         size: 'extra-large',
         message: "Form Users",
         onEscape: true,
         backdrop: true,
         buttons: {
            cancel: {label: '<i class="fas fa-times"></i> Cancel', className: 'btn-secondary' },
            ok: {
               label: '<i class="fas fa-check"></i> Submit',
               className: 'btn-primary',
               callback: function () {

                  let valItemId   = valUsername = valFirstname  = valLastname   = "";
                  let valEmail    = valEnabled  = valAllowLogin = valBirthdate  = "";
                  let valPhone    = valGender   = valAddress    = valUniqueCode = valNik = "";
                  let valImage    = valExpired  = "";
                  let valPassword = "";

                  valItemId     = document.querySelector('#f_id').value * 1;
                  valUsername   = document.querySelector('#f_username').value;
                  valFirstname  = document.querySelector('#f_firstname').value;
                  valLastName   = document.querySelector('#f_lastname').value;
                  valEmail      = document.querySelector('#f_email').value;
                  valEnabled    = document.querySelector('#f_enabled').value;
                  valAllowLogin = document.querySelector('#f_allowlogin').value;
                  valBirthdate  = document.querySelector('#f_birthdate').value;
                  valPhone      = document.querySelector('#f_phone').value;
                  valGender     = document.querySelector('#f_gender').value;
                  valAddress    = document.querySelector('#f_address').value;
                  valUniqueCode = document.querySelector('#f_uniquecode').value;
                  valNik        = document.querySelector('#f_nik').value;
                  valExpired    = document.querySelector('#f_expired').value;

                  if (task === 'INSERT') {
                     let pass1 = document.querySelector('#f_password1').value;
                     let pass2 = document.querySelector('#f_password2').value;
                     if (pass1 === pass2)
                        valPassword = pass1;
                     else {
                        TBS.alert.error("Password does not match", 'Toast', '');
                        // return false, to prevent dialog dismissed
                        return false;
                     }
                  }

                  valExpired = TBS.reformatDatetime(valExpired);
                  let payload = {
                     id:            (valItemId == null) ? 0 : valItemId,
                     uuid:          '',
                     userName:      (valUsername == null) ? '' : valUsername,
                     firstName:     (valFirstname == null) ? '' : valFirstname,
                     lastName:      (valLastName == null) ? '' : valLastName,
                     email:         (valEmail == null) ? '' : valEmail,
                     image:         (valImage == null) ? '' : valImage,
                     enabled:       (valEnabled == '1') ? true : false,
                     passwordSalt:  "",
                     passwordHash:  "",
                     allowLogin:    (valAllowLogin == '1') ? true : false,
                     created:       "",
                     updated:       "",
                     expired:       (valExpired == null) ? '' : valExpired,
                     lastLogin:     "",
                     uniqueCode:    (valUniqueCode == null) ? '' : valUniqueCode,
                     birthDate:     (valBirthdate == null) ? '' : valBirthdate,
                     phone:         (valPhone == null) ? '' : valPhone,
                     gender:        (valGender == null) ? '' : valGender,
                     address:       (valAddress == null) ? '' : valAddress,
                     nik:           (valNik == null) ? '' : valNik,
                  };

                  let url = TBS.baseUrl + '/api/admin/users/';
                  let httpMethod = 'POST';
                  if (task === 'DELETE') {
                     httpMethod = 'DELETE';
                     payload = { id : valItemId };
                  }

                  if (task === 'INSERT') {
                     url += '?password=' + valPassword;
                  }

                  let apiCallOk = false;
                  TBS.apiSend(url, httpMethod, JSON.stringify(payload), function(data) {
                     if (data.code == 200) {
                        window.location.href = TBS.baseUrl + '/admin/users';
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
            let valItemId = valUsername = valFirstname = valLastname = "";
            let valEmail = valEnabled = valAllowLogin = valBirthdate = "";
            let valPhone = valGender = valAddress = valUniqueCode = valNik = "";
            let valImage = valExpired = "";
            let htmlElementPassword = "";

            if (task === 'INSERT') {
               htmlElementPassword =
               `  <div class="form-group row mb-1">
                     <label for="f_password1" class="col-sm-3 col-form-label">Password</label>
                     <div class="col-sm-9">
                        <input type="password" class="form-control" id="f_password1" placeholder="">
                     </div>
                  </div>
                  <div class="form-group row mb-1">
                     <label for="f_password2" class="col-sm-3 col-form-label">Confirm Password</label>
                     <div class="col-sm-9">
                        <input type="password" class="form-control" id="f_password2" placeholder="">
                     </div>
                  </div>
               `;

               valItemId = 0;
            }
            else {
               valItemId     = document.querySelector('#id_item_id_'+ idx ).value;
               valUsername   = document.querySelector('#id_item_username_'+ idx ).innerHTML;
               valFirstname  = document.querySelector('#id_item_firstname_'+ idx ).innerHTML;
               valLastname   = document.querySelector('#id_item_lastname_'+ idx ).innerHTML;
               valEmail      = document.querySelector('#id_item_email_'+ idx ).innerHTML;
               valEnabled    = document.querySelector('#id_item_enabled_'+ idx ).innerHTML;
               valAllowLogin = document.querySelector('#id_item_allowlogin_'+ idx ).innerHTML;
               valBirthdate  = document.querySelector('#id_item_birthdate_'+ idx ).innerHTML;
               valPhone      = document.querySelector('#id_item_phone_'+ idx ).innerHTML;
               valGender     = document.querySelector('#id_item_gender_'+ idx ).innerHTML;
               valAddress    = document.querySelector('#id_item_address_'+ idx ).innerHTML;
               valUniqueCode = document.querySelector('#id_item_uniquecode_'+ idx ).innerHTML;
               valNik        = document.querySelector('#id_item_nik_'+ idx ).innerHTML;
               valExpired    = document.querySelector('#id_item_expired_'+ idx ).innerHTML;
            }

            let readonlyMark = "";
            if (task === 'DELETE')
               readonlyMark = "readonly";

            dialog.find('.bootbox-body').html(
            `<div class="card-body">
               <input type="hidden" id="f_id" value="${valItemId}">
               <div class="row">
                  <!-- left side -->
                  <div class="col-sm-6">
                     <div class="form-group row mb-1">
                        <label for="f_username" class="col-sm-3 col-form-label">User name</label>
                        <div class="col-sm-9">
                           <input type="text" class="form-control" id="f_username" ${readonlyMark} placeholder="" value="${valUsername}">
                        </div>
                     </div>

                     <div class="form-group row mb-1">
                        <label for="f_firstname" class="col-sm-3 col-form-label">First name</label>
                        <div class="col-sm-9">
                           <input type="text" class="form-control" id="f_firstname" ${readonlyMark} placeholder="" value="${valFirstname}">
                        </div>
                     </div>

                     <div class="form-group row mb-1">
                        <label for="f_lastname" class="col-sm-3 col-form-label">Last name</label>
                        <div class="col-sm-9">
                           <input type="text" class="form-control" id="f_lastname" ${readonlyMark} placeholder="" value="${valLastname}">
                        </div>
                     </div>

                     <div class="form-group row mb-1">
                        <label for="f_email" class="col-sm-3 col-form-label">Email</label>
                        <div class="col-sm-9">
                           <input type="email" class="form-control" id="f_email" ${readonlyMark} placeholder="" value="${valEmail}">
                        </div>
                     </div>

                     <div class="form-group row mb-1">
                        <label for="f_enabled" class="col-sm-3 col-form-label">Enabled</label>
                        <div class="col-sm-9">
                           <select class="form-select" ${readonlyMark} name="f_enabled" id="f_enabled">
                              <option value="1" ${valEnabled=='true' ? 'selected="selected"':''}>TRUE</option>
                              <option value="0" ${valEnabled=='false' ? 'selected="selected"':''}>FALSE</option>
                           </select>
                        </div>
                     </div>

                     <div class="form-group row mb-1">
                        <label for="f_allowlogin" class="col-sm-3 col-form-label">Can login</label>
                        <div class="col-sm-9">
                           <select class="form-select" ${readonlyMark} name="f_allowlogin" id="f_allowlogin">
                              <option value="1" ${valAllowLogin=='true' ? 'selected="selected"':''}>TRUE</option>
                              <option value="0" ${valAllowLogin=='false' ? 'selected="selected"':''}>FALSE</option>
                           </select>
                        </div>
                     </div>
                     ${htmlElementPassword}

                  </div>

                  <!-- right side -->
                  <div class="col-sm-6">

                     <div class="form-group row mb-1">
                        <label for="f_birthdate" class="col-sm-3 col-form-label">Birthdate</label>
                        <div class="col-sm-9">
                           <input type="date" class="form-control" id="f_birthdate" placeholder="" ${readonlyMark}
                              min="1920-01-01" max="2100-01-01" value="${valBirthdate}">
                        </div>
                     </div>

                     <div class="form-group row mb-1">
                        <label for="f_phone" class="col-sm-3 col-form-label">Phone</label>
                        <div class="col-sm-9">
                           <input type="text" class="form-control" id="f_phone" placeholder="" ${readonlyMark} value="${valPhone}">
                        </div>
                     </div>

                     <div class="form-group row mb-1">
                        <label for="f_gender" class="col-sm-3 col-form-label">Gender</label>
                        <div class="col-sm-9">
                           <select class="form-select" ${readonlyMark} name="f_gender" id="f_gender">
                              <option value="M" ${valGender=='M' ? 'selected="selected"':''}>Male</option>
                              <option value="F" ${valGender=='F' ? 'selected="selected"':''}>Female</option>
                           </select>
                        </div>
                     </div>

                     <div class="form-group row mb-1">
                        <label for="f_address" class="col-sm-3 col-form-label">Address</label>
                        <div class="col-sm-9">
                           <input type="text" class="form-control" id="f_address" ${readonlyMark} placeholder="" value="${valAddress}">
                        </div>
                     </div>

                     <div class="form-group row mb-1">
                        <label for="f_uniquecode" class="col-sm-3 col-form-label">Unique Code</label>
                        <div class="col-sm-9">
                           <input type="text" class="form-control" id="f_uniquecode" ${readonlyMark} placeholder="" value="${valUniqueCode}">
                        </div>
                     </div>

                     <div class="form-group row mb-1">
                        <label for="f_nik" class="col-sm-3 col-form-label">NIK</label>
                        <div class="col-sm-9">
                           <input type="text" class="form-control" id="f_nik" ${readonlyMark} placeholder="" value="${valNik}">
                        </div>
                     </div>

                     <div class="form-group row mb-1">
                        <label for="f_expired" class="col-sm-3 col-form-label">Expired date</label>
                        <div class="col-sm-9">
                           <input type="datetime-local" class="form-control" id="f_expired" placeholder="" ${readonlyMark}
                           min="2000-01-01" max="2100-01-01" value="${valExpired}">
                        </div>
                     </div>

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

   function showFormChangePassword(idx) {
      let dialog = bootbox.dialog({
         title: 'Reset user password',
         message: "Reset user password",
         size: 'medium',
         onEscape: true,
         backdrop: true,
         buttons: {
            cancel: {label: '<i class="fas fa-times"></i> Cancel', className: 'btn-secondary' },
            ok: {
               label: '<i class="fas fa-check"></i> Submit',
               className: 'btn-primary',
               callback: function() {
                  let newPassword = "";
                  let pwd1 = document.querySelector('#f_resetPwd1').value;
                  let pwd2 = document.querySelector('#f_resetPwd2').value;
                  if (pwd1 === pwd2)
                     newPassword = pwd1;
                  else {
                     TBS.alert.error("Password does not match", 'Toast', '');
                     // return false, to prevent dialog dismissed
                     return false;
                  }

                  let userIdReset = document.querySelector('#f_uid_password').value;

                  let payload =
                  {
                     userId:     (userIdReset == null) ? 0 :userIdReset * 1,
                     password:   (newPassword == null) ? "" :newPassword,
                  };

                  //let apiCallOk = false;
                  TBS.apiSend(TBS.baseUrl + '/api/admin/users/reset_password', 'POST', JSON.stringify(payload), function(data) {
                     if (data.code == 200) {
                        TBS.alert.info("Password reset successfully", 'Toast', '');

                     } else {
                        TBS.alert.error(data.message, 'Toast', '');

                     }
                  });
               }
            }
         }

      });

      dialog.init(function() {
         let valUserId = document.querySelector('#id_item_id_'+ idx ).value;

         dialog.find('.bootbox-body').html(
         `
         <input type="hidden" id="f_uid_password" value="${valUserId}">
         <div class="form-group row mb-1">
            <label for="f_resetPwd1" class="col-sm-3 col-form-label">Password</label>
            <div class="col-sm-9">
               <input type="password" class="form-control" id="f_resetPwd1" placeholder="">
            </div>
         </div>
         <div class="form-group row mb-1">
            <label for="f_resetPwd2" class="col-sm-3 col-form-label">Confirm Password</label>
            <div class="col-sm-9">
               <input type="password" class="form-control" id="f_resetPwd2" placeholder="">
            </div>
         </div>
         `
         );

      });

      dialog.show('modal');
   }

   window.addEventListener('DOMContentLoaded', event => {
      {% if length(users) > 0 and datatableId %}
      let table = new DataTable('#{{ datatableId }}', {
            columnDefs: [
                  { targets: [0], searchable: false, orderable: false}
               ]
         });
      {% endif %}
   });

</script>
{% endblock %}