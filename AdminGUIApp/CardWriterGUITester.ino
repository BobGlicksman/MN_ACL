/******************************************************************************************
 * CardWriterGUITester:  program to test the AI2 GUI App.
 * 
 * This program contains cloud functions and cloud variables used to test out the MN 
 * Card Writer GUI App.  The Card Writer GUI App is an MIT App Inventor 2 App
 * that provides the MN administrator with a user interface for RFID card
 * administration.
 * 
 * (c) 2019, Team Practical Projects, Bob Glicksman, Jim Schrempp
 * version 1.3; by: Bob Glicksman; 9/06/19
 * *****************************************************************************************/

// Global variables
String deviceInfo = "Firmware version 1.3. Last reset @ ";
String memberData = "";
String cardInfo = "";

void setup() {
    // Cloud vartiables for the app to read
    Particle.variable("deviceInfo", deviceInfo);
    Particle.variable("queryMemberResult", memberData);
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

    0 = query accepted
	1 = query is underway
	2 = query is done and results are in the cloud variable (including any error report)
	3 = query was already underway, this query is rejected
	4 = memberNumber was not a number or was 0
	5 = other error, see LCD panel on device


    This function is called initially and if the return value is 0, it is called repeatedly on a timer
    until the return value is > 1.  Successful query (return 2) will load the JSON representation of member 
    data from EZ Facility into the cloud variable:

	String queryMemberResult;

        Where queryMemberResult is JSON formatted name:value pairs.  The GUI will present each 
        JSON name as a literal and each JSON value next to the variable name.  An example of name:value pairs is:

        ErrorCode: 0
	    ErrorMessage: OK
        Name : Bob Glicksman
	    ClientID : 12345678
    	Status : Active

    Where ErrorCode is 
	    0 = member found and membership data is in the JSON.
	    1 = member not found in EZFacility
	    2 = more than one member found in EZfacility (this should be impossible)
	    3 = other error.

    ErrorMessage is a short string in English, suitable for display to a user.

************************************************************/
int queryMember(String memberNumber) {
    // define some member data
    String memberBob = "{\"ErrorCode\":0, \"ErrorMessage\":\"OK\", \"Name\":\"Bob Glicksman\", \"ClientID\":12345678, \"Status\":\"deadbeat\"}";
    String memberJim = "{\"ErrorCode\":0, \"ErrorMessage\":\"OK\", \"Name\":\"Jim Schrempp\", \"ClientID\":98765432, \"Status\":\"active\"}";
    String somethingWrong = "{\"ErrorCode\":1, \"ErrorMessage\":\"member not found\", \"Name\":\"not found\", \"ClientID\":0, \"Status\":\"none\"}";
    
    static int state = 0;
    static int resultCode = 2;  // initialize for good data
    static int lastData = 0;   // toggle between results

    Particle.publish("received member number = ", memberNumber, PRIVATE);
    
    switch(state) {
        case 0: // initial call to fire things off
            state = 1;
            return 0;
        
        case 1: // waiting for a response to the query
            state = 2;  // next time, there will be a response
            return 1;
        
        case 2: // query has completed, for good or bad
            state = 0;  // next time will be a new query
            
            switch(resultCode)  {   // send back different results for testing
                case 2: // good data from query
                    if(lastData == 0) {
                        memberData = memberBob;
                        lastData = 1;
                    }
                    else if(lastData == 1) {
                        memberData = memberJim;
                        lastData = 2;                   
                    } 
                    else {
                        memberData = somethingWrong;
                        lastData = 0; 
                    }
                    
                    resultCode = 3; // Sset the next result type
                    state = 0;
                    return 2;
                    
                case 3: // query underway, rejected
                    memberData = "";
                    resultCode = 4; // Sset the next result type
                    state = 0;
                    return 3;
                    
                case 4: // member not found or NAN
                    memberData = somethingWrong;
                    resultCode = 5;  // Sset the next result type
                    state = 0;
                    return 4;
                    
                case 5: // other error
                    memberData = "";
                    resultCode = 2;  // Sset the next result type
                    state = 0;
                    return 5;
                    
                default:    // should never get here
                    memberData = "";
                    resultCode = 2;  // Sset the next result type
                    state = 0;
                    return 5;               
            }
        
        default:    // should never get here
            resultCode = 2;  // set the next result type
            state = 0;
            return 5;            
    }
    
}  // end of querymember()

/**********************************************************
int burnCard(String clientID)

    Where: clientID is a String representation of the clientID that was returned 
        in the membership information JSON response above
        
    The return value is an int with the following encoding:

	0 = function called successfully; follow instructions on the LCD
    1 = failed to contact card reader/write

************************************************************/
int burnCard(String clientID) {
    static int errorCode = 0;

    Particle.publish("received client ID = ", clientID, PRIVATE);
    
    switch(errorCode++ % 2) {
        case 0: // card burned successfully
            return 0;
            
        case 1: // bad card; burn unsuccessful
            return 1;
         
         default:   // should never get here
            return 1;
    }
}   // end of burnCard

/**********************************************************
int resetCard(String dummy)

    The String argument is not used (required by Particle in the definition)
    
    The return value is an int with the following encoding:

    0 = function called; follow instructions on the LCD panel
	1 = failure to contact the card reader/writer hardware

************************************************************/
int resetCard(String dummy) {
    static int errorCode = 0;

    Particle.publish("reset card ", "called", PRIVATE);
    
    switch(errorCode++ % 2) {
        case 0: // card reset successfully
            return 0;
            
        case 1: // card is already factory fresh
            return 1;
         
         default:   // should never get here
            return 1;
    }  
}   // end of resetCard()


/**********************************************************
int identifyCard(String dummy)

    The String argument is not used (required by Particle in the definition)

    The return value is an int with the following encoding:

    0 = card is MN formatted; query for member data initiated
	1 = query is underway (return in one second)
	2 = query complete; card owner data is in the string memberInformation
	3 = card is factory fresh (blank)
    4 = card not identified; card format unknown.


    Successful query will load the JSON representation of member data from 
        EZ Facility into the cloud variable:

	String memberInformation;

    Where memberInformation is JSON formatted name:value pairs.  The GUI will present 
        each JSON name as a literal and each JSON value next to the variable name.  
        An example of name:value pairs is:

        ErrorCode: 0
	    ErrorMessage: OK
        Name : Bob Glicksman
        Member Number :  7654
	    Card Status :  Current (alternative: Revoked)


**********************************************************************/
int identifyCard(String dummy) {
    // define some member data
    String cardBob = "{\"ErrorCode\":0, \"ErrorMessage\":\"OK\", \"Name\":\"Bob Glicksman\", \"Member Number\":1234, \"Card Status\":\"current\"}";
    String cardJim = "{\"ErrorCode\":0, \"ErrorMessage\":\"OK\", \"Name\":\"Jim Schrempp\", \"Member Number\":8765, \"Card Status\":\"revoked\"}";
    String somethingWrong = "{\"ErrorCode\":3, \"ErrorMessage\":\"member not found\", \"Name\":\"not found\", \"Member Number\":0, \"Card Status\":\"bad card\"}";
    
    static int state = 0;
    static int resultCode = 2;  // initialize for good data
    static bool lastData = false;   // toggle betweent wo good results

    Particle.publish("identifyCard ", "called", PRIVATE);
    
    switch(state) {
        case 0: // initial call to fire things off
            state = 1;
            return 0;
        
        case 1: // waiting for a response to the query
            state = 2;  // next time, there will be a response
            return 1;
        
        case 2: // query has completed, for good or bad
            state = 0;  // next time will be a new query
            
            switch(resultCode)  {   // send back different results for testing
                case 2: // query complete; card owner data is in the string memberInformation
                    if(lastData == false) {
                        cardInfo = cardBob;
                        lastData = true;
                    }
                    else {
                        cardInfo = cardJim;
                        lastData = false;                   
                    }
                    
                    resultCode = 3; // Sset the next result type
                    state = 0;
                    return 2;
                    
                case 3: // card is factory fresh (blank)
                    cardInfo = "";
                    resultCode = 4; // Sset the next result type
                    state = 0;
                    return 3;
                    
                case 4: // card not identified; card format unknown.
                    cardInfo = somethingWrong;
                    resultCode = 2;  // Sset the next result type
                    state = 0;
                    return 4;
                    
                default:    // should never get here
                    cardInfo = "";
                    resultCode = 2;  // Sset the next result type
                    state = 0;
                    return 4;               
            }
        
        default:    // should never get here
            resultCode = 2;  // set the next result type
            state = 0;
            return 5;            
    }
    
}  // end of identifyCard()