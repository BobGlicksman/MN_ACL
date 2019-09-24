<?php

// Quick Report from RFID LOGGING DATABASE
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

// get the HTML skeleton
$myfile = fopen("rfidtop100html.txt", "r") or die("Unable to open file!");
$html = fread($myfile,filesize("rfidtop100html.txt"));
fclose($myfile);

// Get the data
$ini_array = parse_ini_file("rfidconfig.ini", true);
$dbUser = $ini_array["SQL_DB"]["readOnlyUser"];
$dbPassword = $ini_array["SQL_DB"]["readOnlyPassword"];
$dbName = $ini_array["SQL_DB"]["dataBaseName"];

$con = mysqli_connect("localhost",$dbUser,$dbPassword,$dbName);
  
$selectSQL = 
"SELECT * FROM `rawdata` ORDER BY `recNum` DESC LIMIT 100;";

// Check connection
if (mysqli_connect_errno()) {
  echo "Failed to connect to MySQL: " . mysqli_connect_error();
}

$result = mysqli_query($con, $selectSQL);

// Construct the page

if (mysqli_num_rows($result) > 0) {
	// table header

	// output data of each row
    while($row = mysqli_fetch_assoc($result)) {
    	
        $thisTableRow = makeTR( 
          makeTD($row["recNum"]) 
        . makeTD($row["dateEventLocal"])
        . makeTD($row["eventName"])
        . makeTD($row["coreID"])
        . makeTD($row["deviceFunction"])
        . makeTD($row["logEvent"])
        . makeTD($row["clientID"])
        . makeTD($row["firstName"])
        . makeTD($row["logData"])
    	);
    	
    	$tableRows = $tableRows . $thisTableRow;
    }
    
    $html = str_replace("<<TABLEHEADER>>","<tr><td>recNum</td><td>dateEventLocal</td><td>eventName</td><td>coreID</td>
	<td>deviceFunction</td><td>logEvent</td><td>clientID</td><td>firstName</td><td>logData</td></tr>",$html);
    $html = str_replace("<<TABLEROWS>>", $tableRows,$html);

	echo $html;
    
} else {
    echo "0 results";
}

mysqli_close($con);

return;

function makeTR($data) {
	return "<tr>" . $data . "</tr>";
}
function makeTD($data) {
	return "<td>" . $data . "</td>";
}
