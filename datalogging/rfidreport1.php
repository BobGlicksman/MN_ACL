<?php

// Quick Report from RFID LOGGING DATABASE
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

echo "<h2>Log Dump</h2>";

// put values into sql

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

if (mysqli_num_rows($result) > 0) {
	// table header
	echo "<table><tr><td>recNum</td><td>dateEventLocal</td><td>eventName</td><td>coreID</td>
	<td>deviceFunction</td><td>logEvent</td><td>clientID</td><td>logData</td></tr>";
	// output data of each row
    while($row = mysqli_fetch_assoc($result)) {
    	echo "<tr>";
    	
        echo 
        "<td>" . $row["recNum"] . "</td>"
        . "<td>" . $row["dateEventLocal"]  . "</td>"
        . "<td>" . $row["eventName"]  . "</td>"
        . "<td>" . $row["coreID"]  . "</td>"
        . "<td>" . $row["deviceFunction"]  . "</td>"
        . "<td>" . $row["logEvent"]  . "</td>"
        . "<td>" . $row["clientID"]  . "</td>"
        . "<td>" . $row["firstName"]  . "</td>"
        . "<td>" . $row["logData"] 	 . "</td>";
    	
    	echo "</tr>";

    }
    echo "</table>";
    
} else {
    echo "0 results";
}

mysqli_close($con);

return;
