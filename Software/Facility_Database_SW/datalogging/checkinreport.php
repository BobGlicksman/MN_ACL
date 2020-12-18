
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
SELECT MONTH(dateEventLocal) as mnth, YEAR(dateEventLocal) as yr, rd.clientID, rd.firstName, logEvent 
FROM `rawdata` rd
LEFT JOIN clientInfo ci
ON rd.clientID = ci.clientID
WHERE dateEventLocal > '20191001'
  and logEvent = 'Checked In'
  and (TRIM(displayClasses) <> 'staff' or displayClasses is NULL)
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
SELECT COUNT(*) as cnt, dy, mnth, yr, DOY
FROM
(
SELECT DAYOFYEAR(dateEventLocal) as DOY, DAY(dateEventLocal) as dy, MONTH(dateEventLocal) as mnth, YEAR(dateEventLocal) as yr, rd.clientID, rd.firstName, logEvent 
FROM `rawdata` rd
LEFT JOIN clientInfo ci
ON rd.clientID = ci.clientID
WHERE dateEventLocal > '20191001'
  and logEvent = 'Checked In'
  and (TRIM(displayClasses) <> 'staff' or displayClasses is NULL)
group by YEAR(dateEventLocal), MONTH(dateEventLocal), DAY(dateEventLocal), clientID
order by YEAR(dateEventLocal), MONTH(dateEventLocal), DAY(dateEventLocal)
) as X
GROUP BY yr, mnth, dy, DOY
ORDER BY yr, mnth, dy, DOY;
";

$result2 = mysqli_query($con, $selectSQLMembersPerDay);

// Construct the page
$dataX = "";
$dataY = "";

if (mysqli_num_rows($result2) > 0) {

	// Get the data for each day into table rows
  $previousClientID = "---";
  $previousDOY = 0;
  while($row = mysqli_fetch_assoc($result2)) {
    
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
    }
    
    $html = str_replace("<<GRAPH2DATAX>>",$dataX,$html);
    $html = str_replace("<<GRAPH2DATAY>>",$dataY,$html);

    $html = str_replace("<<TABLEHEADER_MembersPerDay>>",
    	makeTR(
    		array( 
          "DOY",
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

// ------------- final output ----------

echo $html;

mysqli_close($con);

return;

?>