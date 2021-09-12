# Maker Nexus EzFacility Local Cache/Proxy server proposal and specifications
## General Overview and Goals of the enhancement
The idea here is to create a local cache of the EZFacility Data on an SQL server in the building.
This cache should provide several advantages:
+	Reduced query load against EZFacility = small cost savings (we get charged per API call)
+	Increased responsiveness to queries for Member data (goal is to answer all requests from readers in <250ms)
+	Quick failover to transparent proxy if SQL server fails for any reaeson (sub 1s reversion if SQL doesn't answer)
+	Nearly transparent to existing code (Host portion of URL code changes in order to point to in-house server)
+	Possible additioanl automatic failover to direct API calls if local cache server fails to respond

### Implementation stages and dependencies
The implementation will be in stages and should be relatively straight forward...

#### Stage 1 -- PoC for building and refreshing the cache
The cache will be an SQL database with a schema built from the schema for a client record in the EZF API.
Currently a member record consists of:
```
Array [
	Struct {
		ClientID			integer
		LocationID			integer
		AccountNumber			string
		SSUserName			string
		Organization			string
		MembershipNumber		string
		MembershipClassification	string
		MembershipHasUnlimitedVisits	bool
		MembershipMaxVisits		integer
		MembershipVisitsCount		integer
		MembershipVisitsRemaining	integer
		FirstName			string
		LastName			string
		Email				array [
				string, ...
		]
		EmailSubscriptions		array [
			{
				EmailAddress		string
				Subscriptions		array [
					Struct {
						Subscription	string
						Optin		bool
					}, ...
				]
			}
		]
		DateOfBirth			dateTime (YYYY-MM-DDTHH:MM:SS[.SSS][TZ])
		Gender				string
		DateCreated			dateTime
		LastModified			dateTime
		Active				bool
		City				string
		PostalCode			string
		PhoneCell			string
		AmountDue			fixedPoint(2 decimal places) (Store in SQL as DECIMAL(10,2))
		CustomFields			array [
			Struct {
				Name			string
				Value			string
			},...
		]
		MemberPhotoURL			string
	}
]
```

The following API calls will be used in the PoC:

1.	POST https://api.sandbox.ezfacility.com/token
	Authroization Basic using application credentials (see primary dev documents)
	grant_type=password&username=<user>&password=<password>&scope=realm:tms

	The response will contain an "access_token" string and "expires_in" integer which
	we will need to provid in all subsequent API calls.

2.	GET https://api.sandbox.ezfacility.com/v2/user/locations
	Authorization: Bearer <token>
	Accept: application/json

	We will use the first Location ID supplied, as there should be only one for now.

	If we reach a point where there is more than one, the code will need to be extended
	to support identifying the correct Location ID in a variety of places.

3.	GET https://api.sandbox.ezfacility.com/v2/<locationID>/<dateTime>/clients/all
	Accept: application/json
	ez-page-number: 1
	Authorization: Bearer <token>

	This URL will return the first page of updates since the dateTime specified. In the headers,
	it will also return the number of results per page ((ez-per-page) appears to be 100 as of this writing), the
	total count of records (ez-total-count), and total available pages (ez-total-pages).

	Every available field is returned for each modified client. TBD: Store everything or store only the
	fields we care about? (In the latter case, the cache will not return identical results as it will
	have only a subset of the fields to put into the JSON returned, but if it has everything we care
	about, the JSON parser shouldn't care about the missing fields). Since it's relatively cheap,
	default decision unless argued otherwise will be store everything.

	Open questions:
		Do we need to deal gracefully with the situation where EZF changes the schema?
		Does this need to be automated?
		Can we just ignore fields that in the EZF JSON that don't exist in our table?
		(Build the table manually then populate automatically from API or build table
		from API data?)

	The above call will be repeated for subsequent calls if there are any.

	When the database is rebuilt from scratched, the following will occur:

	A.	Scrub the database (drop table and create a new one)
	B.	Pull in all pages using the API call with a dateTime for the earliest
		modified record of the unix epoch (1970-01-01T00:00:00.000Z).
	C.	Create and set the timestamp record. (See below).

	Once allrecords have been loaded into the database, a cron job should periodically
	update the database with new records. A special record in the cache with an AccountNumber "CacheTimeStamp" will store 
	the Initial Load of the database time in "DateCreated" and will store the last cache update that successfully completed in "LastModified".
	The initial value of "LastModified" will be the same as "DateCreated".

	Subsequent calls to the API will use the LastModified date - 2*T  seconds as the earliest modified in the API calls, where T is the
	expected duration of a cache refresh cycle (e.g. 1 hour, 3600 seconds). Going back by the expected duration of two cache refresh cycles 
	will result in some duplication of updates, but it should not be excessive and will serve as a safeguard to make sure that all updates are
	eventually received. (for example, if we update the cache every hour, we will receive the roughly the last 3 hours of changes in response
	to our update request). Since we rarely have 33 updates in a day, let alone an hour, this shouldn't increase the page count and therefore
	should not increase the number of API calls required to obtain the data.

#### Stage 2: Build a cache server and APIs to answer queries from Checkin and FrontDoor boxes.
#### Stage 3: Build a test version of the software for the Checkin Boxes that uses the cache server API instead of EZF
#### Stage 4: Implement Passthru when SQL problem detected
#### Stage 5: Build a test version of the software for the checkin and other applicable boxes that tries the cache server first and fails through to the EZFacility API if a satisfactory response isn't received within 500ms.
#### Stage 6: If all goes well with stages 1-5, document performance and behavior and begin in-facility testing.
#### Stage 7: Deploy generally in production.

### SQL Schema
There are two possible ways to go here. The initial implementation for the PoC will manually create the SQL schema.
In theory it is possible to detect changes in the schema and alter the table accordingly through automated processes, but this
involves some tradeoffs and might make the code more fragile. In general, if the schema change involves fields we care about,
it should be possible to modify the code and table manually before they are put into use. The code must be robust against
schema changes in that it should continue to function and not generate SQL statements that can't be executed against
the current database regradless of schema changes implemented by EZFacility.

The initial SQL schema will be as follows:

```
TABLE: Members
JSON Field Name				SQL Field Name				SQL Data Type(length)
ClientID				ClientID				BIGINT(12)
LocationID				LocationID				BIGINT(12)
AccountNumber				AccountNumber				VARCHAR(32)
SSUserName				SSUserName				VARCHAR(32)
Organization				Organization				VARCHAR(32)
MembershipNumber			MembershipNumber			VARCHAR(12)
MembershipClassification		MembershipClassification		VARCHAR(12)
MembershipHasUnlimitedVisits		MembershipHasUnlimitedVisits		BOOL
MembershipMaxVisits			MembershipMaxVisits			INT(6)
MembershipVisitsCount			MembershipVisitsCount			INT(6)
MembershipVistisRemaining		MembershipVisitsRemaining		INT(6)
FirstName				FirstName				VARCHAR(32)
LastName				LastName				VARCHAR(32)
EmailSubscriptions->
	Email				SUBEmail				VARCHAR(64)
	EmailVerificatioNStatus		SUBEmailVerify				VARCHAR(16)
	EmailSubscriptions ->
		Credit Card Notify	SUBCreditCardNotify			BOOL
		Email Campaign		SUBEmailCampaign			BOOL
		Session Reminder	SUBSessionRemind			BOOL
DateOfBirth				DateOfBirth				DATETIME(0)
Gender					Gender					VARCHAR(10)
DateCreated				DateCreated				DATETIME(3)	-- 3 decimal places after seconds
LastModified				LastModified				DATETIME(3)	-- 3 decimal places after seconds
Active					Active					BOOL
City					City					VARCHAR(32)
PostalCode				PostalCode				VARCHAR(16)
PhoneCell				PhoneCell				VARCHAR(20)
AmountDue				AmountDue				DECIMAL(10,2)
CustomFields->
	RFID Card UID			RFID_UID				VARCHAR(20)
	Allow After Hours		After_Hours				BOOL
MemberPhotoUrl				MemberPhotoUrl				VARCHAR(255)

TABLE: MemberEmail
ClientID				BIGINT(12) non-unique Primary Key -- Tied to Members:ClientID (each Client ID may have more than one record)
Email:					VARCHAR(64)
```

Table "MemberEmail" exists strictly because EZF returns emails as an array indicating a member might be able to have more than one email stored in their database. If we could depend on a single email (or not care about anything but the first email), this table could be obviated in favor of a single field in table "Members".

Obviously there's some special processing needed to map the JSON to SQL, but most of it will be straight forward and have a 1:1 direct field name
correlation. In order to provide a robust solution against API changes by EZ Facility (so long as they don't rename fields we are using without
us updating the software), we will ignore any fields in the JSON response that are not present in our schema. We will, however, error out and fall
to pass-thru if any field we consider important is not present as expected. Currently, those fields are:

	ClientID
	LocationID
	AccountNumber
	MembershipNumber
	MemberClassification
	FirstName
	LastName
	Active
	AmountDue
	Allow After Hours
	MemberPhotoUrl

The fields which will require custom attention from the parser are:
	Email (Array)
	SUBEmail
	SUBEmailVerify
	SUBCreditCardNotify
	SUBEmailCampaign
	SUBSessionRemind
		The ones starting with SUB need to be parsed out of the odd JSON hierarchy, but don't appear to
		require a robust Array solution such as Email.
	DateOfBirth
	DateCreated
	LastModified
		These DATETIME() fields require special parsing to translate between JSON and MYSQL as follows:
			JSON: "2021-01-23T12:34:56.123"
			SQL:  "2021-01-23 12:34:56.123" (The T must be removed for SQL and replaced iwht a space)

	All other fields should be a direct pass through.

## Questions or comments to be investigated and resolved
JBS: I think there is a lot of sensitive personal information returned. If we store it then we have a responsibility to protect it while at-rest. I'd prefer to only store the fields we will need, or think we might need. No need for phone number, email, pronoun, address, etc. 

JBS: The cache is updated periodically. If the data changes between updates it will be old. I'm not too concerned about granting access to someone who does not qualify, but I would hate to deny a valid, current member. The vast majority of client info transactions will result in a correct, positive access to the resource. One strategy is to go back to the CRM for current information if the result is to deny access. Of course the cache does not know what the end device is evaluating so it does not know if the result was to deny. I suggest adding an API to the cache that can be called from an end device to say "I just denied access based on client info". The cache would then go back to the CRM system to get current data on that clientID. The next time the cache is queried it will return the latest information; this would happen in less than a second. The user experience would be "I just got my membership today but check-in has denied me! Oh, I just tried again and it worked!" After that it will always work. 

JBS: We do get charged by EZF for each API call to the CRM. If the call to get recent changes requires 10 "get next page" API calls, that might count as 10 API events. Not sure this is a problem, we just need to make sure.

JBS: We'll need to address how this system is backed up - if at all. Maybe no need for that.
	
JBS: Is there some security on the cache system? Maybe have the front end locked to the public IP of MN?

JBS: I wonder if we should discuss coding styles? What is the anticipated language of implementation? We use .PHP for a lot today, will that work? Is there an admin interface to this? If so, a separation of style, layout, and code into different files? Do we put business logic into stored procedures when possible? Maybe a lot of other questions I haven't thought of.
