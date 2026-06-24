<?php
header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Headers: Content-Type");
header("Access-Control-Allow-Methods: GET, POST, OPTIONS");
header("Content-Type: application/json; charset=UTF-8");

// CORS 사전 요청(Options) 처리
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    exit(0);
}

$host = '127.0.0.1';
$db   = 'rfid_db';
$user = 'rfid_user';
$pass = 'rfid_pass123!';
$charset = 'utf8mb4';

$dsn = "mysql:host=$host;dbname=$db;charset=$charset";
$options = [
    PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
    PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
    PDO::ATTR_EMULATE_PREPARES   => false,
];

try {
    $pdo = new PDO($dsn, $user, $pass, $options);
} catch (\PDOException $e) {
    echo json_encode(["error" => "Database connection failed: " . $e->getMessage()]);
    exit();
}

$action = isset($_GET['action']) ? $_GET['action'] : '';

switch ($action) {
    case 'tag':
        handleTag($pdo);
        break;
    case 'logs':
        handleLogs($pdo);
        break;
    case 'stats':
        handleStats($pdo);
        break;
    case 'download_csv':
        handleDownloadCsv($pdo);
        break;
    case 'users':
        handleUsers($pdo);
        break;
    case 'register':
        handleRegister($pdo);
        break;
    default:
        echo json_encode(["status" => "error", "message" => "Invalid action"]);
        break;
}

// 1. RFID 카드 태그 처리 API
function handleTag($pdo) {
    $uid = '';
    $input = json_decode(file_get_contents('php://input'), true);
    
    if (isset($input['uid'])) {
        $uid = trim($input['uid']);
    } elseif (isset($_POST['uid'])) {
        $uid = trim($_POST['uid']);
    }

    if (empty($uid)) {
        echo json_encode(["status" => "failed", "message" => "UID is required"]);
        return;
    }

    $uid = strtoupper($uid);

    // 1. 등록된 카드인지 조회 (phone 컬럼 추가 반영)
    $stmt = $pdo->prepare("SELECT username, role, phone FROM registered_cards WHERE UPPER(uid) = ?");
    $stmt->execute([$uid]);
    $userCard = $stmt->fetch();

    if ($userCard) {
        $username = $userCard['username'];
        $role = $userCard['role'];
        $phone = $userCard['phone'];
        $status = 'success';
        $remarks = '정상 출입';
    } else {
        $username = '미등록 카드';
        $role = '외부인';
        $phone = '';
        $status = 'failed';
        $remarks = '미등록 카드로 인한 거부';
    }

    // 2. 출입 로그 테이블에 기록 저장
    $logStmt = $pdo->prepare("INSERT INTO access_logs (uid, username, role, status, remarks) VALUES (?, ?, ?, ?, ?)");
    $logStmt->execute([$uid, $username, $role, $status, $remarks]);

    // 3. 결과 반환
    echo json_encode([
        "status" => $status,
        "username" => $username,
        "role" => $role,
        "phone" => $phone,
        "remarks" => $remarks,
        "uid" => $uid
    ]);
}

// 2. 실시간 로그 조회 API
function handleLogs($pdo) {
    try {
        $stmt = $pdo->query("SELECT id, uid, username, role, status, remarks, DATE_FORMAT(tagged_at, '%Y-%m-%d %H:%i:%s') as tagged_at FROM access_logs ORDER BY id DESC LIMIT 50");
        $logs = $stmt->fetchAll();
        echo json_encode($logs);
    } catch (\PDOException $e) {
        echo json_encode(["error" => $e->getMessage()]);
    }
}

// 3. 통계 데이터 조회 API
function handleStats($pdo) {
    try {
        $stmtTotal = $pdo->query("SELECT COUNT(*) as count FROM access_logs WHERE DATE(tagged_at) = CURDATE()");
        $total = $stmtTotal->fetch()['count'];

        $stmtSuccess = $pdo->query("SELECT COUNT(*) as count FROM access_logs WHERE DATE(tagged_at) = CURDATE() AND status = 'success'");
        $success = $stmtSuccess->fetch()['count'];

        $stmtFailed = $pdo->query("SELECT COUNT(*) as count FROM access_logs WHERE DATE(tagged_at) = CURDATE() AND status = 'failed'");
        $failed = $stmtFailed->fetch()['count'];

        echo json_encode([
            "total" => (int)$total,
            "success" => (int)$success,
            "failed" => (int)$failed
        ]);
    } catch (\PDOException $e) {
        echo json_encode(["error" => $e->getMessage()]);
    }
}

// 4. 접속 기록 CSV 다운로드 API
function handleDownloadCsv($pdo) {
    header('Content-Type: text/csv; charset=UTF-8');
    header('Content-Disposition: attachment; filename="rfid_access_logs_' . date('Ymd_His') . '.csv"');
    echo "\xEF\xBB\xBF"; // UTF-8 BOM

    $output = fopen('php://output', 'w');
    fputcsv($output, ['ID', '카드 UID', '사용자이름', '역할/직급', '출입상태', '비고', '태그시간']);

    try {
        $stmt = $pdo->query("SELECT id, uid, username, role, status, remarks, tagged_at FROM access_logs ORDER BY id DESC");
        while ($row = $stmt->fetch()) {
            fputcsv($output, [
                $row['id'],
                $row['uid'],
                $row['username'],
                $row['role'],
                $row['status'] === 'success' ? '승인' : '거부',
                $row['remarks'],
                $row['tagged_at']
            ]);
        }
    } catch (\PDOException $e) {
        fputcsv($output, ["Error: " . $e->getMessage()]);
    }
    
    fclose($output);
    exit();
}

// 5. 등록된 전체 사용자 목록 조회 API
function handleUsers($pdo) {
    try {
        $stmt = $pdo->query("SELECT id, uid, username, role, phone, DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') as created_at FROM registered_cards ORDER BY id DESC");
        $users = $stmt->fetchAll();
        echo json_encode($users);
    } catch (\PDOException $e) {
        echo json_encode(["error" => $e->getMessage()]);
    }
}

// 6. 새 카드 등록 / 정보 업데이트 API
function handleRegister($pdo) {
    $uid = '';
    $username = '';
    $role = '';
    $phone = '';

    // raw JSON 및 POST Form 데이터 지원
    $input = json_decode(file_get_contents('php://input'), true);
    
    if (isset($input['uid'])) {
        $uid = trim($input['uid']);
        $username = isset($input['username']) ? trim($input['username']) : '';
        $role = isset($input['role']) ? trim($input['role']) : '';
        $phone = isset($input['phone']) ? trim($input['phone']) : '';
    } else {
        $uid = isset($_POST['uid']) ? trim($_POST['uid']) : '';
        $username = isset($_POST['username']) ? trim($_POST['username']) : '';
        $role = isset($_POST['role']) ? trim($_POST['role']) : '';
        $phone = isset($_POST['phone']) ? trim($_POST['phone']) : '';
    }

    if (empty($uid) || empty($username)) {
        echo json_encode(["status" => "failed", "message" => "UID와 이름을 모두 입력해 주세요."]);
        return;
    }

    $uid = strtoupper($uid);

    try {
        $stmt = $pdo->prepare("INSERT INTO registered_cards (uid, username, role, phone) VALUES (?, ?, ?, ?) 
            ON DUPLICATE KEY UPDATE username = VALUES(username), role = VALUES(role), phone = VALUES(phone)");
        $stmt->execute([$uid, $username, $role, $phone]);

        echo json_encode([
            "status" => "success",
            "message" => "카드가 정상적으로 등록/업데이트되었습니다.",
            "data" => [
                "uid" => $uid,
                "username" => $username,
                "role" => $role,
                "phone" => $phone
            ]
        ]);
    } catch (\PDOException $e) {
        echo json_encode(["status" => "failed", "message" => "DB 오류: " . $e->getMessage()]);
    }
}
?>
