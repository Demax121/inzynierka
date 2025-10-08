<?php
header('Content-Type: application/json'); // powiedz przeglądarce/curl że zwracasz JSON

$command = $_GET['cmd'] ?? '';

switch($command) {
    case "open":
        $output = shell_exec("/var/www/html/venv/bin/python /var/www/html/blinds_open.py 2>&1");
        break;
    case "close":
        $output = shell_exec("/var/www/html/venv/bin/python /var/www/html/blinds_close.py 2>&1");
        break;
    case "status":
        $output = shell_exec("/var/www/html/venv/bin/python /var/www/html/blinds_status.py 2>&1");
        break;
    default:
        http_response_code(400);
        echo json_encode(["error" => "Unknown command"]);
        exit;
}

// If python has worked correctly, return its output in JSON format
if ($output) {
    echo $output;
} else {
    http_response_code(500);
    echo json_encode(["error" => "No output from Python"]);
}
?>
