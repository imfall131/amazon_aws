CREATE DATABASE IF NOT EXISTS rfid_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER IF NOT EXISTS 'rfid_user'@'localhost' IDENTIFIED BY 'rfid_pass123!';
GRANT ALL PRIVILEGES ON rfid_db.* TO 'rfid_user'@'localhost';
FLUSH PRIVILEGES;

USE rfid_db;

CREATE TABLE IF NOT EXISTS registered_cards (
    id INT AUTO_INCREMENT PRIMARY KEY,
    uid VARCHAR(50) UNIQUE NOT NULL,
    username VARCHAR(50) NOT NULL,
    role VARCHAR(50) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS access_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    uid VARCHAR(50) NOT NULL,
    username VARCHAR(50) DEFAULT '미등록 카드',
    role VARCHAR(50) DEFAULT '외부인',
    status VARCHAR(20) NOT NULL, -- 'success' or 'failed'
    remarks VARCHAR(255) DEFAULT '',
    tagged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- 기본 카드 등록
INSERT IGNORE INTO registered_cards (uid, username, role) VALUES 
('04 A2 3B 8C', '홍길동', '총괄 관리자'),
('D3 F2 9A 4E', '이영희', '보안 담당자'),
('E2 B1 8F 9C', '김철수', '시스템 엔지니어'),
('8C C1 D3 42', '박지성', '외부 연구원');
