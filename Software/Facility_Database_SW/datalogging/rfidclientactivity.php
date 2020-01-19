<?php

// Quick Report from RFID LOGGING DATABASE
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

include 'commonfunctions.php';

// get the HTML skeleton
$myfile = fopen("rfidclientactivity.txt", "r") or die("Unable to open file!");
$html = fread($myfile,filesize("rfidclientactivity.txt"));
fclose($myfile);

// Get the data
$ini_array = parse_ini_file("rfidconfig.ini", true);
$dbUser = $ini_array["SQL_DB"]["readOnlyUser"];
$dbPassword = $ini_array["SQL_DB"]["readOnlyPassword"];
$dbName = $ini_array["SQL_DB"]["dataBaseName"];

$con = mysqli_connect("localhost",$dbUser,$dbPassword,$dbName);
  
$selectSQL = "SELECT * FROM (SELECT * FROM rawdata ORDER BY recNum DESC LIMIT 1000) X ORDER BY clientID, recNum;";


//"SELECT * FROM `rawdata` ORDER BY coreID, `recNum` LIMIT 1000;";



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
                 $row["recNum"], 
                 $row["dateEventLocal"],
                 $row["eventName"],
                 rightWEllipsis($row["coreID"], 8),
                 $row["deviceFunction"],
                 $row["logEvent"],
                 rightWEllipsis($row["clientID"],5),
                 $row["firstName"],
                 $row["logData"]
                 )     
       		);
    	
    	if (strcmp($previousClientID, $row["clientID"]) != 0) {
    		$previousClientID = $row["clientID"];
    		//we have a new device so add a separation
    		$separationRow = makeTR(array("- "," "," "," "," "," "," "," "," "));
    		$tableRows = $tableRows . $separationRow;
    	}
    	
    	$tableRows = $tableRows . $thisTableRow;
    }
    
    $html = str_replace("<<TABLEHEADER>>",
    	makeTR(
    		array( 
    			"Rec Num",
    			"Date Event Local",
    			"EventName",
    			"Core ID",
    			"Device Function",
    			"Log Event",
    			"Client ID",
    			"First Name",
    			"Log Data"
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