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

// Get the config info
$ini_array = parse_ini_file("rfidconfig.ini", true);
$photoServer = $ini_array["CRM"]["photoServer"];        // if true, running in sandbox
$dbUser = $ini_array["SQL_DB"]["readOnlyUser"];
$dbPassword = $ini_array["SQL_DB"]["readOnlyPassword"];
$dbName = $ini_array["SQL_DB"]["dataBaseName"];

$con = mysqli_connect("localhost",$dbUser,$dbPassword,$dbName);

// Check connection
if (mysqli_connect_errno()) {
    echo "Failed to connect to MySQL: " . mysqli_connect_error();
}

$selectSQL = 
"CALL sp_checkedInDisplay('" . date_format($today, "Ymd") . "');" ;

$result = mysqli_query($con, $selectSQL);
echo mysqli_error($con);

// Construct the page

$html =  str_replace("<<REFRESHTIME>>","Updated: " . date_format($today, "Y-m-d H:i:s"),$html);

if (mysqli_num_rows($result) > 0) {
	
    // output data of each row
    $currentClientID = "";
    $currentFirstName = "";
    $currentEquipment = "";
    $firstIteration = true;
    while($row = mysqli_fetch_assoc($result)) {

        if ($row["clientID"] != $currentClientID) {
            // new client

            if ($firstIteration) {
                //
                $firstIteration = false;
            } else {
                // create div for previous clientID
                $thisDiv = makeDiv($currentFirstName , $currentClientID, $currentEquipment, $photoServer  ) . "\r\n";
            
                // add div to output accumulation
                $photodivs = $photodivs . $thisDiv;
            }    

            // set up for the next client
            $currentFirstName = $row["firstName"];
            $currentEquipment = $row["photoDisplay"];
            $currentClientID = $row["clientID"];

        } else {

            // same client, add the equipment name
            $currentEquipment = $currentEquipment . " " . $row["photoDisplay"];    
        } 

    }
    // last element from loop
    $thisDiv = makeDiv($currentFirstName , $currentClientID, $currentEquipment, $photoServer ) . "\r\n";
    $photodivs = $photodivs . $thisDiv;

    $html = str_replace("<<PHOTODIVS>>",$photodivs, $html);
    
} else {

    $html = str_replace("<<PHOTODIVS>>","No Records Found",$html);

}

echo $html;

mysqli_close($con);

return;

function makeImageURL($data, $photoServer) {
	return "<img class='IDPhoto' alt='no photo' src='" . $photoServer . $data . ".jpg'>";
}
function makeDiv($name, $clientID, $equip, $photoServer) {
  return "<div class='photodiv' >" . makeTable($name, $clientID, $equip, $photoServer) . "</div>";
}
function makeTable($name, $clientID, $equip, $photoServer){
  return "<table class='clientTable'><tr><td class='clientImageTD'>" . makeImageURL($clientID, $photoServer) . 
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