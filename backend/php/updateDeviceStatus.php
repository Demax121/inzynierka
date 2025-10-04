<?php
header('Content-Type: application/json; charset=utf-8');

/*
 * updateDeviceStatus.php
 *
 * Purpose:
 *   Update the device_last_seen timestamp for a device (heartbeat / presence update)
 *   identified by its device_api_key in the devices table.
 *
 * Method:
 *   POST (OPTIONS allowed for preflight)
 *
 * Request JSON:
 *   { "device_api_key": "<string>" }
 *
 * Responses:
 *   204 (OPTIONS preflight): no body
 *   200: { "success": true }
 *   400: { "success": false, "error": "Missing device_api_key" }
 *   404: { "success": false, "error": "Device not found" }
 *   405: { "success": false, "error": "Method not allowed" }
 *   500: { "success": false, "error": "<database error message>" }
 *
 * Notes:
 *   - Only updates device_last_seen via NOW(); no other fields are touched.
 *   - device_api_key must already exist; otherwise 404 is returned.
 *   - No extra headers here (Caddy handles CORS/security globally).
 */

// Handle preflight (acknowledge and exit).
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

// Enforce POST only.
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    exit(json_encode(['success' => false, 'error' => 'Method not allowed'], JSON_UNESCAPED_UNICODE));
}

require_once 'config.php';

// Decode incoming JSON.
$input = json_decode(file_get_contents('php://input'), true);

// Validate required key.
if (!isset($input['device_api_key'])) {
    http_response_code(400);
    exit(json_encode(['success' => false, 'error' => 'Missing device_api_key'], JSON_UNESCAPED_UNICODE));
}

$deviceApiKey = $input['device_api_key'];

try {
    // Open DB connection (exception mode).
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER,
        DB_PASS,
        [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
    );

    // Update last seen timestamp.
    $updateStmt = $pdo->prepare('UPDATE devices SET device_last_seen = NOW() WHERE device_api_key = :apiKey');
    $updateStmt->bindParam(':apiKey', $deviceApiKey, PDO::PARAM_STR);
    $updateStmt->execute();

    if ($updateStmt->rowCount() > 0) {
        echo json_encode(['success' => true], JSON_UNESCAPED_UNICODE);
    } else {
        http_response_code(404);
        echo json_encode(['success' => false, 'error' => 'Device not found'], JSON_UNESCAPED_UNICODE);
    }

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode([
        'success' => false,
        'error'   => $e->getMessage()
    ], JSON_UNESCAPED_UNICODE);
}
?>