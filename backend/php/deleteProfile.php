<?php
header('Content-Type: application/json; charset=utf-8');

/*
 * deleteProfile.php
 * Deletes a profile row from the `profiles` table.
 *
 * Method: POST
 * Request JSON:
 *   { "profile_id": <int|string> }
 *
 * Responses:
 *   200 OK:
 *     { "success": true, "message": "Profile deleted successfully" }
 *   400 Bad Request (missing/empty id):
 *     { "success": false, "error": "Profile ID is required" }
 *   404 Not Found (no row with that id):
 *     { "success": false, "error": "No profile found with the provided ID" }
 *   405 Method Not Allowed (not POST):
 *     { "success": false, "error": "Only POST method is allowed" }
 *   500 Internal Server Error (DB/other):
 *     { "success": false, "error": "Database error: ..." } or generic error
 *
 * Notes:
 * - CORS / other headers handled by Caddy (do not add here).
 * - No existence pre-check; relies on DELETE rowCount to detect missing ID.
 * - profile_id is not type-cast here; PostgreSQL will validate type implicitly.
 */

// Handle preflight OPTIONS (exit early; Caddy supplies headers)
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    exit(0);
}

// Enforce POST only
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['success' => false, 'error' => 'Only POST method is allowed']);
    exit();
}

// Read raw body and decode JSON
$jsonData = file_get_contents('php://input');
$data = json_decode($jsonData, true);

// Validate presence of profile_id
if (!isset($data['profile_id']) || empty($data['profile_id'])) {
    http_response_code(400);
    echo json_encode(['success' => false, 'error' => 'Profile ID is required']);
    exit();
}

$profile_id = $data['profile_id'];

require_once 'config.php';

try {
    // Open DB connection (exception mode)
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER,
        DB_PASS
    );
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    
    // Prepare parameterized DELETE (avoids injection)
    $stmt = $pdo->prepare("DELETE FROM profiles WHERE profile_id = :profile_id");
    
    // Execute with bound parameter
    $stmt->execute(['profile_id' => $profile_id]);
    
    // Check affected rows to determine existence
    if ($stmt->rowCount() > 0) {
        echo json_encode(['success' => true, 'message' => 'Profile deleted successfully']);
    } else {
        http_response_code(404);
        echo json_encode(['success' => false, 'error' => 'No profile found with the provided ID']);
    }
} catch (PDOException $e) {
    // Database-level failure
    http_response_code(500);
    echo json_encode(['success' => false, 'error' => 'Database error: ' . $e->getMessage()]);
} catch (Exception $e) {
    // Any other unexpected failure
    http_response_code(500);
    echo json_encode(['success' => false, 'error' => 'Error: ' . $e->getMessage()]);
}
?>