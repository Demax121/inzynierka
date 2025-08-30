<?php

$esp_url = "http://192.168.1.4/status";  // ESP32 main lights status endpoint (should match ESP32 IP)
$options = [
    "http" => [
        "header" => "Accept: application/json\r\n",
        "timeout" => 5  // Add timeout for better error handling
    ]
];
$context = stream_context_create($options);
$response = file_get_contents($esp_url, false, $context);

if ($response === FALSE) {
    http_response_code(500);
    echo json_encode(["error" => "Brak odpowiedzi od ESP main lights sensor"]);
    exit;
}

header('Content-Type: application/json');
echo $response;
?>
