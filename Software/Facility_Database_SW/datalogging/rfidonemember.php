<?php

// List everyone who successfully checked in to the makerspace
// (last 200)
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

include 'commonfunctions.php';

// get the ClientID
$clientID = $_GET["clientID"];
if ($clientID == 0) {
    echo "clientID= parameter not found.";
    return;
}

// get the HTML skeleton
$myfile = fopen("rfidonemember.txt", "r") or die("Unable to open file!");
$html = fread($myfile,filesize("rfidcheckinlog.txt"));
fclose($myfile);

// Get the data
$ini_array = parse_ini_file("rfidconfig.ini", true);
$dbUser = $ini_array["SQL_DB"]["readOnlyUser"];
$dbPassword = $ini_array["SQL_DB"]["readOnlyPassword"];
$dbName = $ini_array["SQL_DB"]["dataBaseName"];

$con = mysqli_connect("localhost",$dbUser,$dbPassword,$dbName);

$selectSQL = "SELECT dateEventLocal, firstName, logEvent FROM `rawdata` WHERE clientID = '<<CLIENTID>>' AND logEvent IN ('checkin denied','checked in')  ORDER BY dateEventLocal DESC LIMIT 500;";
$selectSQL = str_replace("<<CLIENTID>>",$clientID,$selectSQL);

// Check connection
if (mysqli_connect_errno()) {
  echo "Failed to connect to MySQL: " . mysqli_connect_error();
  return;
}

$result = mysqli_query($con, $selectSQL);

// Construct the page

if (mysqli_num_rows($result) > 0) {

	$previousClientID = "---";
    while($row = mysqli_fetch_assoc($result)) {

    	$thisTableRow = makeTR( array (
                 $row["dateEventLocal"],
                 $row["firstName"],
                 $row["logEvent"]
                 )
       		);

    	$tableRows = $tableRows . $thisTableRow;
    }

    $html = str_replace("<<TABLEHEADER>>",
    	makeTR(
    		array(
    			"Date Event Local",
    			"First Name",
                "Log Event"
    			)
    		),
    	$html);
    $html = str_replace("<<TABLEROWS>>", $tableRows,$html);
    $html = str_replace("<<CLIENTID>>", $clientID, $html);

	echo $html;

} else {
    echo "0 results";
}

mysqli_close($con);

return;



?>
