window.addEventListener('DOMContentLoaded', event => {
   // Simple-DataTables
   // https://github.com/fiduswriter/Simple-DataTables/wiki
   /*
   const datatablesSimple = document.getElementById('datatablesSimple');
   if (datatablesSimple) {
       new simpleDatatables.DataTable(datatablesSimple);
   }
   */
    
   fetch("/demo.json").then(
       response => response.json()
   ).then(
       data => {
           if (!data.length) {
               return
           }
           new window.simpleDatatables.DataTable("#datatablesSimple", {
               data: {
                   headings: Object.keys(data[0]),
                   data: data.map(item => Object.values(item))
               }
           })
       }
   )    
    
    
});
