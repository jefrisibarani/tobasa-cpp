{% extends "layouts/layout_basic.tpl" %}


{% block pageContent %}
          <div class="container">
            <div class="row justify-content-center">
              <div class="col-lg-5">

                <div id="alertsInToast" class="alertContainer mt-3"></div>
                <div id="alertsInPage" class="alertContainer  mt-3 d-none"></div>
                
                <div class="card shadow-lg border-0 rounded-lg mt-5">
                  <div class="card-header">
                    <h3 class="text-center font-weight-light my-4">Login</h3>
                  </div>
                  <div class="card-body">
                    <form action="{{ pageBaseUrl }}/login" method="post">
                      <div class="form-floating mb-3">
                        <input name="loginName" class="form-control" id="inputEmail" type="text" placeholder="user name" />
                        <label for="inputEmail">User name</label>
                      </div>
                      <div class="form-floating mb-3">
                        <input name="password" class="form-control" id="inputPassword" type="password" placeholder="password" />
                        <label for="inputPassword">Password</label>
                      </div>
                      <div class="form-floating mb-3 d-none">
                        <select class="form-select" id="cbLanguage" name="langId">
                          <option selected >Choose language</option>
                          <option value="id-ID">Indonesia</option>
                          <option value="en-US">English</option>
                        </select>
                      </div>
                      <div class="form-floating mb-3  d-none">
                        <input name="siteId" class="form-control" id="inputSiteId" type="text" placeholder="Site Id" value="1"/>
                        <label for="inputSiteId">Site Id</label>
                      </div>
                      <div class="form-check mb-3 d-none">
                        <input class="form-check-input" id="inputRememberPassword" type="checkbox" value="0" />
                        <label class="form-check-label" for="inputRememberPassword">Remember Password</label>
                      </div>
                      <div class="d-flex align-items-center justify-content-between mt-4 mb-0">
                        <a class="small" href="{{ pageBaseUrl }}/password">Forgot Password?</a>
                        <button type="submit" class="btn btn-primary">Login</button>
                      </div>
                    </form>
                  </div>
                  <div class="card-footer text-center py-3">
                    <div class="small">
                      <a href="{{ pageBaseUrl }}/register">Need an account? Sign up!</a>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
{% endblock %}

{% block pageJS %}
{% endblock %}