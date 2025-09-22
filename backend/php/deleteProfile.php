<?php
header('Content-Type: application/json; charset=utf-8');

// Handle preflight OPTIONS request
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    exit(0);
}

// Only accept POST requests for this endpoint
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405); // Method Not Allowed
    echo json_encode(['success' => false, 'error' => 'Only POST method is allowed']);
    exit();
}

// Get JSON data from request body
$jsonData = file_get_contents('php://input');
$data = json_decode($jsonData, true);

// Check if profile_id is provided
if (!isset($data['profile_id']) || empty($data['profile_id'])) {
    http_response_code(400); // Bad Request
    echo json_encode(['success' => false, 'error' => 'Profile ID is required']);
    exit();
}

$profile_id = $data['profile_id'];

// Dołącz konfigurację bazy
require_once 'config.php';

try {
    // Connect to PostgreSQL database
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER,
        DB_PASS
    );
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    
    // Prepare DELETE query
    $stmt = $pdo->prepare("DELETE FROM profiles WHERE profile_id = :profile_id");
    
    // Execute DELETE query with parameters
    $stmt->execute(['profile_id' => $profile_id]);
    
    // Check if any rows were affected
    if ($stmt->rowCount() > 0) {
        // Success - Profile was deleted
        echo json_encode(['success' => true, 'message' => 'Profile deleted successfully']);
    } else {
        // No profile found with that ID
        http_response_code(404); // Not Found
        echo json_encode(['success' => false, 'error' => 'No profile found with the provided ID']);
    }
} catch (PDOException $e) {
    // Database error
    http_response_code(500); // Internal Server Error
    echo json_encode(['success' => false, 'error' => 'Database error: ' . $e->getMessage()]);
} catch (Exception $e) {
    // Other errors
    http_response_code(500); // Internal Server Error
    echo json_encode(['success' => false, 'error' => 'Error: ' . $e->getMessage()]);
}
?>