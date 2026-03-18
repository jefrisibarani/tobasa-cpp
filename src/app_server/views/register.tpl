{% extends "layouts/layout_basic.tpl" %}

{% block pageContent %}
          <div class="container">
            <div class="row justify-content-center">
              <div class="col-lg-7">

                <div id="alertsInToast" class="alertContainer mt-3"></div>
                <div id="alertsInPage" class="alertContainer  mt-3 d-none"></div>

                <div class="card shadow-lg border-0 rounded-lg mt-5">
                  <div class="card-header">
                    <h3 class="text-center font-weight-light my-4">Create Account</h3>
                  </div>
                  <div class="card-body">
                    <form action="{{ pageBaseUrl }}/register" method="post">
                      <div class="row mb-3">
                        <div class="col-md-6">
                          <div class="form-floating mb-3 mb-md-0">
                            <input class="form-control form-control-sm" name="firstName" id="inputFirstName" type="text" placeholder="Enter your first name" value="{{ default(userData.firstName, "") }}" />
                            <label for="inputFirstName">First name</label>
                          </div>
                        </div>
                        <div class="col-md-6">
                          <div class="form-floating">
                            <input class="form-control form-control-sm" name="lastName" id="inputLastName" type="text" placeholder="Enter your last name" value="{{ default(userData.lastName, "") }}" />
                            <label for="inputLastName">Last name</label>
                          </div>
                        </div>
                      </div>
                      <!--div class="form-floating mb-3">
                        <input class="form-control form-control-sm" name="loginName" id="inputEmail" type="tex" placeholder="name@example.com" />
                        <label for="inputEmail">Email address</label>
                      </div-->

                      <div class="row mb-3">
                        <div class="col-md-6">
                          <div class="form-floating mb-3 mb-md-0">
                            <input class="form-control form-control-sm" name="userName" id="inputUserName" type="text" placeholder="User name" value="{{ default(userData.userName, "") }}" />
                            <label for="inputUserName">User name</label>
                          </div>
                        </div>
                        <div class="col-md-6">
                          <div class="form-floating mb-3 mb-md-0">
                            <input class="form-control form-control-sm" name="email" id="inputEmail" type="email" placeholder="Email address" value="{{ default(userData.email, "") }}" />
                            <label for="inputEmail">Email address</label>
                          </div>
                        </div>
                      </div>


                      <div class="row mb-3">
                        <div class="col-md-6">
                          <div class="form-floating mb-3 mb-md-0">
                            <input class="form-control form-control-sm" name="password0" id="inputPassword" type="password" placeholder="Create a password" />
                            <label for="inputPassword">Password</label>
                          </div>
                        </div>
                        <div class="col-md-6">
                          <div class="form-floating mb-3 mb-md-0">
                            <input class="form-control form-control-sm" name="password1" id="inputPasswordConfirm" type="password" placeholder="Confirm password" />
                            <label for="inputPasswordConfirm">Confirm Password</label>
                          </div>
                        </div>
                      </div>


                      <div class="row mb-3">
                        <div class="col-md-6">
                          <div class="form-floating mb-3 mb-md-0">
                            <input class="form-control form-control-sm" name="address" id="inputAddress" type="text" placeholder="address" value="{{ default(userData.address, "") }}"/>
                            <label for="inputAddress">Address</label>
                          </div>
                        </div>
                        <div class="col-md-6">
                          <div class="form-floating">
                            <input class="form-control form-control-sm" name="birthDate" id="inputBirthdate" type="date" placeholder="birth date" value="{{ default(userData.birthDate, "") }}" />
                            <label for="inputBirthdate">Birth date</label>
                          </div>
                        </div>
                      </div>

                      <div class="row mb-3">
                        <div class="col-md-6">
                          <div class="form-floating mb-3 mb-md-0">
                            <input class="form-control form-control-sm" name="nik" id="inputNik" type="text" placeholder="NIK" value="{{ default(userData.nik, "") }}" />
                            <label for="inputNik">NIK</label>
                          </div>
                        </div>
                        <div class="col-md-6">
                          <div class="form-floating">
                            <!--input class="form-control form-control-sm" name="gender" id="inputGender" type="text" placeholder="gender" value="{{ default(userData.gender, "") }}" /-->
                            <select class="form-select form-select-sm" name="gender" id="inputGender">
                              <option value="M" {% if userData.gender == "M" %} selected="selected" {% endif %}>Male</option>
                              <option value="F" {% if userData.gender == "F" %} selected="selected" {% endif %}>Female</option>
                            </select>
                            <label for="inputGender">Gender</label>
                          </div>
                        </div>
                      </div>

                      <div class="row mb-3">
                        <div class="col-md-6">
                          <div class="form-floating mb-3 mb-md-0">
                            <input class="form-control form-control-sm" name="phone" id="inputPhone" type="text" placeholder="phone number" value="{{ default(userData.phone, "") }}" />
                            <label for="inputPhone">Phone</label>
                          </div>
                        </div>
                        <div class="col-md-6">
                          <div class="form-floating">
                            <input readonly class="form-control form-control-sm" name="uniqueCode" id="inputUniqueCode" type="text" placeholder="unique code" value="{{ default(userData.uniqueCode, "") }}" />
                            <label for="inputUniqueCode">Unique code</label>
                          </div>
                        </div>
                      </div>

                      <div class="mt-4 mb-0">
                        <div class="d-grid">
                          <button type="submit" class="btn btn-primary">Create Account</button>
                        </div>
                      </div>
                    </form>
                  </div>
                  <div class="card-footer text-center py-3">
                    <div class="small">
                      <a href="{{ pageBaseUrl }}/login">Have an account? Go to login</a>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
{% endblock %}

{% block pageJS %}
{% endblock %}