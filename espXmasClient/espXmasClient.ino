#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#define NUM_OF_RELAYS 3
#define MULT_CONST 1000

int relayArray[3] = {5,4,14};//pin (on esp) that each signal wire is connected to
int statusArray[3] = {'0','0','0'}; //current status of each module, init  all to off
int delayInSec = 10; // delay between requests , default 10 seconds

//esp wifi config

// ****** EDIT THE FOLLOWING ******
const char* ssid = "..";	// ssid, name of the access point
const char* password = "..";	// password of the access point

const char* host = "..";	// without https:// part
// for heroku : "NameOfYourApp.herokuapp.com",eg app name: ieeeuoi,  host="ieeeuoi.herokuapp.com"
const int httpsPort = 443;
String url = "/arduino";	// endpoint that accepts get requests, is relative url

// SHA1 fingerprint of our servers certificate 
//eg for our heroku app 08:3B:71:72:02:43:6E:CA:ED:42:86:93:BA:7E:DF:81:C4:BC:62:30
const char* fingerprint = "08 3B 71 72 02 43 6E CA ED 42 86 93 BA 7E DF 81 C4 BC 62 30";

// ****** DONT EDIT AFTER HERE ******




//parse server response
void handleResp(String server_resp){
	// 4 parts, contains 3*','
	int date_end = server_resp.indexOf(",");
	int num_end = server_resp.indexOf(",", date_end+1);
	int payload_end = server_resp.indexOf(",", num_end+1);


	String date_Str = server_resp.substring(0, date_end); // unix timestamp of when the server received the request 
	String len_Str  = server_resp.substring(date_end+1, num_end);	// len of status String

	String relaymsg_Str = server_resp.substring(num_end+1, payload_end); // status String to pass to relay modules
	String delay_Str = server_resp.substring(payload_end+1); // delay between requests
	


	delayInSec = delay_Str.toInt(); // update delay var
	passToRelay(relaymsg_Str);	// activate/dectivate the lights
}



// activates/deactivates the lights according to the status we received from the server
void passToRelay(String statFromServer){// string containing only 0,1 chars
	int real_len = statFromServer.length();
	char curr_stat = ' ';
	char new_stat = ' ';
	int mpin = 0;

	if(real_len > NUM_OF_RELAYS){ // we use 3 relays, ignore other chars if exist
		real_len = NUM_OF_RELAYS;
	}

	for(int i= 0; i<real_len; i++){
		// for each relay module
		curr_stat = statusArray[i];


		char myChar = statFromServer.charAt(i);
		// maybe extra
		if(myChar != '0' && myChar != '1'){
			new_stat = '?';
		}else{
			new_stat = myChar;
		}
	

		mpin = relayArray[i];


		if (new_stat != curr_stat){
			// run only if diff status
			if(new_stat == '0'){
				// 0 == deactivate
				digitalWrite(mpin,LOW);
				statusArray[i] ='0';
			}else if(new_stat == '1'){
				// 1 == activate
				digitalWrite(mpin,HIGH);
				statusArray[i] = '1';
			}else{
				// unknown char do nothing
				Serial.println("Received unknown char ");
			}
		} // status changed, endif


	} //endfor
}


void setup(){
	Serial.begin(115200); // baud rate for esp

	//set esp pins as output
	pinMode(5, OUTPUT);  // relay for group 1 , D1
	pinMode(4, OUTPUT);  // relay for group 2 , D2
	pinMode(14, OUTPUT); // relay for group 3 , D5

/*
	// could be used if we had more relay modules
	for(int j = 0 ; j < NUM_OF_RELAYS; j++){
		pinMode(relayArray[j], OUTPUT);
	}
*/
	//esp wifi init code
	Serial.println();
	Serial.print("connecting to ");
	Serial.println(ssid);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED){
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
}


void loop(){

	WiFiClientSecure client;
	String server_resp = "";	//response from our server, eg "1514152799000,3,000,1"
	
	// Use WiFiClientSecure class to create TLS connection
	Serial.print("connecting to ");
	Serial.println(host);
	if (!client.connect(host, httpsPort)){
		Serial.println("connection failed");
		return;
	}

	if (client.verify(fingerprint, host)){
		Serial.println("certificate matches");
	}else{
		Serial.println("WARNING: certificate doesn't match");
		// because this is a demo, we take no further action , but on
		// production we could call return ,or enter a while true loop
		// to stop the exec

	}

	Serial.print("requesting URL: ");
	Serial.println(url);

	client.print(String("GET ") + url + " HTTP/1.1\r\n" +
		"Host: " + host + "\r\n" +
		"User-Agent: XMAS-ESP8266\r\n" +
		"Connection: close\r\n\r\n");

	Serial.println("request sent");
	while (client.connected()){
		String line = client.readStringUntil('\n');
		if (line == "\r"){
			Serial.println("headers received");
			break;
		}
	}
	
	String line = client.readStringUntil('\n');

/*
	if(line.startsWith("151")){
		Serial.println("esp8266/Arduino CI successfull!");
	} else {
		Serial.println("esp8266/Arduino CI has failed");
	}
*/

	Serial.println("reply was:");
	Serial.println("==========");
	Serial.println(line);
	Serial.println("==========");
	Serial.println("closing connection");


	server_resp = line;
	handleResp(server_resp); // delayInSec updates at handleResp func
	delay(delayInSec*MULT_CONST);
}

