<?php
header('Content-Type: application/json; charset=utf-8');

try {
    $pdo = new PDO(
        "pgsql:host=postgres;port=5432;dbname=inzynierka",
        "postgresAdmin",
        "postgres123",
        [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        ]
    );

    // pobieramy tylko potrzebne pola
    $stmt = $pdo->query('SELECT min_lux, max_lux FROM public.blinds_config LIMIT 1');
    $row = $stmt->fetch();

    echo json_encode([
        'min_lux' => (int)($row['min_lux'] ?? 0),
        'max_lux' => (int)($row['max_lux'] ?? 0)
    ], JSON_UNESCAPED_UNICODE);

} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode([
        'error' => $e->getMessage()
    ], JSON_UNESCAPED_UNICODE);
}
?>