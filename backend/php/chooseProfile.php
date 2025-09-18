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

// Get profiles function
function getProfiles() {
    try {
        // Connect to PostgreSQL
        $pdo = new PDO("pgsql:host=postgres;port=5432;dbname=inzynierka","postgresAdmin","postgres123");
        $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        
        // Query to get all profiles with their names, IDs and JSON data
        $stmt = $pdo->prepare("SELECT profile_id, profile_name, profile_json FROM profiles ORDER BY profile_name ASC");
        $stmt->execute();
        
        // Get all results
        $profiles = $stmt->fetchAll(PDO::FETCH_ASSOC);
        
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
function getProfileById($profileId) {
    try {
        // Connect to PostgreSQL
        $pdo = new PDO("pgsql:host=postgres;port=5432;dbname=inzynierka","postgresAdmin","postgres123");
        $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        
        // Query to get specific profile
        $stmt = $pdo->prepare("SELECT profile_id, profile_name, profile_json FROM profiles WHERE profile_id = :profile_id");
        $stmt->bindParam(':profile_id', $profileId, PDO::PARAM_INT);
        $stmt->execute();
        
        // Get profile
        $profile = $stmt->fetch(PDO::FETCH_ASSOC);
        
        if ($profile) {
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

// Check if a specific profile ID was requested
if (isset($_GET['id'])) {
    $profileId = intval($_GET['id']);
    $result = getProfileById($profileId);
} else {
    // Otherwise return all profiles
    $result = getProfiles();
}

// Output the JSON result
echo json_encode($result);
?>