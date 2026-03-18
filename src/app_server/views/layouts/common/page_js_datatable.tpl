    {% if datatable.useJSZip %}
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/JSZip-2.5.0/jszip.min.js"></script>
    {% endif %}
    {% if datatable.usePdfMake %}
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/pdfmake-0.1.36/pdfmake.min.js"></script>
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/pdfmake-0.1.36/vfs_fonts.js"></script>
    {% endif %}
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/DataTables-1.11.1/js/jquery.dataTables.min.js"></script>
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/DataTables-1.11.1/js/dataTables.bootstrap5.min.js"></script>
    {% if datatable.useAutoFill %}
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/AutoFill-2.3.7/js/dataTables.autoFill.min.js"></script>
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/AutoFill-2.3.7/js/autoFill.bootstrap5.min.js"></script>
    {% endif %}
    {% if datatable.useButtons %}
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/Buttons-2.0.0/js/dataTables.buttons.min.js"></script>
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/Buttons-2.0.0/js/buttons.bootstrap5.min.js"></script>
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/Buttons-2.0.0/js/buttons.colVis.min.js"></script>
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/Buttons-2.0.0/js/buttons.html5.min.js"></script>
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/Buttons-2.0.0/js/buttons.print.min.js"></script>
    {% endif %}
    {% if datatable.useDateTime %}
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/DateTime-1.1.1/js/dataTables.dateTime.min.js"></script>
    {% endif %}
    {% if datatable.useFixedColumns %}
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/FixedColumns-3.3.3/js/dataTables.fixedColumns.min.js"></script>
    {% endif %}
    {% if datatable.useFixedHeader %}
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/FixedHeader-3.1.9/js/dataTables.fixedHeader.min.js"></script>
    {% endif %}
    {% if datatable.useKeyTable %}
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/KeyTable-2.6.4/js/dataTables.keyTable.min.js"></script>
    {% endif %}
    {% if datatable.useResponsive %}
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/Responsive-2.2.9/js/dataTables.responsive.min.js"></script>
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/Responsive-2.2.9/js/responsive.bootstrap5.min.js"></script>
    {% endif %}
    {% if datatable.useRowGroup %}
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/RowGroup-1.1.3/js/dataTables.rowGroup.min.js"></script>
    {% endif %}
    {% if datatable.useRowReorder %}
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/datatables/RowReorder-1.2.8/js/dataTables.rowReorder.min.js"></script>
    {% endif %}
    {% if datatable.useRowDelete or datatable.useRowEdit %}
    <script type="text/javascript" src="{{ pageBaseUrl }}/vendor/bootbox_5.5.2/bootbox.min.js"></script>
    {% endif %}