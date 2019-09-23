<?php

// SUBMIT DATA TO RFID LOGGING DATABASE
// 
// Call with POST and data of:
// {
//  "event": "RFIDLogging",
//  "data": "{"dateEventLocal":"2019-09-22 16:29:13","deviceFunction":"Check In","clientID":21524942,"logEvent":"checkin","logData":"Sunnyvale"}",
//  "published_at": "2019-09-22T23:29:18.543Z",
//  "coreid": "e00fce683ce9c5a9e5a4f43d"
// }
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

echo "Log Submit";

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
$clientID =  cleanInput($myJSON["clientID"] );
$logEvent =  cleanInput($myJSON["logEvent"]);
$logData =  cleanInput($myJSON["logData"] );

// put values into sql
$con = mysqli_connect("localhost","USER","PWD","rfidlogs");
  
$insertSQL = 
"INSERT INTO `rawdata`
       (`dateEventLocal`, `coreID`, `deviceFunction`, `clientID`, `eventName`, `logEvent`, `logData`, `datePublishedAt`) 
VALUES ('<<dateEventLocal>>', '<<coreID>>', '<<deviceFunction>>', '<<clientID>>', '<<eventName>>', '<<logEvent>>', '<<logData>>','<<datePublishedAt>>')";

$insertSQL = str_replace("<<dateEventLocal>>", $dateEventLocal, $insertSQL);
$insertSQL = str_replace("<<coreID>>", $coreID, $insertSQL);
$insertSQL = str_replace("<<deviceFunction>>", $deviceFunction, $insertSQL);
$insertSQL = str_replace("<<clientID>>", $clientID, $insertSQL);
$insertSQL = str_replace("<<eventName>>", $eventName, $insertSQL);
$insertSQL = str_replace("<<logEvent>>", $logEvent, $insertSQL);
$insertSQL = str_replace("<<logData>>", $logData, $insertSQL);
$insertSQL = str_replace("<<datePublishedAt>>", $datePublishedAt, $insertSQL);

//echo "<p>" . $insertSQL;

// Check connection
if (mysqli_connect_errno())
  {
  echo "Failed to connect to MySQL: " . mysqli_connect_error();
  }

if (mysqli_query($con, $insertSQL)) {
    echo "<p>New record created successfully";
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