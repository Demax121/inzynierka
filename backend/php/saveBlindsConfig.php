<?php
// Set proper headers first
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

// Enable error reporting for debugging (remove in production)
error_reporting(E_ALL);
ini_set('display_errors', 1);

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    exit(0);
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['success' => false, 'msg' => 'Method not allowed']);
    exit;
}

try {
    // Get JSON input
    $input = file_get_contents('php://input');
    $data = json_decode($input, true);
    
    if (json_last_error() !== JSON_ERROR_NONE) {
        throw new Exception('Invalid JSON data: ' . json_last_error_msg());
    }
    
    // For Docker containers, use HTTP request to CDN container
    // or use shared volume path if containers share volumes
    
    $jsonString = json_encode($data, JSON_PRETTY_PRINT);
    
    // Option 1: HTTP request to CDN container (recommended for Docker)
    $cdnUrl = 'http://caddy-cdn_inzynierka:8885/config_files/blinds_config.json';
    
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $cdnUrl);
    curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_setopt($ch, CURLOPT_POSTFIELDS, $jsonString);
    curl_setopt($ch, CURLOPT_HTTPHEADER, [
        'Content-Type: application/json',
        'Content-Length: ' . strlen($jsonString)
    ]);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    
    $result = curl_exec($ch);
    $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
    curl_close($ch);
    
    if ($httpCode >= 200 && $httpCode < 300) {
        echo json_encode([
            'success' => true,
            'msg' => 'Configuration saved successfully',
            'data' => $data,
            'method' => 'HTTP to CDN container'
        ]);
    } else {
        // Fallback: Try shared volume approach
        $sharedVolumePaths = [
            '/shared/config_files/blinds_config.json',
            '/app/shared/config_files/blinds_config.json',
            '/data/config_files/blinds_config.json'
        ];
        
        $saved = false;
        foreach ($sharedVolumePaths as $path) {
            $dir = dirname($path);
            if (is_dir($dir) || mkdir($dir, 0777, true)) {
                if (file_put_contents($path, $jsonString) !== false) {
                    $saved = true;
                    echo json_encode([
                        'success' => true,
                        'msg' => 'Configuration saved successfully',
                        'data' => $data,
                        'file_path' => $path,
                        'method' => 'Shared volume'
                    ]);
                    break;
                }
            }
        }
        
        if (!$saved) {
            throw new Exception('Failed to save config - HTTP code: ' . $httpCode . ', cURL result: ' . $result);
        }
    }
    
} catch (Exception $e) {
    http_response_code(500);
    echo json_encode([
        'success' => false,
        'msg' => 'Error saving configuration: ' . $e->getMessage(),
        'debug' => [
            'php_version' => PHP_VERSION,
            'current_dir' => getcwd(),
            'input_data' => $input ?? 'No input received'
        ]
    ]);
}
?>
