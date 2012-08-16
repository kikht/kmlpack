<?php

$day = $_GET["day"];
$file = $_GET["file"];

if (!ctype_digit($day) || strlen($day) != 7) {
    header("HTTP/1.0 404 Not Found");
    echo "Wrong day";
    exit;
}

$db = dba_open("./".$day.".db", "r-", "db4");
if ($db === FALSE) {
    header("HTTP/1.0 404 Not Found");
    echo "Not found day db";
    exit;
}

$data = dba_fetch($file, $db);
if ($data === FALSE) {
    header("HTTP/1.0 404 Not Found");
    echo "<p>Not found file " . $file . "<p>";
    // $key = dba_firstkey($db);
    // while ($key !== FALSE) {
    //     print($key . "<br>");
    //     $key = dba_nextkey($db);
    // }
} else {
    print($data);
}

dba_close($db);

?>
