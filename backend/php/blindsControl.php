<?php
$client_id = "qs7usxh347ecdnwhqq4y";
$client_secret = "0a6e950020a245a5867b932193ee20e2";
$device_id = "bfc1799d3b6346bedb2ygr";
$api_endpoint = "https://openapi.tuyaeu.com";

function getToken() {
    global $client_id, $client_secret, $api_endpoint;
    
    $url = "$api_endpoint/v1.0/token?grant_type=1";
    $timestamp = strval(floor(microtime(true) * 1000));
    
    $sign_str = $client_id . $timestamp . "GET\n" . 
                hash("sha256", "") . "\n\n" . 
                "/v1.0/token?grant_type=1";
    
    $signature = strtoupper(hash_hmac("sha256", $sign_str, $client_secret));
    
    $headers = [
        "client_id: $client_id",
        "sign: $signature",
        "t: $timestamp",
        "sign_method: HMAC-SHA256"
    ];
    
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    
    $response = curl_exec($ch);
    curl_close($ch);
    
    return json_decode($response, true);
}

function controlBlinds($access_token, $action) {
    global $client_id, $client_secret, $device_id, $api_endpoint;
    
    // Validate action
    if (!in_array($action, ['open', 'close'])) {
        return ['success' => false, 'msg' => 'Invalid action. Use "open" or "close".'];
    }
    
    $url = "$api_endpoint/v1.0/devices/$device_id/commands";
    $timestamp = strval(floor(microtime(true) * 1000));
    
    $body = json_encode([
        "commands" => [
            ["code" => "control", "value" => $action]
        ]
    ], JSON_UNESCAPED_SLASHES);
    
    $sign_str = $client_id . $access_token . $timestamp . "POST\n" . 
                hash("sha256", $body) . "\n\n" . 
                "/v1.0/devices/$device_id/commands";
    
    $signature = strtoupper(hash_hmac("sha256", $sign_str, $client_secret));
    
    $headers = [
        "client_id: $client_id",
        "access_token: $access_token",
        "sign: $signature",
        "t: $timestamp",
        "sign_method: HMAC-SHA256",
        "Content-Type: application/json"
    ];
    
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_POST, true);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $body);
    curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_ENCODING, '');
    curl_setopt($ch, CURLOPT_MAXREDIRS, 10);
    curl_setopt($ch, CURLOPT_TIMEOUT, 0);
    curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true);
    curl_setopt($ch, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    
    $response = curl_exec($ch);
    curl_close($ch);
    
    return json_decode($response, true);
}

header('Content-Type: application/json');

// Get the action from query parameter or POST data
$action = $_GET['action'] ?? $_POST['action'] ?? null;

if (!$action) {
    echo json_encode(['success' => false, 'msg' => 'Action parameter is required. Use ?action=open or ?action=close'], JSON_PRETTY_PRINT);
    exit;
}

$token_data = getToken();

if ($token_data['success']) {
    $access_token = $token_data['result']['access_token'];
    $result = controlBlinds($access_token, $action);
    echo json_encode($result, JSON_PRETTY_PRINT);
} else {
    echo json_encode($token_data, JSON_PRETTY_PRINT);
}
?>
