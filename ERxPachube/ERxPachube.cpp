/*
ERxPachube.cpp - Pachube library for Arduino. 
Copyright (c) 2011 Jeffrey Sun.  All right reserved.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ERxPachube.h"
//#include <stdio.h> // for function sprintf
#include <stdlib.h> // For function dtostrf
//#include <EthernetClient.h> // EthernetClient
#include <Ethernet.h>
#include <avr/pgmspace.h>

//#define ENABLE_DEBUG_MESSAGE // Define it if want to print debug message

/*********************Global variable***************************/

// Define the time how long we wait the response of the server.
// If exceed this time, we assume we won't get the response anyway.
#define HTTP_WAITING_TIMEOUT 4000 // milliseconds, 

static String dummyString; // Used to return the dummy string reference.

byte remoteServer[] = { 173,203,98,29 };            // api.pachube.com
EthernetClient localClient;
//Ethernet localClient;
//Client localClient(remoteServer, 80);

#define bufferSIZE 128
char pDataBuffer[bufferSIZE];

char pHttpBegin[]  = "HTTP/1.1";
/******************Save the static strings in program memory***********/
// HTTP header
//
char http_get_header[] PROGMEM = "GET /v2/feeds/" ;
// feed id
char http_host_key_header[] PROGMEM = ".csv HTTP/1.1\nHost: api.pachube.com\nX-PachubeApiKey: " ;
// API key
// \n\n

char http_put_header[] PROGMEM = "PUT /v2/feeds/" ;
// feed id
// http_host_key_header
// API key
char http_content_header[] PROGMEM = "\nContent-Type: text/csv\nContent-Length: " ;
char http_connection_header[] PROGMEM = "\nConnection: close\n\n" ;


// Debug message
// 
char debug_getHttpStatusCodeFromHeader[] PROGMEM = "getHttpStatusCodeFromHeader" ;
char debug_waitForRepsonse[] PROGMEM = "Waiting for response." ;
char debug_timeout[] PROGMEM = "Time out" ;
char debug_success[] PROGMEM = "SUCCESS" ;
char debug_fail[] PROGMEM = "FAIL" ;
char debug_error_data_stream_length[] PROGMEM = "ERROR data stream length: " ;
char debug_data_start[] PROGMEM = "Data start from the empty line above" ;
char debug_find_r[] PROGMEM = "Find \\r" ;
char debug_find_n[] PROGMEM = "Find \\n" ;
char debug_calculate_http_content_length[] PROGMEM = "Calculate http content length:" ;

/*********************Debug util***************************/
#ifdef ENABLE_DEBUG_MESSAGE

void loadStringFromProgramMemory(char* pBuffer, const char* pProgMemAddr, unsigned int size);

template<class T>
void DEBUG_PRINT(T message) 
{
#ifdef ENABLE_DEBUG_MESSAGE
	Serial.print(message); 
#endif
}

template<class T>
void DEBUG_PRINT_LN(T message) 
{
#ifdef ENABLE_DEBUG_MESSAGE
	Serial.println(message); 
#endif
}

template<>
void DEBUG_PRINT(const String& message) 
{
#ifdef ENABLE_DEBUG_MESSAGE
	String tmpStr = message;

	tmpStr.toCharArray(pDataBuffer, bufferSIZE);

	Serial.print(pDataBuffer); 
#endif
}

template<>
void DEBUG_PRINT_LN(const String& message) 
{
#ifdef ENABLE_DEBUG_MESSAGE
	String tmpStr = message;

	tmpStr.toCharArray(pDataBuffer, bufferSIZE);

	Serial.println(pDataBuffer); 
#endif
}

void DEBUG_PRINT_LN_p(const char* pProgMemAddr) 
{
#ifdef ENABLE_DEBUG_MESSAGE
	loadStringFromProgramMemory(pDataBuffer, pProgMemAddr, bufferSIZE);

	Serial.println(pDataBuffer); 
#endif
}

void DEBUG_PRINT_p(const char* pProgMemAddr)
{
#ifdef ENABLE_DEBUG_MESSAGE
	loadStringFromProgramMemory(pDataBuffer, pProgMemAddr, bufferSIZE);

	Serial.print(pDataBuffer);
#endif
}
#else
// define the empty macros
#define DEBUG_PRINT(message)
#define DEBUG_PRINT_LN(message)
#define DEBUG_PRINT_p(pProgMemAddr)
#define DEBUG_PRINT_LN_p(pProgMemAddr)
#endif


/*********************Util function***************************/

// Should we use "char PROGMEM*" or "char*"?
void loadStringFromProgramMemory(char* pBuffer, const char* pProgMemAddr, unsigned int size)
{
	// Use strlcpy_P to avoid overflow.
	// size_t strlcpy_P 	( 	char *  	dst,	PGM_P  	,		size_t  	siz	 		) 
	// Copy src to string dst of size siz. At most siz-1 characters will be copied. Always NULL terminates (unless siz == 0).
	// The strlcpy_P() function returns strlen(src). If retval >= siz, truncation occurred. 
	// See http://89.129.106.89/COPIA%20ANDRESITO/arduino-0011/hardware/tools/avr/share/doc/avr-libc/avr-libc-user-manual/group__avr__pgmspace.html
	strlcpy_P(pBuffer, pProgMemAddr, size);
}

int getHttpStatusCodeFromHeader(char* pFirstLineOfHttpHeader);

// Read a line from the data buffer. 
// Return value : length of the line. (not counting the '\0')
unsigned int getline(Client& client, char *buffer, unsigned int bufsize){

	unsigned int length = 0;
	char c;
	while (client.available()) {
		c = client.read();
		if (length < bufsize - 1){ // The last one is for '\0'
			buffer[length] = c;
			length ++;
		}
		else
		{
			break; // No buffer available
		}

		if(length >= 2)
		{
			// Note: we can NOT use && here. In some cases, there is only an '\n'.
			if(buffer[length-2] == '\r' || buffer[length-1] == '\n') 
				break; // Line finish
		}
	}

	buffer[length] = '\0';
	return length;
}

// Return the http status code
int waitForRepsonse(Client& client){

	// Wait for response.
	int i = 0;
	DEBUG_PRINT_LN_p(debug_waitForRepsonse/*"Waiting for response."*/);
	while(!localClient.available())
	{
		if(i%10 == 0)
		{
			DEBUG_PRINT(i);
			if(i%100 == 0)
				DEBUG_PRINT('\n');
			else
				DEBUG_PRINT(',');
		}
		delay(1);
		i++;
		if(i > HTTP_WAITING_TIMEOUT)
		{
			DEBUG_PRINT_LN_p(debug_timeout/*"time out"*/);
			return 2;
		}
	}
	DEBUG_PRINT('\n');

	unsigned int bufferSize = 50;
	char pBuffer[bufferSize];

	int httpStatusCode = 0; // No http status code.

	while (localClient.available()) {
		unsigned int length = getline(localClient, pBuffer, bufferSize);
		DEBUG_PRINT(pBuffer);

		if(0 == httpStatusCode)
		{
			if( length < 8)
				continue;

			httpStatusCode = getHttpStatusCodeFromHeader(pBuffer);

			if(200 == httpStatusCode) 
				DEBUG_PRINT_LN_p(debug_success/*"SUCCESS"*/);				
			else
				DEBUG_PRINT_LN_p(debug_fail/*"FAIL"*/);

			break;
		}
	} 

	return httpStatusCode;	
}

template<class T>
void sendToServer(T data)
{
	localClient.print(data); 
	DEBUG_PRINT(data);
}

template<>
void sendToServer(const String& data)
{
	String& tmpData = const_cast<String&>(data);
	
	tmpData.toCharArray(pDataBuffer, bufferSIZE);
	sendToServer(pDataBuffer);
}

void sendToServer_p(const char* pProgMemAddr)
{
	//loadStringFromProgramMemory(pDataBuffer, pStr);
	loadStringFromProgramMemory(pDataBuffer, pProgMemAddr, bufferSIZE);
	sendToServer(pDataBuffer); 
}

/*
The first line of the http header is like:
HTTP/1.1 200 OK
HTTP/1.1 404 Not Found

--------------------------------------------

The common status codes returned from Pachube server is documented in section HTTP Status Codes 
at http://api.pachube.com/#http-status-codes

200 OK: request processed successfully.
401 Not Authorized: either you need to provide authentication credentials, or the credentials provided aren't valid.
403 Forbidden: Pachube understands your request, but refuses to fulfill it. An accompanying error message should explain why.
404 Not Found: either you're requesting an invalid URI or the resource in question doesn't exist (eg. no such feed).
422 Unprocessable Entity: Pachube was unable to create a feed because the EEML/JSON was not complete/valid (e.g. it didn't include a "title" element).
500 Internal Server Error: Something went wrong... Please post to the forum about it and we will investigate.
503 No server error: usually occurs when there are too many requests coming into Pachube - if you get this from an API request then the error message will be returned in XML in the response.
*/
int getHttpStatusCodeFromHeader(char* pFirstLineOfHttpHeader)
{

	DEBUG_PRINT_LN_p(debug_getHttpStatusCodeFromHeader/*"getHttpStatusCodeFromHeader"*/);

	char* pReferenceChar = pHttpBegin;    
	char* pIter = pFirstLineOfHttpHeader;

	for( ; *pReferenceChar != '\0' && *pIter != '\0'; ++pIter, 
		++pReferenceChar)
	{
		if(*pReferenceChar != *pIter)
			return 0; // Not match
	}

	if('\0' == *pIter)
		return 0; // The input string is too short.

	while(*pIter == ' ') // Remove the leading space.
		pIter++;

	int httpStatusCode = atoi(pIter);  // Get the first integer.
	return httpStatusCode;    
}

/*********************HackedShareString***************************/
// This class is to avoid the unnecessary string copy and memory reallocation.
// This string is just a reference to an existing string memory.
class HackedShareString : public String
{
public:
	HackedShareString();
	HackedShareString(char* pStr);
	~HackedShareString();

	void aquire(char* pStr ); // We don't acquire string literal. So the parameter can't be const.

};

HackedShareString::HackedShareString()
{
	free(buffer);
	buffer = NULL;
}

HackedShareString::HackedShareString(char* pStr)
{
	free(buffer); // Free the owned memory.
	buffer = NULL;

	aquire(pStr);
}

HackedShareString::~HackedShareString()
{
	buffer = NULL; // avoid the base class release it.
}

void HackedShareString::aquire(char* pStr )
{
	// Share the same memory with the input String
	buffer = pStr;
	capacity = len = strlen(pStr);
}

HackedShareString sHackedShareString;

/*********************ERxPachube***************************/

ERxPachube::ERxPachube(const char* APIKey, unsigned int feedId) 
: mAPIkey(APIKey), mFeedId(feedId), mDatasteamLength(0)
{

}

unsigned int ERxPachube::countDatastreams() const
{
	if(mDatasteamLength > MAX_DATASTREAM_NUM)
	{
		DEBUG_PRINT_p(debug_error_data_stream_length/*"ERROR data stream length: "*/);
		DEBUG_PRINT_LN(mDatasteamLength);
		return MAX_DATASTREAM_NUM;
	}
	return mDatasteamLength;
}

const String& ERxPachube::getValueString(unsigned int id) const
{
	int index = getIndex(id);

	if(index != -1)
		return mDataStreams[index].mValue;

	return dummyString;
}

int ERxPachube::getValueInt(unsigned int id) const
{
	const String& tmpConstStr = getValueString(id); 
	String& tmpStr = const_cast<String&>(tmpConstStr);

	// Library Defect: This function doesn't change the string. This function in the library should be const.
	// So we won't need the cast above then.
	tmpStr.toCharArray(pDataBuffer, bufferSIZE);
	int iValue = atoi(pDataBuffer); 
	
	return iValue;
}

float ERxPachube::getValueFloat(unsigned int id) const
{
	const String& tmpConstStr = getValueString(id); 
	String& tmpStr = const_cast<String&>(tmpConstStr);

	tmpStr.toCharArray(pDataBuffer, bufferSIZE); 
	float fValue = atof(pDataBuffer);

	return fValue;
}

unsigned int ERxPachube::getIdByIndex(unsigned int index) const
{
	if(index < countDatastreams())
		return mDataStreams[index].mId;

	return INVALID_DATA_STREAM_ID;
}

const String& ERxPachube::getValueByIndex(unsigned int index) const
{
	if(index < countDatastreams())
		return mDataStreams[index].mValue;

	return dummyString;
}

const String& ERxPachube::getAPIKey() const
{
	return mAPIkey;
}

unsigned int ERxPachube::getFeedId() const
{
	return mFeedId;
}

bool ERxPachube::addData(unsigned int id)
{
	pDataBuffer[0]='\0';
	sHackedShareString.aquire(pDataBuffer);
	return addData(id, sHackedShareString);
}

bool ERxPachube::addData(unsigned int id, const String& value)
{
	int index = getIndex(id);

	if(index != -1) // Already exists
	{
		mDataStreams[index].mValue = value; 
		return true;
	}
	else
	{
		if(mDatasteamLength < MAX_DATASTREAM_NUM)
		{
			mDataStreams[mDatasteamLength].mId = id;
			mDataStreams[mDatasteamLength].mValue = value; 
			mDatasteamLength++;
			return true;
		}
		else
			return false;
	}
}

bool ERxPachube::updateData(unsigned int id, const String& value)
{
	int index = getIndex(id);

	if(index != -1)
	{
		mDataStreams[index].mValue = value; 

		return true;
	}

	return false;
}

bool ERxPachube::updateData(unsigned int id, int value)
{ 
	itoa((signed long)value, pDataBuffer, 10);

	return updateDataHelper(id, pDataBuffer);
}

bool ERxPachube::updateData(unsigned int id, float value)
{
	// The sprintf function doesn't work. When value = 11.5, the converted string is 0.
	//sprintf(pDataBuffer,"%d",value);
	dtostrf(value, 2, 6, pDataBuffer);
	return updateDataHelper(id, pDataBuffer);
}

int ERxPachube::getIndex(unsigned int id) const
{
	for(unsigned int i = 0; i < mDatasteamLength; i++)
	{
		if(id == mDataStreams[i].mId)
			return i;
	}

	return -1;
}

void ERxPachube::clearData()
{
	mDatasteamLength = 0;
}

bool ERxPachube::updateDataHelper(unsigned int id, char* value)
{
	sHackedShareString.aquire(value);

	updateData(id, sHackedShareString);

}

/*******************ERxPachubeDataOut*****************************/

ERxPachubeDataOut::ERxPachubeDataOut(const char* APIKey, unsigned int feedId)
:ERxPachube(APIKey, feedId)
{

}

// API V2
/*
Http request header
--------------------------------------------------
PUT /v2/feeds/23408.csv HTTP/1.1
Host: api.pachube.com
X-PachubeApiKey: PACHUBE_API_KEY
User-Agent: Arduino (Pachube Out)
Content-Type: text/csv
Content-Length: xxx
Connection: close

1,123
2,456
<stream_id>,<value>
--------------------------------------------------
The data structure is documented in section Read feed: GET /v2/feeds/<feed_id> in the API doc (v2)
http://api.pachube.com/v2/#update-feed-put-v2-feeds-feed-id


------------------------------------------------------------
Create data file with the three lines below. Name it c:\SensorData.txt
0,4
1,5
2,6

Use the curl command below can update the server data.
curl --request PUT --data-binary @C:\SensorData.txt --header "X-PachubeApiKey:PACHUBE_API_KEY" --header "User-Agent: Arduino (Pachube)" --header  "Content-Type: text/csv" --header "Content-Length: 13" --header "Connection: close" "http://api.pachube.com/v2/feeds/23408.csv"
*/
int ERxPachubeDataOut::updatePachube() const
{
	if(countDatastreams() == 0)
		return 3;

	if(!(localClient.connect(remoteServer, 80) > 0))
		return 1;

	DEBUG_PRINT_LN_p(debug_calculate_http_content_length/*"Calculate http content length:"*/);
	unsigned int httpContentLength = 0;
	unsigned int dataLength = countDatastreams();
	for(unsigned int i = 0; i < dataLength; i++)
	{
		unsigned int id = getIdByIndex(i);
		ultoa((unsigned long)id, pDataBuffer, 10);
		httpContentLength += strlen(pDataBuffer);

		httpContentLength += getValueByIndex(i).length();

		httpContentLength += 2; // for ',' and '\n'
		DEBUG_PRINT_LN(httpContentLength);
	}

	// Send PUT request. 

	// API v2
	sendToServer_p(http_put_header/*"PUT /v2/feeds/"*/);
	sendToServer(getFeedId());
	sendToServer_p(http_host_key_header/*".csv HTTP/1.1\nHost: api.pachube.com\nX-PachubeApiKey: "*/);
	sendToServer(getAPIKey());
	//sendToServer(("\nUser-Agent: Arduino (Pachube Out)"));
	sendToServer_p(http_content_header/*"\nContent-Type: text/csv\nContent-Length: "*/);
	sendToServer(httpContentLength);
	sendToServer_p(http_connection_header/*"\nConnection: close\n\n"*/);


	// Send content
	for(unsigned int i = 0; i < dataLength; i++)
	{
		sendToServer(getIdByIndex(i));
		sendToServer(',');
		sendToServer(getValueByIndex(i));
		sendToServer('\n');
	}

	// Wait for response.
	int httpStatusCode = waitForRepsonse(localClient);

	localClient.stop();

	// Return the result.
	return httpStatusCode;
}

/*******************ERxPachubeDataIn*****************************/

ERxPachubeDataIn::ERxPachubeDataIn(const char* APIKey, unsigned int feedId)
: ERxPachube(APIKey, feedId)
{
}

// API v1
/*
Http request header
--------------------------------------------------
GET /api/23408.csv HTTP/1.1
Host: pachube.com
X-PachubeApiKey: PACHUBE_API_KEY
User-Agent: Arduino (Pachube In)
<Include a empty line>
--------------------------------------------------
Returned data format

--------------------------------------------------
HTTP/1.1 200 OK
Date: Thu, 21 Apr 2011 13:21:11 GMT
Content-Type: text/plain; charset=utf-8
Connection: keep-alive
Last-Modified: Thu, 21 Apr 2011 11:20:00 GMT
Content-Length: 115
Age: 0
Vary: Accept-Encoding

246,249,2011-4-21 11:20:0
--------------------------------------------------
*/

// API v2
/*
Http request header
--------------------------------------------------
GET /v2/feeds/23408.csv HTTP/1.1
Host: api.pachube.com
X-PachubeApiKey: PACHUBE_API_KEY
User-Agent: Arduino (Pachube In)
<Include a empty line>
--------------------------------------------------

Returned data format

--------------------------------------------------
HTTP/1.1 200 OK
Date: Thu, 21 Apr 2011 13:21:11 GMT
Content-Type: text/plain; charset=utf-8
Connection: keep-alive
Last-Modified: Thu, 21 Apr 2011 11:20:00 GMT
Content-Length: 115
Age: 0
Vary: Accept-Encoding

0,2011-04-21T11:20:00.989722Z,246
1,2011-04-21T11:20:00.989722Z,249
2,2011-04-21T11:20:00.989722Z,2011-4-21 11:20:0
<stream_id>,<retrieved_at>,<value>
--------------------------------------------------
The data structure is documented in section Read feed: GET /v2/feeds/<feed_id> in the API doc (v2)
http://api.pachube.com/v2/#read-feed-get-v2-feeds-feed-id
*/	
// Use curl to debug the http response . If -I option is included, it will show the http header. Otherwise, show the body.
// curl --request GET --header "X-PachubeApiKey:PACHUBE_API_KEY" --header "User-Agent: Arduino (Pachube In)" "http://api.pachube.com/v2/feeds/23408.csv" -I
// curl --request GET --header "X-PachubeApiKey:PACHUBE_API_KEY" --header "User-Agent: Arduino (Pachube In)" "http://api.pachube.com/v2/feeds/23408.csv" 

// 0 - unknown status
// 1 - can't connect to server
// 2 - time out
// 
int ERxPachubeDataIn::syncPachube()
{
	if(!(localClient.connect(remoteServer, 80) > 0))
		return 1;

	clearData();

	// Send GET request.

	// API v1
	//String httpHeader("GET /api/");
	//httpHeader += getFeedId();
	//httpHeader += (".csv HTTP/1.1\nHost: pachube.com\nX-PachubeApiKey: ");
	//httpHeader += getAPIKey();
	//httpHeader += ("\nUser-Agent: Arduino (Pachube In)\n\n");

	//unsigned int bufferSize = httpHeader.length() + 1;
	//char pBuffer[bufferSize];
	//httpHeader.toCharArray(pBuffer, bufferSize);
	//localClient.print(pBuffer);  

	// String Defect: If cache all the data into string then send out together, the httpHeader will become empty
	// The logic below works well.

	// API v2
	sendToServer_p(http_get_header/*"GET /v2/feeds/"*/);
	sendToServer(getFeedId());
	sendToServer_p(http_host_key_header/*".csv HTTP/1.1\nHost: api.pachube.com\nX-PachubeApiKey: "*/);
	sendToServer(getAPIKey());
	sendToServer("\n\n");

	// Wait for response.
	int httpStatusCode = waitForRepsonse(localClient);

	if(200 == httpStatusCode )
	{
		// Parse the received data.
		unsigned int bufferSize = 128;
		char pBuffer[bufferSize];
		bool bDataBegin = false;
		while (localClient.available())
		{
			unsigned int length = getline(localClient, pBuffer, bufferSize);
			DEBUG_PRINT(pBuffer);
			
			if(2 == length && false == bDataBegin)
			{
				DEBUG_PRINT_LN_p(debug_data_start/*" Data start from the empty line above"*/);
				bDataBegin = true;
				continue;
			}

			if(bDataBegin)
			{
				HackedShareString strLine;
				strLine.aquire(pBuffer);
				int pos = 0;
				pos = strLine.indexOf(',', pos);
				if(-1 == pos)
					continue;

				//String strID = strLine.substring(0, pos);
				int strIDLength = pos;
				char strID[strIDLength + 1];
				for(int i = 0; i < strIDLength; i++)
					strID[i] = strLine[i];
				strID[strIDLength] = '\0';

				pos = strLine.indexOf(',', pos + 1);
				if(-1 == pos)
					continue;

				//String strValue = strLine.substring(pos + 1, strLine.length());

				int strValueLength = strLine.length() - pos - 1;
				char strValue[strValueLength + 1];
				for(int i = 0; i < strValueLength; i++)
					strValue[i] = strLine[pos + 1 + i];
				strValue[strValueLength] = '\0';


				if('\r' == strValue[strValueLength - 2])
				{
					DEBUG_PRINT_LN(debug_find_r/*"Find \\r"*/);
					strValue[strValueLength - 2] = '\0';
				}
				else if('\n' == strValue[strValueLength - 1])
				{
					DEBUG_PRINT_LN(debug_find_n/*"Find \\n"*/);
					strValue[strValueLength - 1] = '\0';
				}
				
				DEBUG_PRINT_LN(strID);
				DEBUG_PRINT_LN(strValue);

				//strID.toCharArray(pDataBuffer, bufferSIZE);
				int id = atoi(strID);
				strLine.aquire(strValue);
				addData((unsigned int)id, strLine);

				if(countDatastreams() == MAX_DATASTREAM_NUM) // No more space to save the left data.
					break;
			}
		} 
	}	

	localClient.stop();
	
	// Return the result.
	return httpStatusCode;
}
