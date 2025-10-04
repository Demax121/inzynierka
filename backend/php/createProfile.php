<?php
header('Content-Type: application/json; charset=utf-8');

/*
 * createProfile.php
 * Creates a new profile in `profiles`.
 * Hardening added:
 *   - Whitelist + length limit on profile_name (regex)
 *   - Explicit size cap on profile_json payload
 *   - Explicit PDO param typing
 */

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['success'=>false,'error'=>'Only POST requests are allowed']);
    exit();
}

$data = json_decode(file_get_contents('php://input'), true);
if (!is_array($data)) {
    http_response_code(400);
    echo json_encode(['success'=>false,'error'=>'Invalid JSON body']);
    exit();
}

$profileName = isset($data['profile_name']) ? trim($data['profile_name']) : '';
$profileJson = $data['profile_json'] ?? null;

if ($profileName === '' || $profileJson === null) {
    http_response_code(400);
    echo json_encode(['success'=>false,'error'=>'Missing required fields: profile_name or profile_json']);
    exit();
}

// Security: enforce allowed chars & max length
const MAX_PROFILE_NAME_LEN = 64;
if (mb_strlen($profileName) > MAX_PROFILE_NAME_LEN) {
    http_response_code(400);
    echo json_encode(['success'=>false,'error'=>'profile_name too long (max 64 chars)']);
    exit();
}
if (!preg_match('/^[A-Za-z0-9 _\-]{1,64}$/u', $profileName)) {
    http_response_code(400);
    echo json_encode(['success'=>false,'error'=>'profile_name has invalid characters']);
    exit();
}

// Security: size cap for JSON (avoid very large blobs / DoS)
$profileJsonStr = json_encode($profileJson);
if ($profileJsonStr === false) {
    http_response_code(400);
    echo json_encode(['success'=>false,'error'=>'profile_json not encodable']);
    exit();
}
if (strlen($profileJsonStr) > 20000) { // ~20 KB limit (adjust as needed)
    http_response_code(400);
    echo json_encode(['success'=>false,'error'=>'profile_json too large']);
    exit();
}

require_once 'config.php';

try {
    $pdo = new PDO(
        "pgsql:host=".DB_HOST.";dbname=".DB_NAME,
        DB_USER,
        DB_PASS
    );
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    $stmt = $pdo->prepare(
        "INSERT INTO profiles (profile_name, profile_json)
         VALUES (:profile_name, :profile_json)
         RETURNING profile_id"
    );
    $stmt->bindValue(':profile_name', $profileName, PDO::PARAM_STR);
    $stmt->bindValue(':profile_json', $profileJsonStr, PDO::PARAM_STR);
    $stmt->execute();

    $profileId = (int)$stmt->fetchColumn();

    echo json_encode([
        'success'=>true,
        'message'=>'Profile saved successfully',
        'profile_id'=>$profileId
    ]);
} catch (Exception $e) {
    http_response_code(500);
    echo json_encode(['success'=>false,'error'=>$e->getMessage()]);
}
?>