<?php
header('Content-Type: application/json; charset=utf-8');

$data = json_decode(file_get_contents('php://input'), true);

$minLux = (int)($data['minLux'] ?? 0);
$maxLux = (int)($data['maxLux'] ?? 0);

try {
    $pdo = new PDO("pgsql:host=postgres;port=5432;dbname=inzynierka","postgresAdmin","postgres123");

    $stmt = $pdo->prepare('UPDATE public.blinds_config SET min_lux = :min, max_lux = :max');
    $stmt->execute([':min' => $minLux, ':max' => $maxLux]);

    if ($stmt->rowCount() == 0) {
        $stmt = $pdo->prepare('INSERT INTO public.blinds_config (min_lux, max_lux) VALUES (:min, :max)');
        $stmt->execute([':min' => $minLux, ':max' => $maxLux]);
    }

    echo json_encode(['success'=>true,'minLux'=>$minLux,'maxLux'=>$maxLux]);
} catch (Exception $e) {
    echo json_encode(['success'=>false,'error'=>$e->getMessage()]);
}
