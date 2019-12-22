<?php

// Show photos of everyone who is checked in today.
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

include 'commonfunctions.php';

allowWebAccess();  // if IP not allowed, then die

$today = new DateTime(); 
$today->setTimeZone(new DateTimeZone("America/Los_Angeles")); 

// get the HTML skeleton
$myfile = fopen("rfidcurrentcheckinshtml.txt", "r") or die("Unable to open file!");
$html = fread($myfile,filesize("rfidcurrentcheckinshtml.txt"));
fclose($myfile);

// Get the data
$ini_array = parse_ini_file("rfidconfig.ini", true);
$dbUser = $ini_array["SQL_DB"]["readOnlyUser"];
$dbPassword = $ini_array["SQL_DB"]["readOnlyPassword"];
$dbName = $ini_array["SQL_DB"]["dataBaseName"];

$con = mysqli_connect("localhost",$dbUser,$dbPassword,$dbName);
  
$selectSQL = 
"SELECT * FROM rawdata JOIN
(SELECT * FROM
	(
     SELECT  MAX(recNum) as maxRecNum, clientID 
        FROM rawdata
       WHERE logEvent in ('Checked In','Checked Out')
         AND CONVERT( dateEventLocal, DATE) = CONVERT('"
 . date_format($today, "Y-m-d") . 
                                "', DATE) 
         AND clientID <> 0
      GROUP by clientID
    ) AS x
    LEFT JOIN
    (
     SELECT max(recNum) as maxRecNum2, clientID AS clientID2, 'Yes' as woodshop 
        FROM rawdata
        WHERE logEvent in ('Woodshop allowed')
        AND CONVERT( dateEventLocal, DATE) = CONVERT( '" 
. date_format($today, 'Y-m-d') . 
                                "', DATE) 
         AND clientID <> 0
      GROUP by clientID
    ) as y 
    ON x.clientID = y.clientID2 
) as z 
on rawdata.recNum = z.maxRecNum
where logEvent = 'Checked In' 
ORDER BY rawdata.dateEventLocal DESC";



// Check connection
if (mysqli_connect_errno()) {
  echo "Failed to connect to MySQL: " . mysqli_connect_error();
}

$result = mysqli_query($con, $selectSQL);

// Construct the page

$html =  str_replace("<<REFRESHTIME>>","Updated: " . date_format($today, "Y-m-d H:i:s"),$html);

if (mysqli_num_rows($result) > 0) {
	
  // output data of each row
  while($row = mysqli_fetch_assoc($result)) {
    
    $equip = "";
    if ($row["woodshop"] == "Yes") {
      $equip = $equip . "Wood";
    }

    $thisDiv = makeDiv( $row["firstName"], $row["clientID"], $equip ) . "\r\n";
    
    $photodivs = $photodivs . $thisDiv;
  }
  
  $html = str_replace("<<PHOTODIVS>>",$photodivs, $html);

    
} else {

    $html = str_replace("<<PHOTODIVS>>","No Records Found",$html);

}

echo $html;

mysqli_close($con);

return;

function makeImageURL($data) {
	return "<img class='IDPhoto' alt='no photo' src='https://c226212.ssl.cf0.rackcdn.com/" . $data . ".jpg'>";
}
function makeDiv($name, $clientID, $equip) {
  return "<div class='photodiv' >" . makeTable($name, $clientID, $equip) . "</div>";
}
function makeTable($name, $clientID, $equip){
  return "<table class='clientTable'><tr><td class='clientImageTD'>" . makeImageURL($clientID) . 
  "</td></tr><tr><td class='clientNameTD'>" . makeNameCheckoutAction($clientID, $name) . 
  "</td></tr><tr><td class='clientEquipTD'>" . makeEquipList($equip) . "</td></tr></table>";
}	
function makeNameCheckoutAction($clientID, $name) {
  return "<p class='photoname' onclick=\"checkout('" . $clientID . "','" . $name . "')\">" . $name . "</p>";
}
function makeEquipList($equip){
  return "<p class='equiplist'>" . $equip . "</p>";
}

?>