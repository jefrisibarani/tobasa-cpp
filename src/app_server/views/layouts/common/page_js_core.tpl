    <script src="{{ pageBaseUrl }}/vendor/bootstrap_5.2.3/js/bootstrap.bundle.min.js" crossorigin="anonymous"></script>
    <script type="text/javascript"> <!-- JS CORE -->
      let enableTooltipsEverywhere = function(){
         // Enable tooltips everywhere
         var tooltipTriggerList = [].slice.call(document.querySelectorAll('[data-bs-toggle="tooltip"]'))
         var tooltipList = tooltipTriggerList.map(function (tooltipTriggerEl) {
            return new bootstrap.Tooltip(tooltipTriggerEl)
         });
      };

      window.addEventListener('DOMContentLoaded', event => {
         // Toggle the side navigation
         const sidebarToggle = document.body.querySelector('#sidebarToggle');
         if (sidebarToggle) {
            // Uncomment Below to persist sidebar toggle between refreshes
            // if (localStorage.getItem('sb|sidebar-toggle') === 'true') {
            //     document.body.classList.toggle('sb-sidenav-toggled');
            // }
            sidebarToggle.addEventListener('click', event => {
               event.preventDefault();
               document.body.classList.toggle('sb-sidenav-toggled');
               localStorage.setItem('sb|sidebar-toggle', document.body.classList.contains('sb-sidenav-toggled'));
            });
         }
         
         enableTooltipsEverywhere();
      });
    </script> <!-- JS CORE -->