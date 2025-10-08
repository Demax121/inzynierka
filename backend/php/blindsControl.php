<?php
header('Content-Type: application/json');
require_once 'config.php';


$tinytuyaIP = TINYTUYA_IP;   

$action = $_GET['action'] ?? '';

switch($action) {
    case "open":
        $url = $tinytuyaIP . "/?cmd=open";
        break;
    case "close":
        $url = $tinytuyaIP . "/?cmd=close";
        break;
    case "status":
        $url = $tinytuyaIP . "/?cmd=status";
        break;
    default:
        http_response_code(400);
        echo json_encode(["error" => "Unknown action"]);
        exit;
}

// wykonanie requestu curl
$ch = curl_init();
curl_setopt($ch, CURLOPT_URL, $url);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
$response = curl_exec($ch);

if ($response === false) {
    http_response_code(500);
    echo json_encode(["error" => "Curl error", "details" => curl_error($ch)]);
    curl_close($ch);
    exit;
}

$httpcode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
curl_close($ch);

// ustaw status HTTP zgodny z backendem
http_response_code($httpcode);

// zwróć dokładnie to, co backend (JSON)
echo $response;
?>
