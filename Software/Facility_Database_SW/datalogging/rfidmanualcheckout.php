<?php

// SUBMIT CHECKOUT TO RFID LOGGING DATABASE
// 
// Call with GET params of clientID=, sourceURL=, firstName=
//
// This will add a Checked Out record to the database for the clientID
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

include 'commonfunctions.php';

allowWebAccess();  // check ip and die if not allowed

$ini_array = parse_ini_file("rfidconfig.ini", true);
$dbUser = $ini_array["SQL_DB"]["writeUser"];
$dbPassword = $ini_array["SQL_DB"]["writePassword"];
$dbName = $ini_array["SQL_DB"]["dataBaseName"];

// data from the device in JSON format
$clientID = $_REQUEST["clientID"];
$coreID =  $_REQUEST["sourceURL"];
$firstName =  $_REQUEST["firstName"];
$eventName = "checkOutClient";

// data from the device's JSON
$today = new DateTime(); 
$today->setTimeZone(new DateTimeZone("America/Los_Angeles"));
$currentDateTime = date_format($today, "Y-m-d H:i:s");
$dateEventLocal = $currentDateTime;
$datePublishedAt = $currentDateTime;

$deviceFunction =  "manual checkout";
cleanInput($myJSON["firstName"] );
$logEvent =  "Checked Out";
$logData =  "9999";

// set up SQL connection
$con = mysqli_connect("localhost",$dbUser,$dbPassword,$dbName);

// Log insert record sql
$insertSQL = 
"INSERT INTO `rawdata`
       (`dateEventLocal`, `coreID`, `deviceFunction`, `clientID`, firstName, `eventName`, `logEvent`, `logData`, `datePublishedAt`, ipAddress) 
VALUES ('<<dateEventLocal>>', '<<coreID>>', '<<deviceFunction>>', '<<clientID>>', '<<firstName>>', '<<eventName>>', '<<logEvent>>', '<<logData>>','<<datePublishedAt>>','<<ipAddress>>')";

// put common values into sql
$insertSQL = str_replace("<<dateEventLocal>>", $dateEventLocal, $insertSQL);
$insertSQL = str_replace("<<coreID>>", $coreID, $insertSQL);
$insertSQL = str_replace("<<clientID>>", $clientID, $insertSQL);
$insertSQL = str_replace("<<firstName>>", $firstName, $insertSQL);
$insertSQL = str_replace("<<datePublishedAt>>", $datePublishedAt, $insertSQL);
$insertSQL = str_replace("<<eventName>>", $eventName, $insertSQL);
$insertSQL = str_replace("<<deviceFunction>>", $deviceFunction, $insertSQL);
$insertSQL = str_replace("<<logEvent>>", $logEvent, $insertSQL);
$insertSQL = str_replace("<<logData>>", $logData, $insertSQL);
$insertSQL = str_replace("<<ipAddress>>", $_SERVER['REMOTE_ADDR'], $insertSQL);

// check the member in or out
if (mysqli_query($con, $insertSQL)) {
    echo $firstName . " has been Checked Out"; 
} else {
    echo "<p>Error: " . $insertSQL . "<br>" . mysqli_error($con);
}

mysqli_close($con);

return;

// Pass in a string headed for the db and clean it. This isn't
// perfect, just meant to hold off some badness
function cleanInput ($data) {
	
	$baditems = array("select ","update ","delete ","`","insert ","alter ", "drop ");
	$data = str_ireplace($baditems, "[] ",$data);
	return $data;
}


// Print any JSON error
function printJSONError ($errorCode) {

	switch ($errorCode) {
        case JSON_ERROR_NONE:
            //echo ' - No JSON errors';
        break;
        case JSON_ERROR_DEPTH:
            echo ' JSON - Maximum stack depth exceeded';
        break;
        case JSON_ERROR_STATE_MISMATCH:
            echo 'JSON - Underflow or the modes mismatch';
        break;
        case JSON_ERROR_CTRL_CHAR:
            echo ' JSON - Unexpected control character found';
        break;
        case JSON_ERROR_SYNTAX:
            echo ' JSON - Syntax error, malformed JSON';
        break;
        case JSON_ERROR_UTF8:
            echo ' JSON - Malformed UTF-8 characters, possibly incorrectly encoded';
        break;
        default:
            echo ' JSON - Unknown error';
        break;
    };
    
}

?>