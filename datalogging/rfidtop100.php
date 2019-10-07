<?php

// Quick Report from RFID LOGGING DATABASE
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

include 'commonfunctions.php';

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
        	array ( 
         		$row["recNum"], 
         		$row["dateEventLocal"],
        		$row["eventName"],
        		$row["coreID"],
        		$row["deviceFunction"],
        		$row["logEvent"],
        		$row["clientID"],
        		$row["firstName"],
        		$row["logData"]
        	)
    	);
    	
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