<?php
header('Content-Type: application/json; charset=utf-8');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    exit(json_encode(['success' => false, 'error' => 'Method not allowed'], JSON_UNESCAPED_UNICODE));
}

require_once 'config.php';

$input = json_decode(file_get_contents('php://input'), true);

if (!isset($input['device_api_key'])) {
    http_response_code(400);
    exit(json_encode(['success' => false, 'error' => 'Missing device_api_key'], JSON_UNESCAPED_UNICODE));
}

$deviceApiKey = $input['device_api_key'];

try {
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER, DB_PASS,
        [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
    );

    // Update the device_last_seen column for the device identified by the API key
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
        'error' => $e->getMessage()
    ], JSON_UNESCAPED_UNICODE);
}
?>