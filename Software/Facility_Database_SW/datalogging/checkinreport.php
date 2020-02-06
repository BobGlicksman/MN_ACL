
<?php

// 2 reports from RFID LOGGING DATABASE
//
// Member summary checkin reports
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

include 'commonfunctions.php';

// get the HTML skeleton

$html = file_get_contents("checkinreport.txt");
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
  
$selectSQLMembersPerMonth = "
SELECT COUNT(*) as cnt, mnth, yr
FROM
(
SELECT MONTH(dateEventLocal) as mnth, YEAR(dateEventLocal) as yr, clientID, firstName, logEvent FROM `rawdata` 
WHERE dateEventLocal > '20191001'
  and logEvent = 'Checked In'
group by YEAR(dateEventLocal), MONTH(dateEventLocal), clientID
order by YEAR(dateEventLocal), MONTH(dateEventLocal)
) as X
GROUP BY yr, mnth
ORDER BY yr, mnth;
";

$result = mysqli_query($con, $selectSQLMembersPerMonth);
$dataX = "";
$dataY = "";

// Construct the page
if (mysqli_num_rows($result) > 0) {

	// Get the data for each month into table rows
	$previousClientID = "---";
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

    $html = str_replace("<<TABLEHEADER_MembersPerMonth>>",
    	makeTR(
    		array( 
    			"Year",
    			"Month",
    			"Unique Members"
    			)
    		),
    	$html);
    $html = str_replace("<<TABLEROWS_MembersPerMonth>>", $tableRows,$html);

} else {
    echo "0 results";
}


// ------------ TABLE 2  

$tableRows = "";

$selectSQLMembersPerDay = "
SELECT COUNT(*) as cnt, dy, mnth, yr
FROM
(
SELECT DAY(dateEventLocal) as dy, MONTH(dateEventLocal) as mnth, YEAR(dateEventLocal) as yr, clientID, firstName, logEvent FROM `rawdata` 
WHERE dateEventLocal > '20191001'
  and logEvent = 'Checked In'
group by YEAR(dateEventLocal), MONTH(dateEventLocal), DAY(dateEventLocal), clientID
order by YEAR(dateEventLocal), MONTH(dateEventLocal), DAY(dateEventLocal)
) as X
GROUP BY yr, mnth, dy
ORDER BY yr, mnth, dy;
";

$result2 = mysqli_query($con, $selectSQLMembersPerDay);

// Construct the page
$dataX = "";
$dataY = "";

if (mysqli_num_rows($result2) > 0) {

	// Get the data for each month into table rows
	$previousClientID = "---";
    while($row = mysqli_fetch_assoc($result2)) {
    	
    	$thisTableRow = makeTR( array (
                 $row["yr"], 
                 $row["mnth"],
                 $row["dy"],
                 $row["cnt"]
                 )     
       		);
      if ($dataX == "") {
        $dataX =  $row["yr"] . "/" . $row["mnth"] . "/" . $row["dy"];
        $dataY =  $row["cnt"];
      } else {
        $dataX = $dataX . " " . $row["yr"] . "/" . $row["mnth"] . "/" . $row["dy"];
        $dataY = $dataY . " " . $row["cnt"];
      }
    	$tableRows = $tableRows . $thisTableRow;
    }
    
    $html = str_replace("<<GRAPH2DATAX>>",$dataX,$html);
    $html = str_replace("<<GRAPH2DATAY>>",$dataY,$html);

    $html = str_replace("<<TABLEHEADER_MembersPerDay>>",
    	makeTR(
    		array( 
    			"Year",
                "Month",
                "Day",
    			"Unique Members"
    			)
    		),
    	$html);
    $html = str_replace("<<TABLEROWS_MembersPerDay>>", $tableRows,$html);

} else {
    echo "0 results";
}

// ------------- Report Data 3 ---------------

$tableRows = "";

$SQLDateRange = date("'Y-m-d'",strtotime("90 days ago")) . " AND " .  date("'Y-m-d'",time()) ;

$selectSQLMembersLast90Days = "
SELECT COUNT(DISTINCT clientID) as numUnique FROM rawdata 
WHERE dateEventLocal BETWEEN "
. $SQLDateRange .
" and logEvent = 'Checked In';
";

$result3 = mysqli_query($con, $selectSQLMembersLast90Days);

if (mysqli_num_rows($result3) > 0) {

    $row = mysqli_fetch_assoc($result3);
    $html = str_replace("<<UNIQUE90>>", $row["numUnique"],$html);

} else {
    $html = str_replace("<<UNIQUE90>>", "no results found",$html);
}

// ------------- final output ----------

echo $html;

mysqli_close($con);

return;

?>