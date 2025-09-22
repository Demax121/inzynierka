<?php
require_once 'config.php';

header('Content-Type: application/json; charset=utf-8');
ini_set('display_errors', 1);
error_reporting(E_ALL);

// --- Funkcje wspólne ---
function tuyaToken() {
    $url = TUYA_API_ENDPOINT . "/v1.0/token?grant_type=1";
    $t = strval(floor(microtime(true) * 1000));
    $sign_str = TUYA_CLIENT_ID . $t . "GET\n" . hash("sha256", "") . "\n\n" . "/v1.0/token?grant_type=1";
    $sign = strtoupper(hash_hmac("sha256", $sign_str, TUYA_CLIENT_SECRET));
    $headers = [
        "client_id: " . TUYA_CLIENT_ID,
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
    $t = strval(floor(microtime(true) * 1000));
    $body = $bodyArr ? json_encode($bodyArr, JSON_UNESCAPED_SLASHES) : "";
    $sign_str = TUYA_CLIENT_ID . $access_token . $t . "$method\n" . hash("sha256", $body) . "\n\n" . $path;
    $sign = strtoupper(hash_hmac("sha256", $sign_str, TUYA_CLIENT_SECRET));
    $headers = [
        "client_id: " . TUYA_CLIENT_ID,
        "access_token: $access_token",
        "sign: $sign",
        "t: $t",
        "sign_method: HMAC-SHA256",
        "Content-Type: application/json"
    ];
    $url = TUYA_API_ENDPOINT . $path;
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

function getBlindsStatusSimple($access_token) {
    $path = "/v1.0/iot-03/devices/" . TUYA_DEVICE_ID . "/status";
    $result = tuyaRequest("GET", $path, $access_token);
    $battery = null; $state = null;
    if (isset($result['result']) && is_array($result['result'])) {
        foreach ($result['result'] as $dp) {
            if ($dp['code'] === 'battery_percentage') $battery = $dp['value'];
            if ($dp['code'] === 'control') $state = $dp['value'];
        }
    }
    return ['success' => true, 'battery_percent' => $battery, 'blinds_state' => $state];
}

// --- Routing ---
$action = $_GET['action'] ?? $_POST['action'] ?? 'status';

$access_token = tuyaToken();
if (!$access_token) {
    echo json_encode(['success'=>false, 'error'=>'Brak tokena Tuya']);
    exit;
}

switch ($action) {
    case 'status':
        echo json_encode(getBlindsStatusSimple($access_token), JSON_UNESCAPED_UNICODE);
        break;
    case 'open':
    case 'close':
        $path = "/v1.0/devices/" . TUYA_DEVICE_ID . "/commands";
        $body = ["commands" => [["code" => "control", "value" => $action]]];
        $result = tuyaRequest("POST", $path, $access_token, $body);
        echo json_encode(['success' => $result['success'] ?? false, 'result' => $result['result'] ?? null, 'error' => $result['msg'] ?? null], JSON_UNESCAPED_UNICODE);
        break;
    default:
        echo json_encode(['success'=>false, 'error'=>'Invalid or missing action. Use ?action=open|close|status']);
}
?>