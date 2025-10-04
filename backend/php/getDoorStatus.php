<?php
header('Content-Type: application/json; charset=utf-8');

/*
 * getDoorStatus.php
 * Purpose:
 *   Returns the full door status history (newest first) from the door_status table.
 *
 * Method:
 *   GET (OPTIONS handled for preflight). Any non‑GET (except OPTIONS) returns 405.
 *
 * Response (200):
 *   {
 *     "success": true,
 *     "data": [ { "status_date": "...", "status_name": "Door open" | "Door closed" }, ... ],
 *     "count": <number_of_rows>
 *   }
 *
 * Errors:
 *   405: { "success": false, "error": "Method not allowed" }
 *   500: { "success": false, "error": "<database error message>" }
 *
 * Notes:
 *   - No additional headers (Caddyfile provides global CORS / cache headers).
 *   - Returns all rows (no pagination); frontend may slice if needed.
 *   - status_date ordering: DESC so most recent is first in array.
 */

// Handle CORS preflight quickly (Caddy supplies necessary headers).
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204); // No Content (ack preflight)
    exit;
}

// Enforce GET only.
if ($_SERVER['REQUEST_METHOD'] !== 'GET') {
    http_response_code(405); // Method Not Allowed
    exit(json_encode(['success' => false, 'error' => 'Method not allowed'], JSON_UNESCAPED_UNICODE));
}

require_once 'config.php';

try {
    // Open a PDO connection (exception mode for error clarity).
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER,
        DB_PASS,
        [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
    );

    // Retrieve full history (ordered newest first).
    $stmt = $pdo->prepare('SELECT * FROM door_status ORDER BY status_date DESC');
    $stmt->execute();

    $results = $stmt->fetchAll(PDO::FETCH_ASSOC);

    // Successful JSON response with count for quick client-side validation.
    echo json_encode([
        'success' => true,
        'data'    => $results,
        'count'   => count($results)
    ], JSON_UNESCAPED_UNICODE);

} catch (PDOException $e) {
    // Database failure path.
    http_response_code(500);
    echo json_encode([
        'success' => false,
        'error'   => $e->getMessage()
    ], JSON_UNESCAPED_UNICODE);
}
?>