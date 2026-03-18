<!DOCTYPE html>
<html lang="en">
  <head>
    {% include "common/page_meta.tpl" %}
    <title>{{ pageTitle }}</title>
    {% include "common/page_css_core.tpl" %}
    {% block pageCss %}{% endblock %}
    {% include "common/page_js_tbs.tpl" %}
  </head>
  <body class="{{ pageBodyClass }}">
    <div id="layoutBasic">
      <div id="layoutBasic_content">
        <main>
          {% block pageContent %}{% endblock %}
        </main>
      </div>
      <div id="layoutBasic_footer">
        {% include "common/page_footer.tpl" %}
      </div>
    </div>
    
    {% include "common/page_alert_img.tpl" %}
    <div id="session_counter" class="bg-info text-danger"></div>
    <!-- Ajax running indicator -->
    <div id="modal_loader"></div>
    <!-- Javascripts -->
    {% include "common/page_js_core.tpl" %}
    {% block pageVendorJS %}{% endblock %}
    {% include "common/page_js_alert.tpl" %}
    {% include "common/page_js_site.tpl" %}
    {% include "common/page_js_sess_expired.tpl" %}

    {% block pageJS %}{% endblock %}
  </body>
</html>