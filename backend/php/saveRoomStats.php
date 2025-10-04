<?php
header('Content-Type: application/json; charset=utf-8');

/*
 * saveRoomStats.php
 *
 * Purpose:
 *   Persist a single environmental reading (temperature, humidity, pressure)
 *   into the room_statistics table with a server-side timestamp (NOW()).
 *
 * Method:
 *   POST (OPTIONS allowed for CORS preflight). Any non‑POST (except OPTIONS) is not explicitly rejected,
 *   but callers should use POST with a JSON body.
 *
 * Expected Request JSON:
 *   {
 *     "temperature": <number>,
 *     "humidity": <number>,
 *     "pressure": <number>
 *   }
 *
 * Validation:
 *   - Body must be valid JSON (object).
 *   - All three keys must be present and numeric (is_numeric).
 *
 * Responses:
 *   200: { "success": true }
 *   204: (preflight OPTIONS, no body)
 *   400: { "success": false, "error": "Invalid JSON" | "temperature/humidity/pressure must be numeric" }
 *   500: { "success": false, "error": "<database error message>" }
 *
 * Notes:
 *   - Values are cast to float before insertion to normalize numeric types.
 *   - stat_date uses NOW() so client clock skew does not matter.
 *   - No additional headers added (Caddyfile manages CORS / security).
 *   - If high write frequency becomes an issue, consider batching or rate limiting upstream.
 */

// Handle preflight (no additional headers here; Caddy supplies them).
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

require_once 'config.php';

// Read raw body and decode JSON.
$raw  = file_get_contents('php://input');
$data = json_decode($raw, true);

// Must be a JSON object.
if (!is_array($data)) {
    http_response_code(400);
    echo json_encode(['success' => false, 'error' => 'Invalid JSON']);
    exit;
}

// Extract fields (allow null for early validation).
$temperature = $data['temperature'] ?? null;
$humidity    = $data['humidity'] ?? null;
$pressure    = $data['pressure'] ?? null;

// All three must be numeric.
if (!is_numeric($temperature) || !is_numeric($humidity) || !is_numeric($pressure)) {
    http_response_code(400);
    echo json_encode(['success' => false, 'error' => 'temperature/humidity/pressure must be numeric']);
    exit;
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

    // Insert a single row (stat_date auto set to NOW()).
    $stmt = $pdo->prepare('
        INSERT INTO public.room_statistics
        (stat_date, stat_temperature, stat_humidity, stat_pressure)
        VALUES (NOW(), :temperature, :humidity, :pressure)
    ');

    $stmt->execute([
        ':temperature' => (float)$temperature,
        ':humidity'    => (float)$humidity,
        ':pressure'    => (float)$pressure,
    ]);

    echo json_encode(['success' => true]);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(
        ['success' => false, 'error' => $e->getMessage()],
        JSON_UNESCAPED_UNICODE
    );
}
?>