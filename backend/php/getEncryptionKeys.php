<?php
header('Content-Type: application/json; charset=utf-8');

/*
 * getEncryptionKeys.php
 * Purpose:
 *   Exposes the list of device API keys together with their AES encryption keys.
 *   (Intended for the Bun WS server to refresh its in‑memory map.)
 *
 * Method:
 *   GET (OPTIONS allowed for preflight). Any non‑GET (except OPTIONS) → 405.
 *
 * Response (200):
 *   {
 *     "data": [
 *       { "device_api_key": "<string>", "device_encryption_key": "<16-char key>" },
 *       ...
 *     ]
 *   }
 *
 * Errors:
 *   405: { "success": false, "error": "Method not allowed" }
 *   500: { "success": false, "error": "<database error message>" }
 *
 * Notes / Security:
 *   - This endpoint returns sensitive symmetric keys. Keep it internal (do NOT expose publicly).
 *   - Rely on network isolation / Caddy access rules; no additional headers added here per project policy.
 *   - Keys are stored as plain text (exactly 16 chars) for AES‑128 usage on devices.
 */

// Preflight (acknowledge CORS negotiation; Caddy supplies CORS headers globally).
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

// Enforce GET only.
if ($_SERVER['REQUEST_METHOD'] !== 'GET') {
    http_response_code(405);
    exit(json_encode(['success' => false, 'error' => 'Method not allowed'], JSON_UNESCAPED_UNICODE));
}

require_once 'config.php';

try {
    // Open PDO connection (exception mode).
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER,
        DB_PASS,
        [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
    );

    // Fetch all device keys (used by WS server for encryption map).
    $stmt = $pdo->prepare('SELECT device_api_key, device_encryption_key FROM devices');
    $stmt->execute();

    $results = $stmt->fetchAll(PDO::FETCH_ASSOC);

    // Successful response.
    echo json_encode([
        'data' => $results
    ], JSON_UNESCAPED_UNICODE);

} catch (PDOException $e) {
    // Database error path.
    http_response_code(500);
    echo json_encode([
        'success' => false,
        'error'   => $e->getMessage()
    ], JSON_UNESCAPED_UNICODE);
}
?>