<?php
header('Content-Type: application/json; charset=utf-8');

require_once 'config.php';

$data = json_decode(file_get_contents('php://input'), true);

$min_lux = (int)($data['min_lux'] ?? 0);
$max_lux = (int)($data['max_lux'] ?? 0);
// Wymuś typ boolean i zamień na 'true'/'false' string dla Postgresa
$automate = isset($data['automate']) && $data['automate'] ? 'true' : 'false';

try {
    $pdo = new PDO(
        "pgsql:host=" . DB_HOST . ";dbname=" . DB_NAME,
        DB_USER,
        DB_PASS
    );

    $stmt = $pdo->prepare('UPDATE public.blinds_config SET min_lux = :min, max_lux = :max, automate = :automate');
    $stmt->execute([':min' => $min_lux, ':max' => $max_lux, ':automate' => $automate]);

    if ($stmt->rowCount() == 0) {
        $stmt = $pdo->prepare('INSERT INTO public.blinds_config (min_lux, max_lux, automate) VALUES (:min, :max, :automate)');
        $stmt->execute([':min' => $min_lux, ':max' => $max_lux, ':automate' => $automate]);
    }

    echo json_encode([
        'success' => true,
        'min_lux' => $min_lux,
        'max_lux' => $max_lux,
        'automate' => $automate === 'true'
    ]);
} catch (Exception $e) {
    echo json_encode(['success'=>false,'error'=>$e->getMessage()]);
}
?>