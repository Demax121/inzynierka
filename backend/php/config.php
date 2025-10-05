<?php
// Central configuration loaded from environment variables.
// This allows consolidating secrets and deployment-specific values in the root .env file (used by docker compose).
// Fallbacks preserve current defaults when env vars are absent (useful for legacy setups or local quick runs).

// Database connection configuration
// Database connection configuration (now using unified DB_* variables only)
define('DB_HOST', getenv('DB_HOST'));
define('DB_NAME', getenv('DB_NAME'));
define('DB_USER', getenv('DB_USER'));
define('DB_PASS', getenv('DB_PASS'));

// Tuya API configuration (export these in .env or compose secrets)
define('TUYA_CLIENT_ID', getenv('TUYA_CLIENT_ID'));
define('TUYA_CLIENT_SECRET', getenv('TUYA_CLIENT_SECRET'));
define('TUYA_DEVICE_ID', getenv('TUYA_DEVICE_ID') ?: '');
define('TUYA_API_ENDPOINT', getenv('TUYA_API_ENDPOINT'));

// Optional: expose a helper array if scripts prefer array-style config access later.
// $APP_CONFIG = [
//   'db' => [ 'host'=>DB_HOST,'name'=>DB_NAME,'user'=>DB_USER,'pass'=>DB_PASS ],
//   'tuya' => [ 'client_id'=>TUYA_CLIENT_ID,'device_id'=>TUYA_DEVICE_ID,'endpoint'=>TUYA_API_ENDPOINT ]
// ];
?>