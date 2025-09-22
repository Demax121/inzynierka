<?php
header('Content-Type: application/json; charset=utf-8');

// Handle preflight requests
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

// Only allow GET requests
if ($_SERVER['REQUEST_METHOD'] !== 'GET') {
    http_response_code(405); // Method Not Allowed
    echo json_encode(['success' => false, 'error' => 'Only GET requests are allowed']);
    exit();
}

// Dodaj konfigurację i połączenie z bazą
require_once 'config.php';

$pdo = new PDO(
    "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
    DB_USER,
    DB_PASS
);
$pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

// Get profiles function
function getProfiles($pdo) {
    try {
        // Query to get all profiles with their names, IDs and JSON data
        $stmt = $pdo->prepare("SELECT profile_id, profile_name, profile_json FROM profiles ORDER BY profile_name ASC");
        $stmt->execute();

        // Get all results
        $profiles = $stmt->fetchAll(PDO::FETCH_ASSOC);

        // Parse JSON strings to objects
        foreach ($profiles as &$profile) {
            // Decode JSON string to PHP array
            $profile['profile_json'] = json_decode($profile['profile_json'], true);
        }

        // Return profiles array
        return [
            'success' => true,
            'profiles' => $profiles
        ];

    } catch (Exception $e) {
        // Return error response
        return [
            'success' => false, 
            'error' => $e->getMessage()
        ];
    }
}

// Get single profile by ID
function getProfileById($pdo, $profileId) {
    try {
        // Query to get specific profile
        $stmt = $pdo->prepare("SELECT profile_id, profile_name, profile_json FROM profiles WHERE profile_id = :profile_id");
        $stmt->bindParam(':profile_id', $profileId, PDO::PARAM_INT);
        $stmt->execute();

        // Get profile
        $profile = $stmt->fetch(PDO::FETCH_ASSOC);

        if ($profile) {
            // Parse JSON string to object
            $profile['profile_json'] = json_decode($profile['profile_json'], true);

            return [
                'success' => true,
                'profile' => $profile
            ];
        } else {
            return [
                'success' => false,
                'error' => 'Profile not found'
            ];
        }

    } catch (Exception $e) {
        // Return error response
        return [
            'success' => false, 
            'error' => $e->getMessage()
        ];
    }
}

// Get single profile by name
function getProfileByName($pdo, $profileName) {
    try {
        // Query to get specific profile by name
        $stmt = $pdo->prepare("SELECT profile_id, profile_name, profile_json FROM profiles WHERE profile_name = :profile_name");
        $stmt->bindParam(':profile_name', $profileName, PDO::PARAM_STR);
        $stmt->execute();

        // Get profile
        $profile = $stmt->fetch(PDO::FETCH_ASSOC);

        if ($profile) {
            // Parse JSON string to object
            $profile['profile_json'] = json_decode($profile['profile_json'], true);

            return [
                'success' => true,
                'profile' => $profile
            ];
        } else {
            return [
                'success' => false,
                'error' => 'Profile not found'
            ];
        }

    } catch (Exception $e) {
        // Return error response
        return [
            'success' => false, 
            'error' => $e->getMessage()
        ];
    }
}

// Check if a specific profile was requested
if (isset($_GET['id'])) {
    // Get by ID
    $profileId = intval($_GET['id']);
    $result = getProfileById($pdo, $profileId);
} else if (isset($_GET['name'])) {
    // Get by name
    $profileName = $_GET['name'];
    $result = getProfileByName($pdo, $profileName);
} else {
    // Otherwise return all profiles
    $result = getProfiles($pdo);
}

// Output the JSON result
echo json_encode($result);
?>