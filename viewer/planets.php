<?php


$PATH_TO_INI  = "/var/lib/ra/planets.ini";
$PATH_TO_DATA = "/var/lib/ra/data.dat";

function err($e) {
	$r = array("status" => $e);
	return json_encode($r);
}

if (!isset($_GET['time'])) {
	echo err("Argument 'time' required");
	exit();
}

/* get time */
$time = intval($_GET['time']);

$ini = parse_ini_file($PATH_TO_INI, true);
if ($ini === false) {
	echo err("Can't open ini file");
	exit();
}

/* number of planets from ini file */
$nplanets = sizeof($ini);

$fsize = filesize($PATH_TO_DATA);
if ($fsize === false) {
	echo err("Can't get data file size");
	exit();
}

/* number of records in file */
$nrecords = $fsize / ($nplanets * 3 * 8);
if ($time >= $nrecords) {
	echo err("Incorrect time");
	exit();
}

$data = fopen($PATH_TO_DATA, "rb");
if ($data === false) {
	echo err("Can't open data file");
	exit();
}

if (fseek($data, $time * 3 * 8 * $nplanets) != 0) {
	echo err("Can't seek file");
	exit();
}


$planets = array();

for ($i=0; $i<$nplanets; $i++) {
	$rawcoord = fread($data, 3 * 8);
	if ($rawcoord === false) {
		echo err("Can't read file");
		exit();
	}
	$coords = unpack("d3c", $rawcoord);
	$planets[] = array("x" => $coords["c1"], "y" => $coords["c2"], "z" => $coords["c3"]);
}

$response = array("status" => "ok", "planets" => $planets);

if ($time == 0) {
	$response["nrecords"] = $nrecords;
}

echo json_encode($response);

fclose($data);

?>

