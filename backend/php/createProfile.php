<?php
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST');
header('Access-Control-Allow-Headers: Content-Type');

// For preflight OPTIONS request
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

// Only allow POST requests
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['success' => false, 'message' => 'Method not allowed']);
    exit();
}

// Read the request body
$requestBody = file_get_contents('php://input');
$profile = json_decode($requestBody, true);

// Validate profile data
if (!$profile || !isset($profile['name']) || empty($profile['name'])) {
    http_response_code(400);
    echo json_encode(['success' => false, 'message' => 'Invalid profile data']);
    exit();
}

try {
    // Database connection parameters
    $host = 'postgres';
    $dbname = 'inzynierka';
    $user = 'postgresAdmin';
    $password = 'postgres123'; // Replace with your actual password
    
    // Create connection string
    $dsn = "pgsql:host=$host;dbname=$dbname";
    
    // Create PDO instance
    $pdo = new PDO($dsn, $user, $password);
    
    // Set the PDO error mode to exception
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    
    // Prepare the SQL statement to insert the profile
    $stmt = $pdo->prepare("
        INSERT INTO profiles (profile_name, profile_json)
        VALUES (:profile_name, :profile_json)
        RETURNING profile_id
    ");
    
    // Execute the statement with the profile data
    $stmt->execute([
        ':profile_name' => $profile['name'],
        ':profile_json' => json_encode($profile)
    ]);
    
    // Get the newly created profile ID
    $profileId = $stmt->fetchColumn();
    
    // Return success response
    echo json_encode([
        'success' => true, 
        'message' => 'Profile saved successfully',
        'profile_id' => $profileId
    ]);
    
} catch (PDOException $e) {
    // Handle database errors
    http_response_code(500);
    echo json_encode([
        'success' => false, 
        'message' => 'Database error: ' . $e->getMessage()
    ]);
} catch (Exception $e) {
    // Handle other errors
    http_response_code(500);
    echo json_encode([
        'success' => false, 
        'message' => 'Server error: ' . $e->getMessage()
    ]);
}
?>