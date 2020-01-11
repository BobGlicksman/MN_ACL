-- phpMyAdmin SQL Dump
-- version 
-- https://www.phpmyadmin.net/
--
-- Host: localhost
-- Generation Time: Jan 11, 2020 at 12:29 AM
-- Server version: 5.7.23-percona-sure1-log
-- PHP Version: 7.2.21

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET AUTOCOMMIT = 0;
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `makernexuswiki_rfidlogs_sandbox`
--
CREATE DATABASE IF NOT EXISTS `makernexuswiki_rfidlogs_sandbox` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;
USE `makernexuswiki_rfidlogs_sandbox`;

DELIMITER $$
--
-- Procedures
--
CREATE DEFINER=`makernexuswiki`@`localhost` PROCEDURE `sp_checkedInDisplay` (IN `dateToQuery` VARCHAR(8))  BEGIN 

-- get list of members checked in
DROP TEMPORARY TABLE IF EXISTS tmp_checkedin_clients;
CREATE TEMPORARY TABLE tmp_checkedin_clients AS
(
SELECT maxRecNum, clientID, logEvent
 	FROM
	(
     SELECT  MAX(recNum) as maxRecNum, clientID 
        FROM rawdata
       WHERE logEvent in ('Checked In','Checked Out')
         AND CONVERT( dateEventLocal, DATE) = CONVERT(dateToQuery, DATE) 
         AND clientID <> 0
      GROUP by clientID
    ) AS p 
    LEFT JOIN
    (
     SELECT  recNum, logEvent 
        FROM rawdata
       WHERE CONVERT( dateEventLocal, DATE) = CONVERT(dateToQuery, DATE) 
    ) AS q 
    ON p.maxRecNum = q.recNum
    HAVING logEvent = 'Checked In'
)
;

-- create table with members and the stations they have tapped
DROP TEMPORARY TABLE IF EXISTS tmp_client_taps;
CREATE TEMPORARY TABLE tmp_client_taps
SELECT DISTINCT p.logEvent, p.clientID, p.firstName
FROM tmp_checkedin_clients a 
LEFT JOIN 
	(
     	SELECT * 
        FROM rawdata 
        WHERE CONVERT( dateEventLocal, DATE) = CONVERT(dateToQuery, DATE) 
       	  AND  logEvent in (select logEvent from stationConfig where active = 1)
 	) AS  p 
ON a.clientID = p.clientID
;

-- add the photoDisplay column and return the final query
SELECT DISTINCT photoDisplay, clientID, firstName
FROM tmp_client_taps a 
LEFT JOIN stationConfig b 
ON a.logEvent = b.logEvent
ORDER BY firstName, photoDisplay
;

END$$

DELIMITER ;

-- --------------------------------------------------------

--
-- Table structure for table `rawdata`
--

CREATE TABLE `rawdata` (
  `recNum` int(11) NOT NULL,
  `dateCreated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `datePublishedAt` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `dateEventLocal` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `coreID` varchar(255) NOT NULL,
  `deviceFunction` varchar(255) NOT NULL,
  `clientID` varchar(255) NOT NULL DEFAULT ' ',
  `firstName` varchar(255) NOT NULL,
  `eventName` varchar(255) NOT NULL,
  `logEvent` varchar(255) NOT NULL DEFAULT 'Not Provided',
  `logData` varchar(255) NOT NULL DEFAULT ' ',
  `ipAddress` varchar(45) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `stationConfig`
--

CREATE TABLE `stationConfig` (
  `recNum` int(11) NOT NULL,
  `active` int(11) NOT NULL DEFAULT '0' COMMENT 'if <> 0 this is the active record for the deviceType',
  `deviceType` int(11) NOT NULL COMMENT 'number used by device when looking for its config',
  `deviceName` varchar(20) NOT NULL COMMENT 'Will be logged as deviceFunction in raw data',
  `LCDName` varchar(16) NOT NULL COMMENT 'short name to display on LCD',
  `photoDisplay` varchar(16) NOT NULL COMMENT 'short word to put under photo on Checked In web page',
  `OKKeywords` varchar(128) NOT NULL COMMENT 'comma delimited. any word found in package allows checkin',
  `logEvent` varchar(32) NOT NULL COMMENT 'Used as Particle Event Name. Will show up in rawdata',
  `dateCreated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `rawdata`
--
ALTER TABLE `rawdata`
  ADD PRIMARY KEY (`recNum`),
  ADD KEY `i_dateEventLocal` (`dateEventLocal`);

--
-- Indexes for table `stationConfig`
--
ALTER TABLE `stationConfig`
  ADD PRIMARY KEY (`recNum`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `rawdata`
--
ALTER TABLE `rawdata`
  MODIFY `recNum` int(11) NOT NULL AUTO_INCREMENT;

--
-- AUTO_INCREMENT for table `stationConfig`
--
ALTER TABLE `stationConfig`
  MODIFY `recNum` int(11) NOT NULL AUTO_INCREMENT;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
