DELIMITER $$

-- This stored proc maintains the clientInfo table. When called it looks for
-- clientID in the table. It either inserts a new row or updates an existing
-- row with the parameters passed in.

DROP PROCEDURE IF EXISTS sp_insert_update_clientInfo$$

CREATE DEFINER=`makernexuswiki`@`localhost` PROCEDURE `sp_insert_update_clientInfo` (
	IN INclientID INT,
	IN INfirstName VARCHAR(255),
	IN INlastName VARCHAR(255),
	IN INdateLastSeen DATETIME,
	IN INisCheckedIn INT)
	
BEGIN 

IF EXISTS (
    SELECT clientID FROM clientInfo 
	WHERE clientID = INclientID)
	
THEN
	UPDATE clientInfo set firstName=INfirstName, lastName=INLastName,
    	dateLastSeen=INdateLastSeen 
    WHERE clientID = INclientID;
    
ELSE
	INSERT INTO clientInfo (clientID, firstName, lastName, dateLastSeen, isCheckedIn)
    VALUES (INclientID, INfirstName, INlastName, INdateLastSeen, isCheckedIn);
    
END IF;


END$$
