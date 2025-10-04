<?php
header('Content-Type: application/json; charset=utf-8');

/*
 * chooseProfile.php
 *
 * Purpose:
 *   Retrieve profile data from the `profiles` table.
 *   Supports three modes:
 *     1. GET ?id=<profile_id>      → single profile by numeric ID
 *     2. GET ?name=<profile_name>  → single profile by unique name
 *     3. GET (no params)           → all profiles (ordered alphabetically by name)
 *
 * Method:
 *   GET (OPTIONS allowed for preflight). Any non‑GET (except OPTIONS) returns 405.
 *
 * Request Examples:
 *   GET /chooseProfile.php?id=3
 *   GET /chooseProfile.php?name=Evening
 *   GET /chooseProfile.php
 *
 * Successful Responses:
 *   Single profile:
 *     {
 *       "success": true,
 *       "profile": {
 *         "profile_id": 3,
 *         "profile_name": "Evening",
 *         "profile_json": { ...parsed JSON object... }
 *       }
 *     }
 *   All profiles:
 *     {
 *       "success": true,
 *       "profiles": [
 *         { "profile_id": 1, "profile_name": "Day",    "profile_json": { ... } },
 *         { "profile_id": 2, "profile_name": "Evening","profile_json": { ... } },
 *         ...
 *       ]
 *     }
 *
 * Error Responses:
 *   405 (wrong method):
 *     { "success": false, "error": "Only GET requests are allowed" }
 *   200 with not found (id or name not matching):
 *     { "success": false, "error": "Profile not found" }
 *   500 (unexpected DB error):
 *     { "success": false, "error": "<exception message>" }
 *
 * Notes:
 *   - `profile_json` is stored as a JSON string; it is decoded into a PHP array
 *     before returning so the frontend receives proper JSON objects.
 *   - No pagination for the list mode; caller can slice client‑side.
 *   - No additional headers (Caddy handles CORS / cache globally).
 */

// Handle preflight requests early (CORS negotiation). Caddy adds necessary headers.
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

// Enforce GET only.
if ($_SERVER['REQUEST_METHOD'] !== 'GET') {
    http_response_code(405); // Method Not Allowed
    echo json_encode(['success' => false, 'error' => 'Only GET requests are allowed']);
    exit();
}

// DB configuration
require_once 'config.php';

// Create PDO connection (exception mode for transparent error handling)
$pdo = new PDO(
    "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
    DB_USER,
    DB_PASS
);
$pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

/**
 * Fetch all profiles ordered by name.
 * Decodes each profile_json string into a PHP array.
 */
function getProfiles($pdo) {
    try {
        $stmt = $pdo->prepare("SELECT profile_id, profile_name, profile_json FROM profiles ORDER BY profile_name ASC");
        $stmt->execute();
        $profiles = $stmt->fetchAll(PDO::FETCH_ASSOC);

        foreach ($profiles as &$profile) {
            $profile['profile_json'] = json_decode($profile['profile_json'], true);
        }

        return [
            'success'  => true,
            'profiles' => $profiles
        ];
    } catch (Exception $e) {
        return [
            'success' => false,
            'error'   => $e->getMessage()
        ];
    }
}

/**
 * Fetch a single profile by numeric ID.
 */
function getProfileById($pdo, $profileId) {
    try {
        $stmt = $pdo->prepare("SELECT profile_id, profile_name, profile_json FROM profiles WHERE profile_id = :profile_id");
        $stmt->bindParam(':profile_id', $profileId, PDO::PARAM_INT);
        $stmt->execute();
        $profile = $stmt->fetch(PDO::FETCH_ASSOC);

        if ($profile) {
            $profile['profile_json'] = json_decode($profile['profile_json'], true);
            return [
                'success' => true,
                'profile' => $profile
            ];
        }
        return [
            'success' => false,
            'error'   => 'Profile not found'
        ];
    } catch (Exception $e) {
        return [
            'success' => false,
            'error'   => $e->getMessage()
        ];
    }
}

/**
 * Fetch a single profile by its unique name (case-sensitive match).
 */
function getProfileByName($pdo, $profileName) {
    try {
        $stmt = $pdo->prepare("SELECT profile_id, profile_name, profile_json FROM profiles WHERE profile_name = :profile_name");
        $stmt->bindParam(':profile_name', $profileName, PDO::PARAM_STR);
        $stmt->execute();
        $profile = $stmt->fetch(PDO::FETCH_ASSOC);

        if ($profile) {
            $profile['profile_json'] = json_decode($profile['profile_json'], true);
            return [
                'success' => true,
                'profile' => $profile
            ];
        }
        return [
            'success' => false,
            'error'   => 'Profile not found'
        ];
    } catch (Exception $e) {
        return [
            'success' => false,
            'error'   => $e->getMessage()
        ];
    }
}

// Dispatch based on query parameters priority: id > name > all
if (isset($_GET['id'])) {
    $profileId = intval($_GET['id']);
    $result = getProfileById($pdo, $profileId);
} elseif (isset($_GET['name'])) {
    $profileName = $_GET['name'];
    $result = getProfileByName($pdo, $profileName);
} else {
    $result = getProfiles($pdo);
}

// Emit JSON response (already structured)
echo json_encode($result);
?>