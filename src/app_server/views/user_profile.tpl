{% extends "layouts/layout_vertical.tpl" %}

{% block pageContent %}
            <form action="{{ pageBaseUrl }}/api/users/update_profile_with_image" id="form_user_profile" enctype="multipart/form-data" method="post" accept-charset="utf-8">
            <div class="card mb-4">
              <div class="card-header">
                <i class="fas fa-table me-1"></i> {{ userData.firstName }} {{ userData.lastName }}  Profile
              </div>
              <div class="card-body">
                <input type="hidden" id="f_id" name="userId" value="{{ userData.id }}">
                <div class="row">
                  <!-- left side -->
                  <div class="col-sm-6">
                    <div class="form-group row mb-1">
                      <label for="f_username" class="col-sm-2 col-form-label">User name</label>
                      <div class="col-sm-10">
                          <input type="text" class="form-control form-control-sm" id="f_username" name="userName" readonly placeholder="" value="{{ userData.userName }}">
                      </div>
                    </div>

                    <div class="form-group row mb-1">
                      <label for="f_firstname" class="col-sm-2 col-form-label">First name</label>
                      <div class="col-sm-10">
                          <input type="text" class="form-control form-control-sm" id="f_firstname" name="firstName" readonly placeholder="" value="{{ userData.firstName }}">
                      </div>
                    </div>

                    <div class="form-group row mb-1">
                      <label for="f_lastname" class="col-sm-2 col-form-label">Last name</label>
                      <div class="col-sm-10">
                          <input type="text" class="form-control form-control-sm" id="f_lastname" name="lastName" readonly placeholder="" value="{{ userData.lastName }}">
                      </div>
                    </div>

                    <div class="form-group row mb-1">
                      <label for="f_email" class="col-sm-2 col-form-label">Email</label>
                      <div class="col-sm-10">
                          <input type="email" class="form-control form-control-sm" id="f_email" name="emailAddress" readonly placeholder="" value="{{ userData.email }}">
                      </div>
                    </div>

                    <div class="form-group row mb-1">
                      <label for="f_birthdate" class="col-sm-2 col-form-label">Birthdate</label>
                      <div class="col-sm-10">
                          <input type="date" class="form-control form-control-sm" id="f_birthdate" name="birthDate" placeholder="" readonly
                            min="1920-01-01" max="2200-01-01" value="{{ userData.birthDate }}">
                      </div>
                    </div>

                    <div class="form-group row mb-1">
                      <label for="f_phone" class="col-sm-2 col-form-label">Phone</label>
                      <div class="col-sm-10">
                          <input type="text" class="form-control form-control-sm" id="f_phone" name="phone" placeholder="" readonly value="{{ userData.phone }}">
                      </div>
                    </div>

                    <div class="form-group row mb-1">
                      <label for="f_gender" class="col-sm-2 col-form-label">Gender</label>
                      <div class="col-sm-10">
                          <select class="form-select form-select-sm" readonly name="gender" id="f_gender">
                            <option value="M" {% if userData.gender == "M" %} selected="selected" {% endif %}>Male</option>
                            <option value="F" {% if userData.gender == "F" %} selected="selected" {% endif %}>Female</option>
                          </select>
                      </div>
                    </div>

                    <div class="form-group row mb-1">
                      <label for="f_address" class="col-sm-2 col-form-label">Address</label>
                      <div class="col-sm-10">
                          <input type="text" class="form-control form-control-sm" id="f_address" name="address" readonly placeholder="" value="{{ userData.address }}">
                      </div>
                    </div>

                    <div class="form-group row mb-1">
                      <label for="f_uniquecode" class="col-sm-2 col-form-label">Unique Code</label>
                      <div class="col-sm-10">
                          <input type="text" class="form-control form-control-sm" id="f_uniquecode" name="uniqueCode" readonly placeholder="" value="{{ userData.uniqueCode }}">
                      </div>
                    </div>

                    <div class="form-group row mb-1">
                      <label for="f_nik" class="col-sm-2 col-form-label">NIK</label>
                      <div class="col-sm-10">
                          <input type="text" class="form-control form-control-sm" id="f_nik" name="nik" readonly placeholder="" value="{{ userData.nik }}">
                      </div>
                    </div>

                    <!--div class="form-group row mb-1">
                      <label for="f_expired" class="col-sm-2 col-form-label">Expired date</label>
                      <div class="col-sm-10">
                          <input type="datetime-local" class="form-control form-control-sm" id="f_expired" placeholder="" readonly
                          min="2000-01-01" max="2100-01-01" value="{{ userData.expired }}">
                      </div>
                    </div-->

                  </div><!-- /left side -->


                  <!-- right side -->
                  <div class="col-sm-6">

                    <div class="card mb-2">
                      <div class="card-body text-center">
                          <img id="image_uploader_preview" class="rounded img-fluid mx-auto"
                              src="{{ appResourceUrl("images_user", userData.image) }}" width="250" height="275">
                          <div class="input-group mt-2">
                              <input type="file" class="form-control" name="_image" id="fileUploadInput">
                              <label class="input-group-text" for="fileUploadInput"></label>
                              <input type="hidden" value="{{ userData.image }}" name="_oldimage">
                          </div>
                      </div>
                   </div>


                  </div><!-- /right side -->
                </div>
              </div> <!-- ./card-body  -->
              <div class="modal-footer">
                <button type="submit" name="submit" class="btn btn-primary m-3">Submit</button>
              </div>
            </div>
           </form>

{% endblock %}


{% block pageVendorCSS %}
{% endblock %}


{% block pageCSS %}
{% endblock %}


{% block pageVendorJS %}
    {# Required for bootbox dialog, datatable, summernote #}
    <script src="{{ pageBaseUrl }}/vendor/jquery/jquery.min.js" crossorigin="anonymous"></script>
    <script src="{{ pageBaseUrl }}/vendor/bootbox_5.5.2/bootbox.min.js" crossorigin="anonymous"></script>
{% endblock %}

{% block pageJS %}
<script>

   // Function to preview image after validation
   function imageIsLoaded(e) {
      $('#image_uploader_preview').attr('src', e.target.result);
      $('#image_uploader_preview').attr('width', '175px');
      $('#image_uploader_preview').attr('height', '200px');
   };

   window.addEventListener('DOMContentLoaded', event => {

      // Function to preview image after validation
      $("#fileUploadInput").change(function() {
         var errMsg = "Hanya file gambar jpeg, jpg and png yang bisa diupload. Dengan ukuran file maksimum 253600 Byte";
         var maxFileSize = 253600;

         var file = this.files[0];
         var imagefile = file.type;
         var match = ["image/jpeg", "image/png", "image/jpg"];
         if (!((imagefile == match[0])||(imagefile == match[1])||(imagefile == match[2]))|| (file.size >  maxFileSize)) {
            $("#fileUploadInput").val('');   // reset value
            alert(errMsg);
         } else {
            var reader = new FileReader();
            reader.onload = imageIsLoaded;
            reader.readAsDataURL(this.files[0]);
         }
      });


      var formProfile = document.getElementById('form_user_profile');
      formProfile.onsubmit = async (e) => {
         e.preventDefault();

         const form = e.currentTarget;
         const url = form.action;
         try {
            const formData = new FormData(form);
            TBS.apiSend(url, 'POST', formData, function(data) {
                if (data.code == 200) {
                  //window.location.href = TBS.baseUrl + '/admin/menus';
                  location.reload();
                } else {
                  TBS.alert.error(data.message, 'Toast', '');
                }
            }, false );
         } catch (error) {
            console.error(error);
         }
      };


   });

</script>
{% endblock %}
