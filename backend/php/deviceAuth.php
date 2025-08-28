<?php
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST');
header('Access-Control-Allow-Headers: Content-Type');

// Database connection
$host = getenv('POSTGRES_HOST') ?: 'postgres';
$port = '5432';
$dbname = getenv('POSTGRES_DB') ?: 'basePostgresDB';
$username = getenv('POSTGRES_USER') ?: 'postgresAdmin';
$password = getenv('POSTGRES_PASSWORD') ?: 'postgres123';

try {
    $pdo = new PDO("pgsql:host=$host;port=$port;dbname=$dbname", $username, $password);
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
} catch (PDOException $e) {
    http_response_code(500);
    echo json_encode(['error' => 'Database connection failed']);
    exit;
}

$method = $_SERVER['REQUEST_METHOD'];

if ($method === 'GET') {
    // Step 1: Generate temporary key
    if (isset($_GET['action']) && $_GET['action'] === 'request_temp_key') {
        $tempKey = bin2hex(random_bytes(32)); // 256-bit key
        
        // Store temporary key in database
        $stmt = $pdo->prepare("INSERT INTO temp_keys (temp_key) VALUES (?)");
        $stmt->execute([$tempKey]);
        
        echo json_encode([
            'status' => 'success',
            'temp_key' => $tempKey,
            'expires_in' => 600 // 10 minutes
        ]);
        exit;
    }
}

if ($method === 'POST') {
    $input = json_decode(file_get_contents('php://input'), true);
    
    if (!$input) {
        http_response_code(400);
        echo json_encode(['error' => 'Invalid JSON']);
        exit;
    }
    
    // Step 2: Device authentication with temporary key
    if (isset($input['action']) && $input['action'] === 'authenticate') {
        $tempKey = $input['temp_key'] ?? '';
        $encryptedData = $input['encrypted_data'] ?? '';
        
        // Verify temporary key exists and hasn't expired
        $stmt = $pdo->prepare("SELECT temp_key FROM temp_keys WHERE temp_key = ? AND expires_at > NOW()");
        $stmt->execute([$tempKey]);
        
        if (!$stmt->fetch()) {
            http_response_code(401);
            echo json_encode(['error' => 'Invalid or expired temporary key']);
            exit;
        }
        
        // Simple decryption simulation (replace with actual decryption)
        $decryptedData = simpleDecrypt($encryptedData, $tempKey);
        $deviceData = json_decode($decryptedData, true);
        
        if (!$deviceData || !isset($deviceData['device_uid']) || !isset($deviceData['mac_address'])) {
            http_response_code(400);
            echo json_encode(['error' => 'Invalid device data']);
            exit;
        }
        
        // Check if device exists in database
        $stmt = $pdo->prepare("SELECT device_uid, encryption_key FROM devices WHERE device_uid = ? AND mac_address = ?");
        $stmt->execute([$deviceData['device_uid'], $deviceData['mac_address']]);
        $device = $stmt->fetch(PDO::FETCH_ASSOC);
        
        if (!$device) {
            http_response_code(401);
            echo json_encode(['error' => 'Device not authorized']);
            exit;
        }
        
        // Update last seen
        $stmt = $pdo->prepare("UPDATE devices SET last_seen = NOW() WHERE device_uid = ?");
        $stmt->execute([$device['device_uid']]);
        
        // Remove temporary key
        $stmt = $pdo->prepare("DELETE FROM temp_keys WHERE temp_key = ?");
        $stmt->execute([$tempKey]);
        
        // Return permanent encryption key
        $response = json_encode([
            'status' => 'authenticated',
            'permanent_key' => $device['encryption_key']
        ]);
        
        // Encrypt response with temporary key
        $encryptedResponse = simpleEncrypt($response, $tempKey);
        
        echo json_encode([
            'status' => 'success',
            'encrypted_response' => $encryptedResponse
        ]);
        exit;
    }
}

// Simple encryption/decryption functions (replace with proper implementation)
function simpleEncrypt($data, $key) {
    return base64_encode($data . '|' . $key);
}

function simpleDecrypt($encryptedData, $key) {
    $decoded = base64_decode($encryptedData);
    $parts = explode('|', $decoded);
    if (count($parts) === 2 && $parts[1] === $key) {
        return $parts[0];
    }
    return false;
}

// Clean up expired temporary keys
$stmt = $pdo->prepare("DELETE FROM temp_keys WHERE expires_at < NOW()");
$stmt->execute();

http_response_code(404);
echo json_encode(['error' => 'Invalid request']);
?>
