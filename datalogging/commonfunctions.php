<?php

function makeTR($data) {
	$rtn = "";
	foreach ($data as $item) {
		$rtn = $rtn . makeTD($item);
	}  

	return "<tr>" . $rtn . "</tr>";
}

function makeTD($data) {
	return "<td>" . $data . "</td>";
}

function rightWEllipsis($data, $length) {
	if (strlen($data) < $length-1) {
		return $data;
	} else {
		return "... " . substr($data, strlen($data) - $length, $length);
	}
}

?>