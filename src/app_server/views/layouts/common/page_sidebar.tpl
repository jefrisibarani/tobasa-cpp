        <nav class="sb-sidenav accordion sb-sidenav-dark" id="sidenavAccordion">
          <div class="sb-sidenav-menu">
            <div class="nav">

              <!-- /Menu: Core -->
              <div class="sb-sidenav-menu-heading">Core</div>
              <a class="nav-link" href="{{ pageBaseUrl }}/dashboard">
                <div class="sb-nav-link-icon"><i class="fas fa-tachometer-alt"></i></div> Dashboard
              </a>

              <!-- /Menu: Interface -->
              <!-- div class="sb-sidenav-menu-heading">Interface</div> -->


              {% if exists("sidebarMenu") %}
              {% for groupMenu in sidebarMenu %}
              <!-- Menu group: {{ groupMenu.groupName }} Menu -->
              <a class="nav-link collapsed" href="#" data-bs-toggle="collapse" data-bs-target="#node_{{ groupMenu.groupName }}" aria-expanded="false" aria-controls="node_{{ groupMenu.groupName }}">
                <div class="sb-nav-link-icon"><i class="{{ groupMenu.icon }}"></i></div>{{ groupMenu.caption }}
                <div class="sb-sidenav-collapse-arrow"><i class="fas fa-angle-down"></i></div>
              </a>
              <div class="collapse" id="node_{{ groupMenu.groupName }}" aria-labelledby="headingOne" data-bs-parent="#sidenavAccordion">
                <nav class="sb-sidenav-menu-nested nav" id="node_nav_{{ groupMenu.groupName }}">
                {% for menu in groupMenu.menuList %}
                  {% if length(menu.menuList) > 0 %}

                    <!-- Menu sub group: {{ groupMenu.groupName }} -->
                    <a class="nav-link collapsed" href="#" data-bs-toggle="collapse" data-bs-target="#subnode_{{ menu.name }}" aria-expanded="false" aria-controls="subnode_{{ menu.name }}">
                      <div class="sb-nav-link-icon"><i class="{{menu.icon}}"></i></div>  {{ menu.caption }}
                      <div class="sb-sidenav-collapse-arrow"><i class="fas fa-angle-down"></i></div>
                    </a>
                    <div class="collapse" id="subnode_{{ menu.name }}" aria-labelledby="headingOne" data-bs-parent="#node_nav_{{ groupMenu.groupName }}">
                      <nav class="sb-sidenav-menu-nested nav">
                        {% for xmenu in menu.menuList %}
                        <a class="nav-link" href="{{ pageBaseUrl }}/{{ xmenu.requestPath }}">
                          <div class="sb-nav-link-icon"><i class="{{xmenu.icon}}"></i></div> {{ xmenu.caption }}
                        </a>
                        {% endfor %}
                      </nav>
                    </div>
                    <!-- /Menu sub group: {{ groupMenu.groupName }} -->

                  {% else %}
                  <a class="nav-link" href="{{ pageBaseUrl }}/{{ menu.requestPath }}">
                    <div class="sb-nav-link-icon"><i class="{{menu.icon}}"></i></div> {{ menu.caption }}
                  </a>
                  {% endif %}
                
                {% endfor %}
                </nav>
              </div>
              <!-- /Menu group: {{ groupMenu.groupName }} Menu -->
              {% endfor %}
              {% endif %}

              {% if appBuildMode == "DEVELOPMENT" %}
              <!-- Menu group: Pages -->
              <a class="nav-link collapsed" href="#" data-bs-toggle="collapse" data-bs-target="#node_pages" aria-expanded="false" aria-controls="node_pages">
                <div class="sb-nav-link-icon"><i class="fas fa-book-open"></i></div> Pages 
                <div class="sb-sidenav-collapse-arrow"><i class="fas fa-angle-down"></i></div>
              </a>
              <div class="collapse" id="node_pages" aria-labelledby="headingTwo" data-bs-parent="#sidenavAccordion">
                <nav class="sb-sidenav-menu-nested nav accordion" id="node_nav_pages">
                  
                  <a class="nav-link" href="{{ pageBaseUrl }}/server_status">Server status</a>

                  <!-- Menu sub group: Authentication -->
                  <a class="nav-link collapsed" href="#" data-bs-toggle="collapse" data-bs-target="#node_auth" aria-expanded="false" aria-controls="node_auth"> Authentication
                    <div class="sb-sidenav-collapse-arrow"><i class="fas fa-angle-down"></i></div>
                  </a>
                  <div class="collapse" id="node_auth" aria-labelledby="headingOne" data-bs-parent="#node_nav_pages">
                    <nav class="sb-sidenav-menu-nested nav">
                      <a class="nav-link" href="{{ pageBaseUrl }}/login">Login</a>
                      <a class="nav-link" href="{{ pageBaseUrl }}/register">Register</a>
                      <a class="nav-link" href="{{ pageBaseUrl }}/password">Forgot Password</a>
                    </nav>
                  </div>
                  <!-- /Menu sub group: Authentication -->
                  
                  <!-- Menu group: Error -->
                  <a class="nav-link collapsed" href="#" data-bs-toggle="collapse" data-bs-target="#node_error" aria-expanded="false" aria-controls="node_error"> Error
                    <div class="sb-sidenav-collapse-arrow"><i class="fas fa-angle-down"></i></div>
                  </a>
                  <div class="collapse" id="node_error" aria-labelledby="headingOne" data-bs-parent="#node_nav_pages">
                    <nav class="sb-sidenav-menu-nested nav">
                      <a class="nav-link" href="{{ pageBaseUrl }}/spage/401">401 Page</a>
                      <a class="nav-link" href="{{ pageBaseUrl }}/spage/404">404 Page</a>
                      <a class="nav-link" href="{{ pageBaseUrl }}/spage/500">500 Page</a>
                    </nav>
                  </div>
                  <!-- /Menu sub group: Error -->

                </nav>
              </div>
              <!-- /Menu group: Pages -->
              {% endif %}

            </div>
          </div>
          <div class="sb-sidenav-footer">
            <div class="small">Logged in as:</div> {{ identity.userName }}
          </div>
        </nav>