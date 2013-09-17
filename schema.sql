DROP DATABASE IF EXISTS `mcpp`;

CREATE DATABASE `mcpp` CHARACTER SET 'utf8mb4' COLLATE 'utf8mb4_unicode_ci';

USE `mcpp`;

CREATE TABLE `settings` (
	`setting` varchar(191) PRIMARY KEY,
	`value` text
);

CREATE TABLE `log` (
	`when` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`type` varchar(255) NOT NULL,
	`text` text NOT NULL
);

CREATE TABLE `chat_log` (
	`when` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
	`from` text NOT NULL,
	`to` text,
	`message` text NOT NULL,
	`notes` text
);

CREATE TABLE `keyvaluestore` (
	`key` varchar(191) NOT NULL,
	`value` text NOT NULL,
	INDEX(`key`)
);

CREATE TABLE `binary` (
	`key` varchar(191) PRIMARY KEY,
	`value` mediumblob NOT NULL
);
