<?php
header('Content-Type: application/json; charset=utf-8');

/*
 * saveDoorStatus.php
 *
 * Purpose:
 *   Persist a single door status event into the door_status table.
 *   Each call inserts a new row with current timestamp (NOW()) and a validated status_name.
 *
 * Method:
 *   POST (OPTIONS allowed for CORS preflight). Script does not explicitly reject other verbs,
 *   but callers should use POST with a JSON body.
 *
 * Expected Request JSON:
 *   { "status": "Door open" }  OR  { "status": "Door closed" }
 *
 * Validation Rules:
 *   - Body must be valid JSON object.
 *   - "status" must exist, be a string, and equal exactly "Door open" or "Door closed".
 *
 * Responses:
 *   200: { "success": true }
 *   400: { "success": false, "error": "Invalid JSON" | "Invalid status value" }
 *   500: { "success": false, "error": "<database error message>" }
 *
 * Notes:
 *   - Timestamp is generated server-side via NOW().
 *   - No updates/deletes; this is an append-only history log.
 *   - CORS / other headers are managed by Caddy globally (do not add them here).
 *   - If stricter method enforcement is desired, add a POST check before decoding.
 */

// Handle CORS preflight quickly (no body needed).
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

require_once 'config.php';

// Decode raw JSON; on failure json_decode returns null (not an array).
$data = json_decode(file_get_contents('php://input'), true);
if (!is_array($data)) {
    http_response_code(400);
    exit(json_encode(['success' => false, 'error' => 'Invalid JSON']));
}

// Validate and normalize the status field.
$status = null;
if (isset($data['status']) && is_string($data['status'])) {
    $s = trim($data['status']);
    if ($s === 'Door open' || $s === 'Door closed') {
        $status = $s;
    }
}

// Reject if status not one of the allowed literals.
if (!$status) {
    http_response_code(400);
    exit(json_encode(['success' => false, 'error' => 'Invalid status value']));
}

try {
    // Open PDO connection with exception mode for clearer error handling.
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER, DB_PASS,
        [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
    );

    // Insert new event row (timestamp + status).
    $pdo->prepare('INSERT INTO door_status (status_date, status_name) VALUES (NOW(), :status)')
        ->execute([':status' => $status]);

    // Success response.
    echo json_encode(['success' => true]);
} catch (PDOException $e) {
    // Database failure response.
    http_response_code(500);
    echo json_encode(['success' => false, 'error' => $e->getMessage()], JSON_UNESCAPED_UNICODE);
}
?>