<?php
header('Content-Type: application/json; charset=utf-8');

/*
 * setBlindsAutomate.php
 *
 * Purpose:
 *   Toggle ONLY the 'automate' flag in the singleton blinds_config row.
 *   (Does not alter min_lux / max_lux — those are managed via saveBlindsConfig.php.)
 *
 * Method:
 *   POST (application/json body)
 *
 * Request JSON:
 *   { "automate": true }   // or false
 *   - Missing or falsy value (except literal true) is treated as false.
 *
 * Successful Response (HTTP 200):
 *   { "success": true, "automate": true | false }
 *
 * Failure Response (HTTP 200):
 *   { "success": false, "error": "<message>" }
 *
 * Notes:
 *   - No validation beyond boolean coercion (same pattern as saveBlindsConfig.php).
 *   - UPDATE has no WHERE clause because table is designed to hold at most one row.
 *   - Does not add extra headers (Caddyfile handles CORS / security globally).
 */

require_once 'config.php';

// Decode JSON body (null on invalid JSON → treated as empty array).
$data = json_decode(file_get_contents('php://input'), true);

// Coerce 'automate' to PostgreSQL textual boolean ('true' / 'false').
// Wymuś typ boolean i zamień na 'true'/'false' string dla Postgresa
$automate = (isset($data['automate']) && $data['automate']) ? 'true' : 'false';

try {
    // Open PDO connection (exception mode for clearer failures).
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER,
        DB_PASS
    );
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    // Update only the automate column (singleton row).
    $stmt = $pdo->prepare('UPDATE public.blinds_config SET automate = :automate');
    $stmt->execute([':automate' => $automate]);

    echo json_encode([
        'success'  => true,
        'automate' => $automate === 'true'
    ]);
} catch (Exception $e) {
    echo json_encode([
        'success' => false,
        'error'   => $e->getMessage()
    ]);
}
?>