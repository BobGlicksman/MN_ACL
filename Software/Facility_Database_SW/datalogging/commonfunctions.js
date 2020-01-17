// javascript functions to run in the client web pages
// 
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

function checkout(clientID,name) {
    var xhttp = new XMLHttpRequest();
    var theURL = "http://rfid.makernexuswiki.com/rfidmanualcheckout.php?clientID=" + clientID 
        + "&firstName=" + encodeURI(name);
    xhttp.open("GET", theURL, true);
    xhttp.send();
    location.reload(true);
}
