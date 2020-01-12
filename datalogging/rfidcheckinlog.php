<?php

// List everyone who successfully checked in to the makerspace
// (last 200)
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

include 'commonfunctions.php';

// get the HTML skeleton
$myfile = fopen("rfidcheckinlog.txt", "r") or die("Unable to open file!");
$html = fread($myfile,filesize("rfidcheckinlog.txt"));
fclose($myfile);

// Get the data
$ini_array = parse_ini_file("rfidconfig.ini", true);
$dbUser = $ini_array["SQL_DB"]["readOnlyUser"];
$dbPassword = $ini_array["SQL_DB"]["readOnlyPassword"];
$dbName = $ini_array["SQL_DB"]["dataBaseName"];

$con = mysqli_connect("localhost",$dbUser,$dbPassword,$dbName);
  
$selectSQL = "SELECT * FROM rawdata where logEvent = 'Checked In' ORDER BY recNum DESC LIMIT 200;";

// Check connection
if (mysqli_connect_errno()) {
  echo "Failed to connect to MySQL: " . mysqli_connect_error();
}

$result = mysqli_query($con, $selectSQL);

// Construct the page

if (mysqli_num_rows($result) > 0) {
	// table header

	// Get the data for each device 
	$previousClientID = "---";
    while($row = mysqli_fetch_assoc($result)) {
    	
    	$thisTableRow = makeTR( array (
                 $row["dateEventLocal"],
                 $row["logEvent"],
                 $row["clientID"],
                 $row["firstName"]
                 )     
       		);
    	
    	$tableRows = $tableRows . $thisTableRow;
    }
    
    $html = str_replace("<<TABLEHEADER>>",
    	makeTR(
    		array( 
    			"Date Event Local",
    			"Log Event",
    			"Client ID",
    			"First Name"
    			)
    		),
    	$html);
    $html = str_replace("<<TABLEROWS>>", $tableRows,$html);

	echo $html;
    
} else {
    echo "0 results";
}

mysqli_close($con);

return;



?>