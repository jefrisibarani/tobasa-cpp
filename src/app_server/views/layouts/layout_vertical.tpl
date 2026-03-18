<!DOCTYPE html>
<html lang="{{ pageLang }}">
  <head>
    {% include "common/page_meta.tpl" %}
    <title>{{ pageTitle }}</title>
    {% include "common/page_css_core.tpl" %}
    {% block pageVendorCSS %}{% endblock %}
    {% block pageCSS %}{% endblock %}
    {% include "common/page_js_head.tpl" %}
    {% include "common/page_js_tbs.tpl" %}
  </head>
  <body id="page-top" class="sb-nav-fixed">
    {% include "common/page_header.tpl" %}
    <div id="layoutSidenav">
      <div id="layoutSidenav_nav">
        {% include "common/page_sidebar.tpl" %}
      </div>    
      <div id="layoutSidenav_content">
        <main id="{{ pageId }}">
          <div class="container-fluid px-4">
            <h2 class="mt-2">{{ pageContentTitle }}</h2>
            <!-- Breadcrumbs-->
            {% include "common/page_breadcrumb.tpl" %}
            <!-- Alerts/Flashdata in page -->
            <div id="alertsInPage" class="alert_container d-none"></div>
            <!-- Page Content -->
            {% block pageContent %}{% endblock %}
          </div>
        </main>
        {% include "common/page_footer.tpl" %}
      </div>
    </div>
    
    {% include "common/page_alert_img.tpl" %}
    <!-- Scroll to Top Button-->
    <a id="scroll_to_top" class="rounded" href="#page-top"><i class="fas fa-angle-up"></i></a>
    <!-- Alerts/Flashdata in toast -->
    <div id="alertsInToast" class="alert_container"></div>
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