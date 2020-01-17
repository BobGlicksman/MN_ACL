<?php

// Common functions used by server scripts
//
// Creative Commons: Attribution/Share Alike/Non Commercial (cc) 2019 Maker Nexus
// By Jim Schrempp

// if the client ip address is not on the allowed list, the web page will die
function allowWebAccess() {

	// xxx for now return true until we get the public ip of MN
	return;

	$allowedIPs = array (
		'34.197.209.41'   // Particle.io cloud (all our devices)
		,'107.139.32.104'   // Jim in San Carlos
	);

	$rtnvalue = false;
	if (!in_array($_SERVER['REMOTE_ADDR'], $allowedIPs)) {
		die("access from this ip is not allowed: " . $_SERVER['REMOTE_ADDR']);
	} 

}

// Make a table row of $data
function makeTR($data) {
	$rtn = "";
	foreach ($data as $item) {
		$rtn = $rtn . makeTD($item);
	}  

	return "<tr>" . $rtn . "</tr>";
}

// make a table data cell of $data
function makeTD($data) {
	return "<td>" . $data . "</td>";
}

// take $length of the end of a string and put elipsis as a prefix in front of it
function rightWEllipsis($data, $length) {
	if (strlen($data) < $length-1) {
		return $data;
	} else {
		return "... " . substr($data, strlen($data) - $length, $length);
	}
}

?>