-- phpMyAdmin SQL Dump
-- version 
-- https://www.phpmyadmin.net/
--
-- Host: localhost
-- Generation Time: Jan 22, 2020 at 01:32 AM
-- Server version: 5.7.23-percona-sure1-log
-- PHP Version: 7.2.21

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET AUTOCOMMIT = 0;
START TRANSACTION;
SET time_zone = "+00:00";

--
-- Dumping data for table `stationConfig`
--

INSERT INTO `stationConfig` (`recNum`, `active`, `deviceType`, `deviceName`, `LCDName`, `photoDisplay`, `OKKeywords`, `logEvent`, `dateCreated`) VALUES
(1, 1, 1, 'Check In', 'Check In', '', '', 'checkin allowed', '2019-12-29 17:20:51'),
(2, 1, 2, 'Admin Station', 'Admin', '', '', '', '2019-12-29 17:20:51'),
(3, 1, 4, 'Woodshop', 'Wood', 'Wood', 'Wood, ShopBot', 'Woodshop allowed', '2019-12-29 17:20:51'),
(4, 1, 0, 'Default', 'Undefined', '', '', '', '2019-12-29 17:20:51'),
(5, 0, 1, 'Check In Inactive', 'Check In Inactiv', '', 'OLD record, should not get this', '', '2019-12-29 17:20:51'),
(6, 1, 106, 'Textiles', 'Textile', 'Textile', 'Consew, Barudan, Serger, Heat Press, Vinyl, Sewing, Embroidery', 'Textile allowed', '2019-12-29 17:28:39'),
(7, 1, 105, 'Lasers', 'Laser', 'Laser', 'Laser', 'Laser allowed', '2019-12-29 17:28:39'),
(8, 1, 107, '3D Printers', '3D Printer', '3D', '3D', '3D allowed', '2019-12-29 17:29:28'),
(9, 1, 108, 'Electronics Bench', 'Electronics', 'Elec.', '', 'Electronics', '2019-12-29 17:30:22');
