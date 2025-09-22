<?php
header('Content-Type: application/json; charset=utf-8');

// Handle preflight requests
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

// Only allow POST requests
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405); // Method Not Allowed
    echo json_encode(['success' => false, 'error' => 'Only POST requests are allowed']);
    exit();
}

// Get the POST data
$data = json_decode(file_get_contents('php://input'), true);

// Validate required fields
if (!isset($data['profile_name']) || empty(trim($data['profile_name'])) || !isset($data['profile_json'])) {
    http_response_code(400); // Bad Request
    echo json_encode(['success' => false, 'error' => 'Missing required fields: profile_name or profile_json']);
    exit();
}

// Sanitize inputs
$profileName = trim($data['profile_name']);
$profileJson = $data['profile_json'];

// Dołącz konfigurację bazy
require_once 'config.php';

try {
    // Connect to PostgreSQL
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER,
        DB_PASS
    );
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    
    // Prepare and execute INSERT query
    $stmt = $pdo->prepare("INSERT INTO profiles (profile_name, profile_json) VALUES (:profile_name, :profile_json) RETURNING profile_id");
    
    // Convert profile_json to JSON string
    $profileJsonStr = json_encode($profileJson);
    
    // Bind parameters
    $stmt->bindParam(':profile_name', $profileName);
    $stmt->bindParam(':profile_json', $profileJsonStr);
    
    // Execute the statement
    $stmt->execute();
    
    // Get the inserted profile ID
    $profileId = $stmt->fetchColumn();
    
    // Return success response
    echo json_encode([
        'success' => true, 
        'message' => 'Profile saved successfully', 
        'profile_id' => $profileId
    ]);
    
} catch (Exception $e) {
    // Return error response
    http_response_code(500); // Internal Server Error
    echo json_encode(['success' => false, 'error' => $e->getMessage()]);
}
?>