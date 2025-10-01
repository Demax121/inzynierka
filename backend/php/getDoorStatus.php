<?php
header('Content-Type: application/json; charset=utf-8');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'GET') {
    http_response_code(405);
    exit(json_encode(['success' => false, 'error' => 'Method not allowed'], JSON_UNESCAPED_UNICODE));
}

require_once 'config.php';

try {
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER, DB_PASS,
        [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
    );

    $stmt = $pdo->prepare('SELECT * FROM door_status ORDER BY status_date DESC');
    $stmt->execute();
    
    $results = $stmt->fetchAll(PDO::FETCH_ASSOC);

    echo json_encode([
        'success' => true,
        'data' => $results,
        'count' => count($results)
    ], JSON_UNESCAPED_UNICODE);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode([
        'success' => false, 
        'error' => $e->getMessage()
    ], JSON_UNESCAPED_UNICODE);
}
?>