<?php
header('Content-Type: application/json; charset=utf-8');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

require_once 'config.php';

$data = json_decode(file_get_contents('php://input'), true);
if (!is_array($data)) {
    http_response_code(400);
    exit(json_encode(['success' => false, 'error' => 'Invalid JSON']));
}

$status = null;
if (isset($data['status']) && is_string($data['status'])) {
    $s = trim($data['status']);
    if ($s === 'Door open' || $s === 'Door closed') {
        $status = $s;
    }
}

if (!$status) {
    http_response_code(400);
    exit(json_encode(['success' => false, 'error' => 'Invalid status value']));
}

try {
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER, DB_PASS,
        [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
    );

    $pdo->prepare('INSERT INTO door_status (status_date, status_name) VALUES (NOW(), :status)')
        ->execute([':status' => $status]);

    echo json_encode(['success' => true]);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(['success' => false, 'error' => $e->getMessage()], JSON_UNESCAPED_UNICODE);
}
?>