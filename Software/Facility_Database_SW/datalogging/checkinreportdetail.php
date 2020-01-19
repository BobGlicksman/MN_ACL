
<?php

// 1 report from RFID LOGGING DATABASE
//
// Member detail checkin reports
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

include 'commonfunctions.php';

// get the HTML skeleton
$myfile = fopen("checkinreport.txt", "r") or die("Unable to open file!");
$html = fread($myfile,filesize("checkinreport.txt"));
fclose($myfile);

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
SELECT YEAR(dateEventLocal) as yr, month(dateEventLocal) as mnth, clientID, firstName, logEvent 
FROM `rawdata` 
WHERE dateEventLocal > '20191001'
  and logEvent = 'Checked In'
group by YEAR(dateEventLocal),month(dateEventLocal), clientID
order by YEAR(dateEventLocal),month(dateEventLocal), firstName;
";

$result = mysqli_query($con, $selectSQLMembersPerMonth);

// Construct the page

if (mysqli_num_rows($result) > 0) {

	// Get the data for each month into table rows
  $previousMonth = "---";
  while($row = mysqli_fetch_assoc($result)) {
    
    // Add a header line if changing months
    $thisMonth = $row["mnth"];
    if ($thisMonth != $previousMonth) {
      $tableRows = $tableRows . makeTR( array ($row["yr"],$row["mnth"]," "," "));
      $previousMonth = $thisMonth;
    }

    $thisTableRow = makeTR( array (
                " ", 
                " ",
                $row["firstName"]
                ,
                $row["clientID"]
                )     
        );

    $tableRows = $tableRows . $thisTableRow;

  }
    
  $html = str_replace("<<TABLEHEADER_MembersPerMonth>>",
    makeTR(
      array( 
        "Year",
        "Month",
        "First Name",
        "Client ID"
        )
      ),
    $html);
    $html = str_replace("<<TABLEROWS_MembersPerMonth>>", $tableRows,$html);

} else {
    echo "0 results";
}


echo $html;

mysqli_close($con);

return;



?>