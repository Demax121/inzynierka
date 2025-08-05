<?php

$esp_url = "http://192.168.1.162/sensor";
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
    echo json_encode(["error" => "Brak odpowiedzi od ESP room sensor"]);
    exit;
}

header('Content-Type: application/json');
echo $response;
?>
