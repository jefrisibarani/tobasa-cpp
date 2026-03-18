{% extends "layouts/layout_status.tpl" %}

{% block pageContent %}
          <div class="container">
            <div class="row justify-content-center">
              <div class="col-lg-6">
                <div class="text-center mt-4">
                  <h1 class="display-1">{{ statusCode }}</h1>
                  <p class="lead">{{ statusMessage }}</p>
                  <p>{{ statusMessageLong }}</p>
                  <a href="{{ pageBaseUrl }}/"> <i class="fas fa-arrow-left me-1"></i> Return to Home </a><br/>
                  <a href="#" onclick="history.back()"> <i class="fas fa-arrow-left me-1"></i> Go Back </a>
                </div>
              </div>
            </div>
          </div>
{% endblock %}