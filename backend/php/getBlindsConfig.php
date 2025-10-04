<?php
header('Content-Type: application/json; charset=utf-8');

/*
 * getBlindsConfig.php
 * Returns the (single) blinds automation configuration row.
 * Table: public.blinds_config
 * Columns used: min_lux (int), max_lux (int), automate (bool)
 *
 * Notes:
 * - Only one row is expected; we LIMIT 1.
 * - Type casting is applied to ensure stable JSON types for the frontend.
 * - No extra CORS / cache headers here (Caddyfile handles those globally).
 */

require_once 'config.php';

try {
    // Create PDO connection (exception mode + assoc fetch)
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER,
        DB_PASS,
        [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        ]
    );

    // Fetch only the fields we need (single config row)
    $stmt = $pdo->query('SELECT min_lux, max_lux, automate FROM public.blinds_config LIMIT 1');
    $row = $stmt->fetch();

    // Build a safe response with explicit type casting
    echo json_encode([
        'min_lux'  => (int)($row['min_lux'] ?? 0),
        'max_lux'  => (int)($row['max_lux'] ?? 0),
        'automate' => (bool)($row['automate'] ?? false),
    ], JSON_UNESCAPED_UNICODE);

} catch (PDOException $e) {
    // Generic 500 on DB error; message included for debugging (internal use)
    http_response_code(500);
    echo json_encode([
        'error' => $e->getMessage()
    ], JSON_UNESCAPED_UNICODE);
}
?>