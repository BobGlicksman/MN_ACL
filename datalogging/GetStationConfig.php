<?php

// REQUEST DEVICE DATA
// 
// Call with GET and data of:
// cmd="<<command>>"
//
// where cmd="-1"  returns list of active device types to use
//       cmd="nn"  returns configuration values for device type nn
//                 or error if nn does not exist
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

$ini_array = parse_ini_file("rfidconfig.ini", true);
$dbUser = $ini_array["SQL_DB"]["writeUser"];
$dbPassword = $ini_array["SQL_DB"]["writePassword"];
$dbName = $ini_array["SQL_DB"]["dataBaseName"];
$con = mysqli_connect("localhost",$dbUser,$dbPassword,$dbName);
// Check connection
if (mysqli_connect_errno()) {
    echo "Failed to connect to MySQL: " . mysqli_connect_error();
  }

// command
$cmd = $_GET["cmd"];

switch($cmd) {
    case "":
        echo "cmd is blank";
        break;
    case -1:
        returnDeviceList($con);
        break;
    default:
        returnDeviceConfig($con, $cmd);
        break;
}

mysqli_close($con);

return;

function returnDeviceList($con) {
    $output = "";
    $getListSQL = 
    "SELECT deviceType, deviceName FROM stationConfig WHERE active = 1 order by deviceName;";
    $result = mysqli_query($con, $getListSQL);
    if (mysqli_num_rows($result) > 0) { 
        $arr = array();
        while($row = mysqli_fetch_assoc($result)) {
            $arr[] = array(
                "deviceType" => $row["deviceType"] + 0,
                "deviceName" => $row["deviceName"]
            );
        }
        $output = json_encode($arr);

    } else {
        $output = "no records found for station list";
    }

    echo $output;

    
}

function returnDeviceConfig($con, $devType) {
    $output = "";
    $getDevConfigSQL = 
    "SELECT deviceType, deviceName, LCDName, photoDisplay, logEvent, OKKeywords from stationConfig WHERE active = 1 and deviceType = <<DEVICETYPE>>;";
    $getDevConfigSQL = str_replace("<<DEVICETYPE>>", $devType, $getDevConfigSQL);

    $result = mysqli_query($con, $getDevConfigSQL);
    $numRows = 0;
    if ($result) {
        $numRows = mysqli_num_rows($result);
    } 
    switch($numRows) {
        case 0:
            $output = "device " . $devType . " not found";
            break;
        case 1: 
            $arr = array();
            while($row = mysqli_fetch_assoc($result)) {
                $thisDevice = array(
                    "deviceType" => $row["deviceType"] + 0,
                    "deviceName" => $row["deviceName"],
                    "LCDName" => $row["LCDName"],
                    "photoDisplay" => $row["photoDisplay"],
                    "logEvent" => $row["logEvent"],
                    "OKKeywords" => $row["OKKeywords"]
                );
                $arr[] = $thisDevice;
            }
            $output = json_encode($arr);
            break;
        default:
            $output = "more than one config record found for device: " . $devtype;
            break;    
    }
    
    echo $output;

}

