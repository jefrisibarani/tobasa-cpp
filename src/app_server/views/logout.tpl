{% extends "layouts/layout_status.tpl" %}

{% block pageContent %}
          <div class="container">
            <div class="row justify-content-center">
              <div class="col-lg-5">
                <div id="alertsInToast" class="alertContainer mt-3"></div>
                <div id="alertsInPage" class="alertContainer  mt-3 d-none"></div>
                <div class="text-center mt-4">
                  <p class="lead">You have been logged out</p>
                  <a href="{{ pageBaseUrl }}/">
                    <i class="fas fa-arrow-left me-1"></i> Return to Home </a>
                </div>
              </div>
            </div>
          </div>
{% endblock %}

{% block pageJS %}
{% endblock %}