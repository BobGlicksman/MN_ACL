DELIMITER $$

-- This procedure returns a rowset of information for the web display
-- that shows photos of the members who are currently checked in to 
-- the facility: rfidcurrentcheckins.php


DROP PROCEDURE IF EXISTS sp_checkedInDisplay $$

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
