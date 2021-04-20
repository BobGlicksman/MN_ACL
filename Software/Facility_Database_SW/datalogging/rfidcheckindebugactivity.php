
<?php

// report from RFID LOGGING DATABASE
//
// checkin debug activity reports
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

include 'commonfunctions.php';

// get the HTML skeleton

$html = file_get_contents("rfidcheckindebugactivity.txt");
if (!$html){
  die("unable to open file");
}

// Get the data
$ini_array = parse_ini_file("rfidconfig.ini", true);
$dbUser = $ini_array["SQL_DB"]["readOnlyUser"];
$dbPassword = $ini_array["SQL_DB"]["readOnlyPassword"];
$dbName = $ini_array["SQL_DB"]["dataBaseName"];

$con = mysqli_connect("localhost",$dbUser,$dbPassword,$dbName);

// Check connection
if (mysqli_connect_errno()) {
    echo "Failed to connect to MySQL: " . mysqli_connect_error();
  }


// ------------ TABLE 1  
  
$selectSQLCheckEventsPerMonth = "
SELECT MONTH(dateEventLocal) as mnth, YEAR(dateEventLocal) as yr, count(recNum) as cnt
FROM `rawdata` rd
WHERE dateEventLocal > '20191001'
  and eventName = 'RFIDLogCheckInOut'
group by YEAR(dateEventLocal), MONTH(dateEventLocal)
order by YEAR(dateEventLocal), MONTH(dateEventLocal)
";

$result = mysqli_query($con, $selectSQLCheckEventsPerMonth);
$dataX = "";
$dataY = "";

// Construct the page
if (mysqli_num_rows($result) > 0) {

	// Get the data for each month into table rows
    while($row = mysqli_fetch_assoc($result)) {
    	
    	$thisTableRow = makeTR( array (
                 $row["yr"], 
                 $row["mnth"],
                 $row["cnt"]
                 )     
           );
      
        if ($dataX == "") {
            $dataX =  $row["yr"] . "/" . $row["mnth"];
            $dataY =  $row["cnt"];
        } else {
            $dataX = $dataX . " " . $row["yr"] . "/" . $row["mnth"];
            $dataY = $dataY . " " . $row["cnt"];
        }
    	$tableRows = $tableRows . $thisTableRow;
    }
    
    $html = str_replace("<<GRAPH1DATAX>>",$dataX,$html);
    $html = str_replace("<<GRAPH1DATAY>>",$dataY,$html);

    $html = str_replace("<<TABLEHEADER_RecordsPerMonth>>",
    	makeTR(
    		array( 
    			"Year",
    			"Month",
    			"num of RFIDLogCheckInOut"
    			)
    		),
    	$html);
    $html = str_replace("<<TABLEROWS_RecordsPerMonth>>", $tableRows,$html);

} else {
    echo "0 results";
}


// ------------ TABLE 2  

$tableRows = "";

$selectSQLLogRecordsPerDay = "
SELECT DAYOFYEAR(dateEventLocal) as DOY, MONTH(dateEventLocal) as mnth, YEAR(dateEventLocal) as yr, count(recNum) as cnt
FROM `rawdata` rd
WHERE dateEventLocal > '20191001'
group by YEAR(dateEventLocal), MONTH(dateEventLocal)
order by YEAR(dateEventLocal), MONTH(dateEventLocal)
";



$result2 = mysqli_query($con, $selectSQLLogRecordsPerDay);

// Construct the page
$dataX = "";
$dataY = "";

if (mysqli_num_rows($result2) > 0) {

	// Get the data for each day into table rows
    while($row = mysqli_fetch_assoc($result2)) {
    
        $thisTableRow = makeTR( array (
            $row["yr"], 
            $row["mnth"],
            $row["cnt"]
            )     
        );
    
        if ($dataX == "") {
            $dataX =  $row["yr"] . "/" . $row["mnth"];
            $dataY =  $row["cnt"];
        } else {
            $dataX = $dataX . " " . $row["yr"] . "/" . $row["mnth"];
            $dataY = $dataY . " " . $row["cnt"];
        }
        $tableRows = $tableRows . $thisTableRow;

        /*
        // this code puts in a row per DOY and inserts 0's when needed
        $thisDOY = intval($row["DOY"]);
        
        $thisTableRow = makeTR( array (
                    $row["DOY"],
                    $row["yr"], 
                    $row["mnth"],
                    $row["dy"],
                    $row["cnt"]
                    )     
            );
        if ($dataX == "") {
            // first row
            $previousDOY = $row["yr"] . "/" . $thisDOY;
            $dataX = $thisDOY;
            $dataY =  $row["cnt"];
        } else {
            while ($thisDOY > ($previousDOY + 1)) {
                // if we have a gap in day of year, add 0 data values
                $previousDOY = $previousDOY + 1;
                $dataX = $dataX . " " . $row["yr"] . "/" . $previousDOY;
                $dataY = $dataY . " 0";
            }
            $previousDOY = $thisDOY;
            $dataX = $dataX . " " . $row["yr"] . "/" . $thisDOY;
            $dataY = $dataY . " " . $row["cnt"];
        }
        $tableRows = $tableRows . $thisTableRow;
        */
    }
    
    $html = str_replace("<<GRAPH2DATAX>>",$dataX,$html);
    $html = str_replace("<<GRAPH2DATAY>>",$dataY,$html);

    $html = str_replace("<<TABLEHEADER_LogRecordsPerDay>>",
    	makeTR(
    		array( 
                "Year",
                "Month",
    			"Count"
    			)
    		),
    	$html);
    $html = str_replace("<<TABLEROWS_LogRecordsPerDay>>", $tableRows,$html);

} else {
    echo "0 results";
}

/*
// ------------- Report Data 3 ---------------

$tableRows = "";

$SQLDateRange = date("'Y-m-d'",strtotime("60 days ago")) . " AND " .  date("'Y-m-d'",time()) ;

$selectSQLMembersLast90Days = "
SELECT COUNT(DISTINCT rd.clientID) as numUnique
FROM rawdata rd
LEFT JOIN clientInfo ci
ON rd.clientID = ci.clientID
WHERE dateEventLocal BETWEEN "
. $SQLDateRange .
" and logEvent = 'Checked In'
  and (TRIM(displayClasses) <> 'staff' OR displayClasses is NULL);
";

$result3 = mysqli_query($con, $selectSQLMembersLast90Days);

if (mysqli_num_rows($result3) > 0) {

    $row = mysqli_fetch_assoc($result3);
    $html = str_replace("<<UNIQUE90>>", $row["numUnique"],$html);

} else {
    $html = str_replace("<<UNIQUE90>>", "no results found",$html);
}

*/

// ------------- final output ----------

echo $html;

mysqli_close($con);

return;

?>