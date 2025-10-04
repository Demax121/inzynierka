<?php
header('Content-Type: application/json; charset=utf-8');

/*
 * saveBlindsConfig.php
 * Hardened:
 *   - Strict numeric validation & range checks
 *   - Logical constraint: min_lux < max_lux
 *   - Explicit integer param binding
 */

require_once 'config.php';

$data = json_decode(file_get_contents('php://input'), true);
if (!is_array($data)) {
    echo json_encode(['success'=>false,'error'=>'Invalid JSON body']);
    exit();
}

$min_lux = $data['min_lux'] ?? null;
$max_lux = $data['max_lux'] ?? null;
$automateBool = isset($data['automate']) && $data['automate'];

if (!is_numeric($min_lux) || !is_numeric($max_lux)) {
    echo json_encode(['success'=>false,'error'=>'min_lux and max_lux must be numeric']);
    exit();
}

$min_lux = (int)$min_lux;
$max_lux = (int)$max_lux;

// Reasonable boundaries safeguard (avoid absurd / overflow values)
if ($min_lux < 0 || $max_lux < 0 || $min_lux > 1000000 || $max_lux > 1000000) {
    echo json_encode(['success'=>false,'error'=>'lux values out of allowed range (0-1,000,000)']);
    exit();
}
if ($min_lux >= $max_lux) {
    echo json_encode(['success'=>false,'error'=>'min_lux must be less than max_lux']);
    exit();
}

$automate = $automateBool ? 'true' : 'false';

try {
    $pdo = new PDO(
        "pgsql:host=".DB_HOST.";dbname=".DB_NAME,
        DB_USER,
        DB_PASS
    );
    $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

    $stmt = $pdo->prepare(
        'UPDATE public.blinds_config
         SET min_lux = :min, max_lux = :max, automate = :automate'
    );
    $stmt->bindValue(':min', $min_lux, PDO::PARAM_INT);
    $stmt->bindValue(':max', $max_lux, PDO::PARAM_INT);
    $stmt->bindValue(':automate', $automate, PDO::PARAM_STR);
    $stmt->execute();

    if ($stmt->rowCount() === 0) {
        $ins = $pdo->prepare(
            'INSERT INTO public.blinds_config (min_lux, max_lux, automate)
             VALUES (:min, :max, :automate)'
        );
        $ins->bindValue(':min', $min_lux, PDO::PARAM_INT);
        $ins->bindValue(':max', $max_lux, PDO::PARAM_INT);
        $ins->bindValue(':automate', $automate, PDO::PARAM_STR);
        $ins->execute();
    }

    echo json_encode([
        'success'=>true,
        'min_lux'=>$min_lux,
        'max_lux'=>$max_lux,
        'automate'=>$automate === 'true'
    ]);
} catch (Exception $e) {
    echo json_encode(['success'=>false,'error'=>$e->getMessage()]);
}
?>