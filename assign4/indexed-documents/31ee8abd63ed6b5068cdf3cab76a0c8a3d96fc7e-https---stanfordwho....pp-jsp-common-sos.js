// JavaScript Document

function qs(el,type) {
    var qe  = document.getElementById('search_string').value;
    if (qe.length > 0){
       if (type == 'who' || type == 'org'){
      	el.href+= "/Search.do?search=";
	el.href+= qe;
	 }
     else if ( type=='web'){
     	el.href+="?cx=003265255082301896483%3Asq5n7qoyfh8&cof=FORID%3A9&q=";
	el.href+= qe;
     }
    else if (type =='map'){
	el.href+="?srch=";
	el.href+=
	qe;
	}
   }
  return 1;
}

function startstate(){

	var li = document.getElementById('ToggleLink');
	
	if (li) {
		li.style.display="block";
	}
	document.getElementById('search_string').focus();	
}


function submit_search(){
	if (document.getElementById('searchform').search.value) {
		url='/OSApp/authSearch.do?';
		url = url + 'search=' + document.getElementById('searchform').search.value;
		url = url + '&stanfordonly=checkbox';    
		
		location = url; 
	} else {
		location = '/OSApp/jsp/stanford/stanford_index.jsp';
	}
}




