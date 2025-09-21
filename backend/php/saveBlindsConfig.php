<?php
header('Content-Type: application/json; charset=utf-8');

$data = json_decode(file_get_contents('php://input'), true);

$minLux = (int)($data['minLux'] ?? 0);
$maxLux = (int)($data['maxLux'] ?? 0);
// Convert the automate value to string 'true' or 'false' for database storage
$automate = isset($data['automate']) ? (bool)$data['automate'] : false;


try {
    $pdo = new PDO("pgsql:host=postgres;port=5432;dbname=inzynierka","postgresAdmin","postgres123");

    $stmt = $pdo->prepare('UPDATE public.blinds_config SET min_lux = :min, max_lux = :max, automate = :automate');
    $stmt->execute([':min' => $minLux, ':max' => $maxLux, ':automate' => $automate]);

    if ($stmt->rowCount() == 0) {
        $stmt = $pdo->prepare('INSERT INTO public.blinds_config (min_lux, max_lux, automate) VALUES (:min, :max, :automate)');
        $stmt->execute([':min' => $minLux, ':max' => $maxLux, ':automate' => $automate]);
    }

    echo json_encode([
        'success' => true,
        'minLux' => $minLux,
        'maxLux' => $maxLux,
        'automate' => $data['automate']
    ]);
} catch (Exception $e) {
    echo json_encode(['success'=>false,'error'=>$e->getMessage()]);
}
