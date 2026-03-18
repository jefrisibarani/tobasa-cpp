{% extends "layouts/layout_basic.tpl" %}

{% block pageContent %}
          <div class="container">
            <div class="row justify-content-center">
              <div class="col-lg-5">
                
                <div id="alertsInToast" class="alertContainer mt-3"></div>
                <div id="alertsInPage" class="alertContainer  mt-3 d-none"></div>

                <div class="card shadow-lg border-0 rounded-lg mt-5">
                  <div class="card-header">
                    <h3 class="text-center font-weight-light my-4">Password Recovery</h3>
                  </div>
                  <div class="card-body">
                    <div class="small mb-3 text-muted">Enter your user name and we will send you a link to reset your password.</div>
                    <form action="{{ pageBaseUrl }}/password" method="post">
                      <div class="form-floating mb-3">
                        <input class="form-control" name="username" id="inputUsername" type="text" placeholder="user name" />
                        <label for="inputUsername">User name</label>
                      </div>
                      <div class="d-flex align-items-center justify-content-between mt-4 mb-0">
                        <a class="small" href="{{ pageBaseUrl }}/login">Return to login</a>
                        <button type="submit" class="btn btn-primary">Reset Password</button>
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