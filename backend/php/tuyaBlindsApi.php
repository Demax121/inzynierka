<?php
require_once 'config.php';

header('Content-Type: application/json; charset=utf-8');
ini_set('display_errors', 1);
error_reporting(E_ALL);

/*
 * tuyaBlindsApi.php
 *
 * Purpose:
 *   Thin proxy / adapter between the backend and the Tuya Cloud API for controlling a
 *   single blinds device (open, close) and fetching its current status (battery + state).
 *
 * Endpoints (query or POST param 'action'):
 *   ?action=status  (default) → returns basic device status (battery %, control state)
 *   ?action=open             → sends "open" control command
 *   ?action=close            → sends "close" control command
 *
 * HTTP Method:
 *   GET or POST are both accepted for simplicity (action read from either).
 *
 * Security / Notes:
 *   - Relies on constants defined in config.php (TUYA_* and TUYA_DEVICE_ID).
 *   - Each request obtains a fresh token (could be optimized with caching if rate limits matter).
 *   - No additional headers added here beyond JSON content-type (Caddy manages CORS/security).
 *   - Errors from Tuya are surfaced in a simplified form: success flag + optional 'error' field.
 *
 * Response Shapes:
 *   Status:
 *     {
 *       "success": true,
 *       "battery_percent": <int|null>,
 *       "blinds_state": "open" | "close" | <other/raw> | null
 *     }
 *   Command (open|close):
 *     {
 *       "success": true|false,
 *       "result": <raw Tuya result payload or null>,
 *       "error":  <error message from Tuya or null>
 *     }
 */

/**
 * Obtain an access token from Tuya Cloud.
 *
 * Flow:
 *   - Build canonical string for signing (timestamp + method/path).
 *   - HMAC-SHA256 sign with client secret.
 *   - Perform GET to /v1.0/token?grant_type=1
 *
 * @return string|null  access_token or null on failure
 */
function tuyaToken() {
    $url = TUYA_API_ENDPOINT . "/v1.0/token?grant_type=1";
    $t = strval(floor(microtime(true) * 1000));

    // Sign string format: client_id + t + "GET\n" + SHA256(body) + "\n\n" + path+query
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
    return ($data['success'] ?? false) ? ($data['result']['access_token'] ?? null) : null;
}

/**
 * Generic Tuya API request helper.
 *
 * @param string      $method       "GET" or "POST"
 * @param string      $path         API path (starting with /v1.0/...)
 * @param string      $access_token Token from tuyaToken()
 * @param array|null  $bodyArr      Optional body (for POST)
 *
 * @return array|null Decoded JSON response from Tuya
 */
function tuyaRequest($method, $path, $access_token, $bodyArr = null) {
    $t = strval(floor(microtime(true) * 1000));

    // Body serialization (empty string for GET)
    $body = $bodyArr ? json_encode($bodyArr, JSON_UNESCAPED_SLASHES) : "";

    // Canonical sign string:
    // client_id + access_token + timestamp + method + "\n" + SHA256(body) + "\n\n" + path
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

/**
 * Fetch simplified blinds status: battery percentage + control state.
 *
 * @param string $access_token
 * @return array { success: bool, battery_percent: int|null, blinds_state: string|null }
 */
function getBlindsStatusSimple($access_token) {
    $path = "/v1.0/iot-03/devices/" . TUYA_DEVICE_ID . "/status";
    $result = tuyaRequest("GET", $path, $access_token);

    $battery = null;
    $state = null;

    if (isset($result['result']) && is_array($result['result'])) {
        foreach ($result['result'] as $dp) {
            if (($dp['code'] ?? null) === 'battery_percentage') {
                $battery = $dp['value'];
            }
            if (($dp['code'] ?? null) === 'control') {
                $state = $dp['value'];
            }
        }
    }

    return [
        'success'         => true,
        'battery_percent' => $battery,
        'blinds_state'    => $state
    ];
}

// --- Routing / Dispatch ---

// Accept action from GET or POST; default to 'status'
$action = $_GET['action'] ?? $_POST['action'] ?? 'status';

// Obtain fresh token (no caching layer here)
$access_token = tuyaToken();
if (!$access_token) {
    echo json_encode(['success' => false, 'error' => 'Brak tokena Tuya']);
    exit;
}

switch ($action) {
    case 'status':
        echo json_encode(getBlindsStatusSimple($access_token), JSON_UNESCAPED_UNICODE);
        break;

    case 'open':
    case 'close':
        // Send a control command (open / close)
        $path = "/v1.0/devices/" . TUYA_DEVICE_ID . "/commands";
        $body = [
            "commands" => [
                [
                    "code"  => "control",
                    "value" => $action
                ]
            ]
        ];
        $result = tuyaRequest("POST", $path, $access_token, $body);
        echo json_encode([
            'success' => $result['success'] ?? false,
            'result'  => $result['result'] ?? null,
            'error'   => $result['msg'] ?? null
        ], JSON_UNESCAPED_UNICODE);
        break;

    default:
        // Invalid action guard
        echo json_encode([
            'success' => false,
            'error'   => 'Invalid or missing action. Use ?action=open|close|status'
        ]);
        break;
}
?>