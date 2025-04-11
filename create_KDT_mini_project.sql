DROP DATABASE IF EXISTS KDT_mini_project;
CREATE DATABASE KDT_mini_project DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- 어디다 만들지 지정
USE KDT_mini_project;

CREATE TABLE users (
	user_id int primary key,
    username varchar(50) not null,
    password varchar(50),
    status enum("active", "suspended"),
    created_at datetime
);

create table message_log(
	message_id INT AUTO_INCREMENT PRIMARY KEY,
    sender_id int,
    content varchar(200),
    sent_at datetime,
	FOREIGN KEY (sender_id) REFERENCES users(user_id)
	ON DELETE cascade
	ON UPDATE CASCADE
);

create table user_sessions(
	session_id int AUTO_INCREMENT PRIMARY KEY,
    user_id int,
    login_time datetime,
    logout_time datetime,
    ip_address varchar(50),
	FOREIGN KEY (user_id) REFERENCES users(user_id)
	ON DELETE cascade
	ON UPDATE CASCADE
);