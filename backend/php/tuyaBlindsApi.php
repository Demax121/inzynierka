<?php
// Ustawienia
$client_id = "qs7usxh347ecdnwhqq4y";
$client_secret = "0a6e950020a245a5867b932193ee20e2";
$device_id = "bfc1799d3b6346bedb2ygr";
$api_endpoint = "https://openapi.tuyaeu.com";

header('Content-Type: application/json; charset=utf-8');
ini_set('display_errors', 1);
error_reporting(E_ALL);

// --- Funkcje wspólne ---
function tuyaToken() {
    global $client_id, $client_secret, $api_endpoint;
    $url = "$api_endpoint/v1.0/token?grant_type=1";
    $t = strval(floor(microtime(true) * 1000));
    $sign_str = $client_id . $t . "GET\n" . hash("sha256", "") . "\n\n" . "/v1.0/token?grant_type=1";
    $sign = strtoupper(hash_hmac("sha256", $sign_str, $client_secret));
    $headers = [
        "client_id: $client_id",
        "sign: $sign",
        "t: $t",
        "sign_method: HMAC-SHA256"
    ];
    $ch = curl_init($url);
    curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    $resp = curl_exec($ch);
    curl_close($ch);
    $data = json_decode($resp, true);
    return $data['success'] ? $data['result']['access_token'] : null;
}

function tuyaRequest($method, $path, $access_token, $bodyArr = null) {
    global $client_id, $client_secret, $api_endpoint;
    $t = strval(floor(microtime(true) * 1000));
    $body = $bodyArr ? json_encode($bodyArr, JSON_UNESCAPED_SLASHES) : "";
    $sign_str = $client_id . $access_token . $t . "$method\n" . hash("sha256", $body) . "\n\n" . $path;
    $sign = strtoupper(hash_hmac("sha256", $sign_str, $client_secret));
    $headers = [
        "client_id: $client_id",
        "access_token: $access_token",
        "sign: $sign",
        "t: $t",
        "sign_method: HMAC-SHA256",
        "Content-Type: application/json"
    ];
    $url = "https://openapi.tuyaeu.com$path";
    $ch = curl_init($url);
    curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    if ($method === "POST") {
        curl_setopt($ch, CURLOPT_POST, true);
        curl_setopt($ch, CURLOPT_POSTFIELDS, $body);
    }
    $resp = curl_exec($ch);
    curl_close($ch);
    return json_decode($resp, true);
}

function getBlindsStatusSimple($access_token, $device_id) {
    $path = "/v1.0/iot-03/devices/$device_id/status";
    $result = tuyaRequest("GET", $path, $access_token);
    $battery = null; $state = null;
    if (isset($result['result']) && is_array($result['result'])) {
        foreach ($result['result'] as $dp) {
            if ($dp['code'] === 'battery_percentage') $battery = $dp['value'];
            if ($dp['code'] === 'control') $state = $dp['value'];
        }
    }
    return ['battery_percent' => $battery, 'blinds_state' => $state];
}

// --- Routing ---
$action = $_GET['action'] ?? $_POST['action'] ?? null;
if (!$action) $action = basename($_SERVER['SCRIPT_NAME']) === 'getBlindsStatus.php' ? 'status' : null;

$access_token = tuyaToken();
if (!$access_token) {
    echo json_encode(['success'=>false, 'error'=>'Opis błędu']);
    exit;
}

if ($action === 'status') {
    echo json_encode(getBlindsStatusSimple($access_token, $device_id), JSON_PRETTY_PRINT);
} elseif ($action === 'open' || $action === 'close') {
    $path = "/v1.0/devices/$device_id/commands";
    $body = ["commands" => [["code" => "control", "value" => $action]]];
    $result = tuyaRequest("POST", $path, $access_token, $body);
    echo json_encode($result, JSON_PRETTY_PRINT);
} else {
    echo json_encode(['success'=>false, 'msg'=>'Invalid or missing action. Use ?action=open|close|status']);
}
?>