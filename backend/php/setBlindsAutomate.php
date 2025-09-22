<?php
header('Content-Type: application/json; charset=utf-8');

require_once 'config.php';

$data = json_decode(file_get_contents('php://input'), true);

// Wymuś typ boolean i zamień na 'true'/'false' string dla Postgresa
$automate = isset($data['automate']) && $data['automate'] ? 'true' : 'false';

try {
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER,
        DB_PASS
    );

    // Aktualizuj tylko pole automate
    $stmt = $pdo->prepare('UPDATE public.blinds_config SET automate = :automate');
    $stmt->execute([':automate' => $automate]);

    echo json_encode([
        'success' => true,
        'automate' => $automate === 'true'
    ]);
} catch (Exception $e) {
    echo json_encode(['success'=>false,'error'=>$e->getMessage()]);
}
?>