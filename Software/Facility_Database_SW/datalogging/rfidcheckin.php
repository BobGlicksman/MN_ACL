<?php

// Check In calls from the rfidreaders should go go rfidcheckinout.php
// All other logging calls should go to rfidlogging.php
// then this file should be deleted

// SUBMIT CHECKIN/OUT DATA TO RFID LOGGING DATABASE
// 
// Call with POST and data of:
// {
//  "event": "RFIDLogging",
//  "data": "{"dateEventLocal":"2019-09-22 16:29:13","deviceFunction":"Check In","clientID":21524942,"logEvent":"checkin","logData":"Sunnyvale"}",
//  "published_at": "2019-09-22T23:29:18.543Z",
//  "coreid": "e00fce683ce9c5a9e5a4f43d"
// }
// 
// If ClientID is currently Checked Out or missing, then add a checkin record
// If ClientID is currently Checked In, then add a checkout record
//
// Assume if a ClientID has not been seen today, then they are checked out
//
// RETURN "CheckedIn" or "CheckedOut"
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

$ini_array = parse_ini_file("rfidconfig.ini", true);
$dbUser = $ini_array["SQL_DB"]["writeUser"];
$dbPassword = $ini_array["SQL_DB"]["writePassword"];
$dbName = $ini_array["SQL_DB"]["dataBaseName"];

// standard data from webhook
$datePublishedAt =  cleanInput($_POST["published_at"]);
$coreID =  cleanInput($_POST["coreid"] );
$eventName = cleanInput($_POST["event"]);

// data from the device in JSON format
$postedData =  $_POST["data"];
//echo "posted data:" . $postedData;
$myJSON = json_decode($postedData,true);
echo "<p>" . printJSONError(json_last_error());
//var_dump($myJSON); // xxx

// data from the device's JSON
$dateEventLocal = cleanInput($myJSON["dateEventLocal"] );
$deviceFunction =  cleanInput($myJSON["deviceFunction"] );
$firstName =  cleanInput($myJSON["firstName"] );
$clientID =  cleanInput($myJSON["clientID"] );
$logEvent =  cleanInput($myJSON["logEvent"]);
$logData =  cleanInput($myJSON["logData"] );

// set up SQL connection
$con = mysqli_connect("localhost",$dbUser,$dbPassword,$dbName);

// SQL to determine if this person is checked in or out
// by getting most recent status for this clientID 
$today = new DateTime(); 
$today->setTimeZone(new DateTimeZone("America/Los_Angeles"));

$currentStatusSQL = 
"SELECT * FROM rawdata 
WHERE clientID = <<clientID>> 
  AND logEvent in ('Checked In','Checked Out')
  AND CONVERT( dateEventLocal, DATE) = CONVERT('" . date_format($today, "Y-m-d") . "', DATE) 
ORDER BY dateEventLocal DESC 
LIMIT 1";

$currentStatusSQL = str_replace("<<clientID>>",$clientID,$currentStatusSQL);

$resultCheckin = mysqli_query($con, $currentStatusSQL);

// determine if the client is currently checked in or checked out
$isClientCheckedIn = false; 
$lastCheckedInTime = "";
if (mysqli_num_rows($resultCheckin)) {

    $returnedRow = mysqli_fetch_assoc($resultCheckin);
    
    if (strcmp($returnedRow["logEvent"],"Checked In") == 0) {
        // they were found checked in
        $lastCheckedInTime = $returnedRow["dateEventLocal"];
        $isClientCheckedIn = true;
    }
}

// Log insert record sql
$insertSQL = 
"INSERT INTO `rawdata`
       (`dateEventLocal`, `coreID`, `deviceFunction`, `clientID`, firstName, `eventName`, `logEvent`, `logData`, `datePublishedAt`,ipAddress) 
VALUES ('<<dateEventLocal>>', '<<coreID>>', '<<deviceFunction>>', '<<clientID>>', '<<firstName>>', '<<eventName>>', '<<logEvent>>', '<<logData>>','<<datePublishedAt>>','<<ipAddress>>')";

// put common values into sql
$insertSQL = str_replace("<<dateEventLocal>>", $dateEventLocal, $insertSQL);
$insertSQL = str_replace("<<coreID>>", $coreID, $insertSQL);
$insertSQL = str_replace("<<clientID>>", $clientID, $insertSQL);
$insertSQL = str_replace("<<firstName>>", $firstName, $insertSQL);
$insertSQL = str_replace("<<datePublishedAt>>", $datePublishedAt, $insertSQL);
$insertSQL = str_replace("<<eventName>>", $eventName, $insertSQL);
$insertSQL = str_replace("<<ipAddress>>", $_SERVER['REMOTE_ADDR'], $insertSQL);

// keep it as this point for the checkin/out function
$checkInOutSQL = $insertSQL;

// continue with device log specific data
$insertSQL = str_replace("<<deviceFunction>>", $deviceFunction, $insertSQL);
$insertSQL = str_replace("<<logEvent>>", $logEvent, $insertSQL);
$insertSQL = str_replace("<<logData>>", $logData, $insertSQL);

$returnMessage = "default return message";

// log device event
if (mysqli_query($con, $insertSQL)) {
    $returnMessage = "New log record created successfully";
} else {
    echo "<p>Error: " . $insertSQL . "<br>" . mysqli_error($con);
}

// if the device has said we can check this person in, then toggle their status
// prepend a space to make this damn strpos work like any reasonable language would!
if (strpos(" " . $logEvent,"checkin allowed") == 1) {

    // check the person in or out
    $checkInOutSQL = str_replace("<<deviceFunction>>", "rfidcheckin.php", $checkInOutSQL);
    

    if ($isClientCheckedIn) {
        // They are checked in, so check them out
        $checkInOutSQL = str_replace("<<logEvent>>","Checked Out" , $checkInOutSQL);

        $minutesInSpace = round((strtotime($dateEventLocal) - strtotime($lastCheckedInTime) )/60);

        $checkInOutSQL = str_replace("<<logData>>", $minutesInSpace , $checkInOutSQL);
        $returnMessage = "Checked Out";
    } else {
        // They are checked out, so check them in
        $checkInOutSQL = str_replace("<<logEvent>>","Checked In" , $checkInOutSQL);
        $checkInOutSQL = str_replace("<<logData>>", "", $checkInOutSQL);
        $returnMessage = "Checked In";
    }

    // check the member in or out
    if (mysqli_query($con, $checkInOutSQL)) {
        //echo "<p>New record created successfully";
    } else {
        echo "<p>Error: " . $checkInOutSQL . "<br>" . mysqli_error($con);
    }

    // The Particle device will parse the return and show this to the user.
    $returnMessage = "<ActionTaken>" . $returnMessage . "</ActionTaken>"; 

} 

echo $returnMessage;  

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