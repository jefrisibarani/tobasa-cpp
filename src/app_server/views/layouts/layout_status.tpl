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
    <div id="layoutError">
      <div id="layoutError_content">
        <main>
          {% block pageContent %}{% endblock %}
        </main>
      </div>
      <div id="layoutError_footer">
        {% include "common/page_footer.tpl" %}
      </div>
    </div>
    {% block pageJS %}{% endblock %}
  </body>
</html>