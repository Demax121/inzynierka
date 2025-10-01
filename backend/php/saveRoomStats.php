<?php
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

require_once 'config.php';

$raw = file_get_contents('php://input');
$data = json_decode($raw, true);

if (!is_array($data)) {
    http_response_code(400);
    echo json_encode(['success' => false, 'error' => 'Invalid JSON']);
    exit;
}


$temperature = isset($data['temperature']) ? floatval($data['temperature']) : null;
$humidity = isset($data['humidity']) ? floatval($data['humidity']) : null;
$pressure = isset($data['pressure']) ? floatval($data['pressure']) : null;

if (!is_float($temperature) || !is_float($humidity) || !is_float($pressure)) {
    http_response_code(400);
    echo json_encode(['success' => false, 'error' => 'temperature/humidity/pressure must be float']);
    exit;
}

try {
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER,
        DB_PASS,
        [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        ]
    );

    $stmt = $pdo->prepare('
        INSERT INTO public.room_statistics
        (stat_date, stat_temperature, stat_humidity, stat_pressure)
        VALUES (NOW(), :temperature, :humidity, :pressure)
    ');

    $stmt->execute([
        ':temperature' => (float)$temperature,
        ':humidity' => (float)$humidity,
        ':pressure' => (float)$pressure,
    ]);

    echo json_encode(['success' => true]);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode([
        'success' => false,
        'error' => $e->getMessage()
    ], JSON_UNESCAPED_UNICODE);
}
?>