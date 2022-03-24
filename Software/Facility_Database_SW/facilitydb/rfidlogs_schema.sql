-- phpMyAdmin SQL Dump
-- version 
-- https://www.phpmyadmin.net/
--
-- Host: localhost
-- Generation Time: Mar 24, 2022 at 10:14 PM
-- Server version: 5.7.36-39-log
-- PHP Version: 8.0.2

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
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
DROP PROCEDURE IF EXISTS `sp_checkedInDisplay`$$
CREATE DEFINER=`makernexuswiki`@`localhost` PROCEDURE `sp_checkedInDisplay` (IN `dateToQuery` VARCHAR(8))   BEGIN 

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
       	  AND  logEvent in 
             (select logEvent from stationConfig 
               where active = 1
                 and length(logEvent) > 0
             )
 	) AS  p 
ON a.clientID = p.clientID
;

-- add the photoDisplay column and return the final query
SELECT DISTINCT photoDisplay, a.clientID, a.firstName, c.displayClasses
FROM tmp_client_taps a 
LEFT JOIN stationConfig b 
ON a.logEvent = b.logEvent
LEFT JOIN clientInfo c
ON a.clientID = c.clientID
ORDER BY firstName, photoDisplay
;

END$$

DROP PROCEDURE IF EXISTS `sp_checkedInDisplayOLD`$$
CREATE DEFINER=`makernexuswiki`@`localhost` PROCEDURE `sp_checkedInDisplayOLD` (IN `dateToQuery` VARCHAR(8))   BEGIN 

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
SELECT DISTINCT photoDisplay, a.clientID, a.firstName, c.displayClasses
FROM tmp_client_taps a 
LEFT JOIN stationConfig b 
ON a.logEvent = b.logEvent
LEFT JOIN clientInfo c
ON a.clientID = c.clientID
ORDER BY firstName, photoDisplay
;

END$$

DROP PROCEDURE IF EXISTS `sp_insert_update_clientInfo`$$
CREATE DEFINER=`makernexuswiki`@`localhost` PROCEDURE `sp_insert_update_clientInfo` (IN `INclientID` INT, IN `INfirstName` VARCHAR(255), IN `INlastName` VARCHAR(255), IN `INdateLastSeen` DATETIME, IN `INisCheckedIn` INT)   BEGIN 

IF EXISTS 
    (
    SELECT clientID FROM clientInfo 
	WHERE clientID = INclientID
    )
	
THEN
	UPDATE clientInfo set firstName = INfirstName, lastName = INLastName,
    	dateLastSeen = INdateLastSeen 
    WHERE clientID = INclientID;
    
ELSE
 	INSERT INTO clientInfo (clientID, firstName, lastName, dateLastSeen, isCheckedIn)
    VALUES (INclientID, INfirstName, INlastName, INdateLastSeen, isCheckedIn);
    
END IF;

END$$

DELIMITER ;

-- --------------------------------------------------------

--
-- Table structure for table `clientInfo`
--

DROP TABLE IF EXISTS `clientInfo`;
CREATE TABLE `clientInfo` (
  `recNum` int(11) NOT NULL,
  `dateCreated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `clientID` int(11) NOT NULL,
  `lastName` varchar(255) NOT NULL,
  `firstName` varchar(255) NOT NULL,
  `dateLastSeen` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `isCheckedIn` int(11) NOT NULL DEFAULT '0',
  `displayClasses` varchar(25) NOT NULL DEFAULT ' ' COMMENT 'Will be the class attribute of the checkin photo screen',
  `MOD_Eligible` tinyint(1) NOT NULL DEFAULT '0' COMMENT 'true if client can be MoD'
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- --------------------------------------------------------

--
-- Table structure for table `rawdata`
--

DROP TABLE IF EXISTS `rawdata`;
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

DROP TABLE IF EXISTS `stationConfig`;
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
-- Indexes for table `clientInfo`
--
ALTER TABLE `clientInfo`
  ADD PRIMARY KEY (`recNum`),
  ADD UNIQUE KEY `clientID` (`clientID`);

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
-- AUTO_INCREMENT for table `clientInfo`
--
ALTER TABLE `clientInfo`
  MODIFY `recNum` int(11) NOT NULL AUTO_INCREMENT;

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
