<?php
header('Content-Type: application/json; charset=utf-8');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

// Only allow GET for this endpoint
if ($_SERVER['REQUEST_METHOD'] !== 'GET') {
    http_response_code(405);
    header('Allow: GET, OPTIONS');
    echo json_encode(['success' => false, 'error' => 'Method not allowed, use GET']);
    exit;
}

require_once 'config.php';

// Optional ?limit parameter to limit number of rows returned (default 100)
$limit = 100;
if (isset($_GET['limit'])) {
    $l = filter_var($_GET['limit'], FILTER_VALIDATE_INT, ['options' => ['min_range' => 1]]);
    if ($l === false) {
        http_response_code(400);
        echo json_encode(['success' => false, 'error' => 'limit must be a positive integer']);
        exit;
    }
    $limit = $l;
}

try {
    // Open PDO connection (exception + associative fetch mode).
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER,
        DB_PASS,
        [
            PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        ]
    );

    // Fetch the most recent stats ordered by stat_date DESC limited by ?limit
    $stmt = $pdo->prepare('SELECT stat_date, stat_temperature, stat_humidity, stat_pressure FROM public.room_statistics ORDER BY stat_date DESC LIMIT :limit');
    // PDO requires integer binding via bindValue with explicit type
    $stmt->bindValue(':limit', (int)$limit, PDO::PARAM_INT);
    $stmt->execute();
    $rows = $stmt->fetchAll();

    echo json_encode(['success' => true, 'data' => $rows], JSON_UNESCAPED_UNICODE);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(
        ['success' => false, 'error' => $e->getMessage()],
        JSON_UNESCAPED_UNICODE
    );
}
?>