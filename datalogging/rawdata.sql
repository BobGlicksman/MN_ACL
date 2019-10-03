-- phpMyAdmin SQL Dump
-- version 
-- https://www.phpmyadmin.net/
--
-- Host: localhost
-- Generation Time: Sep 22, 2019 at 11:55 PM
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
-- Database: `makernexuswiki_rfidlogs`
--

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
  `eventName` varchar(255) NOT NULL,
  `logEvent` varchar(255) NOT NULL DEFAULT 'Not Provided',
  `logData` varchar(255) NOT NULL DEFAULT ' '
) ENGINE=MyISAM DEFAULT CHARSET=utf8;


--
-- Indexes for table `rawdata`
--
ALTER TABLE `rawdata`
  ADD PRIMARY KEY (`recNum`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `rawdata`
--
ALTER TABLE `rawdata`
  MODIFY `recNum` int(11) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=34;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
