/******************************************************************************************
 * CardWriterGUITester:  program to test the AI2 GUI App.
 * 
 * This program contains cloud functions and cloud variables used to test out the MN 
 * Card Writer GUI App.  The Card Writer GUI App is an MIT App Inventor 2 App
 * that provides the MN administrator with a user interface for RFID card
 * administration.
 * 
 * (c) 2019, Team Practical Projects, Bob Glicksman, Jim Schrempp
 * version 1.0; by: Bob Glicksman; 8/31/19
 * *****************************************************************************************/

// Global variables
String deviceInfo = "Firmware version 1.0. Last reset @ ";
String memberData = "";
String cardInfo = "";

void setup() {
    // Cloud vartiables for the app to read
    Particle.variable("deviceInfo", deviceInfo);
    Particle.variable("memberData", memberData);
    Particle.variable("cardInfo", cardInfo);
    
    // Cloud functions for the app to call
    Particle.function("queryMember", queryMember);
    Particle.function("burnCard", burnCard);
    Particle.function("resetCard", resetCard);
    Particle.function("identifyCard", identifyCard);
    
    pinMode(D7, OUTPUT);    // the on-board LED
    
    // wait for time to become valid and add current time to deviceInfo string
    while(!Time.isValid()) {
        Particle.process();
    }
    deviceInfo += Time.timeStr();
    deviceInfo += " Z";
    
    // blink the D7 LED to indicate that setup() is complete
    digitalWrite(D7, HIGH);
    delay(500);
    digitalWrite(D7, LOW);
    delay(500);
    digitalWrite(D7, HIGH);
    delay(500);
    digitalWrite(D7, LOW);
    delay(500);
}

void loop() {
    // do nothing here.  Cloud functions only.

}

/************ Cloud Functions for Testing *****************/

/**********************************************************
int queryMember(String memberNumber)

    Where: memberNumber is a String representation of the number typed into the GUI by the administrator.

    The return value is an int with the following encoding:

	0 = member found and membership data is in the cloud variable.
	1 = membership number incorrect (e.g. not a number)
	2 = member not found in EZFacility
	3 = more than one member found in EZfacility
	4 = other error.

    Successful query will load the JSON representation of member data from EZ Facility into the cloud variable:

	String memberInformation;

        Where memberInformation is JSON formatted name:value pairs.  The GUI will present each 
        JSON name as a literal and each JSON value next to the variable name.  An example of name:value pairs is:

        	Name : Bob Glicksman
        	ClientID : 12345678
        	Status : Active
************************************************************/
int queryMember(String memberNumber) {
    // define some member data
    String memberBob = "name: Bob Glicksman; clientID: 12345678; status: deadbeat";
    String memberJim = "name: Jim Schrempp; clientID: 98765432; status: active";
    static uint8_t errorCode = 0;
    static bool lastData = false;

    Particle.publish("received member number = ", memberNumber, PRIVATE);
    
    switch(errorCode++ % 5) {
        case 0: // member data found; toggle the data reported in the global String
            if(lastData == false) {
                memberData = memberBob;
                lastData = true;
            }
            else {
                memberData = memberJim;
                lastData = false;
            }
            return 0;
            
        case 1: // membership number incorrect
            memberData = "ERROR: member number is incorrect. Please try again.";
            return 1;
         
        case 2: // member not found in EZ Facility
            memberData = "ERROR: member not found in EZ Facility.  Please try again.";
            return 2;
        
        case 3: // more than one member matches the member number
            memberData = "ERROR: member number is not unique in EZ Facility.  Please try again.";
            return 3;
            
        case 4: // other error
            memberData = "ERROR: an unidentifed error occurred.  Please try again.";
            return 4;
        
         default:
            memberData = "CODE ERROR -- incorrect case";
            return 4;
    }
   
}  // end of querymember()

/**********************************************************
int burnCard(String clientID)

    Where: clientID is a String representation of the clientID that was returned 
        in the membership information JSON response above
        
    The return value is an int with the following encoding:

	0 = card burned successfully
	1 = bad card; burn unsuccessful
	2 = EZFacility issue; burn unsuccessful
	3 = other error.
************************************************************/
int burnCard(String clientID) {
    static uint8_t errorCode = 0;

    Particle.publish("received client ID = ", clientID, PRIVATE);
    
    switch(errorCode++ % 4) {
        case 0: // card burned successfully
            return 0;
            
        case 1: // bad card; burn unsuccessful
            return 1;
         
        case 2: // EZFacility issue; burn unsuccessful
            return 2;
        
        case 3: // other error
            return 3;
        
         default:   // should never get here
            return 4;
    }
}   // end of burnCard

/**********************************************************
int resetCard(String dummy)

    The String argument is not used (required by Particle in the definition)
    
    The return value is an int with the following encoding:

	0 = card successfully reset
	1 = card is already factory fresh
	2 = not an MN formatted card (unknown format); card not reset
    3 = other error; card not reset
************************************************************/
int resetCard(String dummy) {
    static uint8_t errorCode = 0;

    Particle.publish("reset card ", "called", PRIVATE);
    
    switch(errorCode++ % 4) {
        case 0: // card reset successfully
            return 0;
            
        case 1: // card is already factory fresh
            return 1;
         
        case 2: // not an MN formatted card (unknown format); card not reset
            return 2;
        
        case 3: // other error; card not reset
            return 3;
        
         default:   // should never get here
            return 4;
    }  
}   // end of resetCard()


/**********************************************************
int identifyCard(String dummy)

    The String argument is not used (required by Particle in the definition)

    The return value is an int with the following encoding:

	0 = card successfully identified
	1 = card is factory fresh (blank)
    2 = card not identified; card format unknown.

    Successful query will load the JSON representation of member data from 
        EZ Facility into the cloud variable:

	String memberInformation;

    Where memberInformation is JSON formatted name:value pairs.  The GUI will present 
        each JSON name as a literal and each JSON value next to the variable name.  
        An example of name:value pairs is:

    	Name : Bob Glicksman
        Member Number :  7654
    	Card Status :  Current (alternative: Revoked)

**********************************************************************/
int identifyCard(String dummy) {
    // define some member data
    String cardBob = "name: Bob Glicksman; member ID: 1234; status: currentt";
    String cardJim = "name: Jim Schrempp; member ID: 8765; status: revoked";
    static uint8_t errorCode = 0;
    static bool lastData = false;

    Particle.publish("card ", "identified", PRIVATE);
    
    switch(errorCode++ % 3) {
        case 0: // card successfully identified; toggle the data reported in the global String
            if(lastData == false) {
                cardInfo = cardBob;
                lastData = true;
            }
            else {
                cardInfo = cardJim;
                lastData = false;
            }
            return 0;
            
        case 1: // card is factory fresh (blank)
            cardInfo = "ERROR: card is factory fresh (blank).";
            return 1;
         
        case 2: // card not identified; card format unknown.
            memberData = "ERROR: card not identified; card format unknown.";
            return 2;
    
         default:
            memberData = "CODE ERROR -- incorrect case";
            return 4;
    }
}   // end of identifyCard()

