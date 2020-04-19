 /* 
  WiFiTelnetToSerial - Example Transparent UART to Telnet Server for esp8266
  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WiFi library for Arduino environment.
 
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  visit 
  http://www.techiesms.com
  for IoT project tutorials.
  
        #techiesms
  explore | learn | share
  
*/

/* Coded by Udara Viduranga Avinash, Sri Lanka.
   Several other codes were used to create this(Hats off to them!!!!).
   free for anyone*/

#include <ESP8266WiFi.h>
#include <WifiUDP.h>
#include <String.h>
#include <Wire.h>
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <stdlib.h>

#include "SevenSegmentTM1637.h"
#include "SevenSegmentExtended.h"

#include <OneWire.h>
#include <DallasTemperature.h>

// Define NTP properties
#define NTP_OFFSET   60 * 60           // In seconds
#define NTP_INTERVAL 60 * 1000        // In miliseconds
#define NTP_ADDRESS  "pool.ntp.org"  // change this to whatever pool is closest (see ntp.org)


//how many clients should be able to telnet to this ESP8266,In this case it is 2 nos.You can change it as you want.
#define MAX_SRV_CLIENTS 2
const char* ssid = "<your-ssid>";
const char* password = "<your-passwd>";

WiFiServer server(23); // --> default port for communication usign TELNET protocol | Server Instance
WiFiClient serverClients[MAX_SRV_CLIENTS]; // --> Client Instanse

// Set up the NTP UDP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);


// to display time on Serial Monitor
String date;


// to display temparature on 4-digit LED Display
String TEMP;

// as required for time,date,AM/PM 
const char * days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"} ;
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"} ;
const char * ampm[] = {"AM", "PM"} ;

// to display time on 4-digit LED Display
byte t1 ; 
byte t2 ;

String t3=""; 

float tempC;

const byte PIN_CLK = D6;   // define CLK pin (any digital pin) for 4-digit LED Display
const byte PIN_DIO = D5;   // define DIO pin (any digital pin) 4-digit LED Display

SevenSegmentExtended      display(PIN_CLK, PIN_DIO);


#define ONE_WIRE_BUS D4

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);


void NewClient_Check();
void ClientData_Check();
void SerialData_Check();
void SendData();
void Internet_Time();
void Temp();

uint8_t i = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  timeClient.begin();   // Start the NTP UDP client

  Serial.print("WiFi_TelNet_to_Serial_Internet_time_Temparature_Logging_V1.0");

  display.begin();            // initializes the display
  display.setBacklight(100);  // set the brightness to 100 %(working levels are 25%,50%,75% and 100%)
  display.print("INIT");      // display "INIT" on the display
  delay(1000);                // wait 1000 ms
  

  // -->Try to connect to particular host for 20 times, If still not connected then automatically resets.
  Serial.print("\nConnecting to "); Serial.println(ssid);
  
  while (WiFi.status() != WL_CONNECTED && i++ < 20){ 
         delay(500);
         Serial.print(".");
         display.setBacklight(75);  // set the brightness to 75 %
         display.print("8888");     // display "8888" on the display
         display.blink();           // blink "8888"
         if(i == 21){
                    Serial.print("Could not connect to"); Serial.println(ssid);
                    while(1);
                    }
         }
  //start UART and the server
  Serial.begin(115200);
  server.begin();
  server.setNoDelay(true); // --> Won't be storing data into buffer and wait for the ack. rather send the next data and in case nack is received, it will resend the whole data
  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");

  Serial.println("");
  display.clear();
  display.print("OK");      // display "Ok" on the display
  display.blink();          // blink "Ok", this means that ESP8266 is connected to Wifi network
  delay(1000);

  sensors.begin();         // Initiation of temparature sensor Dallas/Maxim Ds18B20
}

void loop() {
          NewClient_Check();
          ClientData_Check();
          SerialData_Check();
          Internet_Time();
          Temp();
          SendData();
          delay(50000);
          }
          
void NewClient_Check(){
                     //check if there are any new clients
                      if (server.hasClient()){
                        for(i = 0; i < MAX_SRV_CLIENTS; i++){
                                //find free/disconnected spot
                                if (!serverClients[i] || !serverClients[i].connected()){
                                            if(serverClients[i]) serverClients[i].stop();
                                            serverClients[i] = server.available();
                                            Serial.print("New client: "); Serial.println(i);
                                            continue;
                                    }
                            }
                        //no free/disconnected spot so reject
                        WiFiClient serverClient = server.available();
                        serverClient.stop();
                      }
  
}


void ClientData_Check(){
                      //check clients for data
                        for(i = 0; i < MAX_SRV_CLIENTS; i++){
                                if (serverClients[i] && serverClients[i].connected()){
                                  if(serverClients[i].available()){
                                    //get data from the telnet client and push it to the UART
                                    while(serverClients[i].available()) Serial.write(serverClients[i].read());
                                  }
                                }
                        }
  
}


void SerialData_Check(){
                      //check UART for data
                        if(Serial.available()){
                          size_t len = Serial.available();
                          uint8_t sbuf[len];
                          Serial.readBytes(sbuf, len);
                          //push UART data to all connected telnet clients
                          for(i = 0; i < MAX_SRV_CLIENTS; i++){
                              if (serverClients[i] && serverClients[i].connected()){
                                serverClients[i].write(sbuf, len);
                                delay(1);
                            }
                          }
                        }
  
}


void SendData(){
//push Sensor and Time data to all connected telnet clients
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (serverClients[i] && serverClients[i].connected()){
          // Time
          char Time_1[11];
          t3.toCharArray(Time_1,11);
          uint8_t s3_buf[8];
          memcpy(s3_buf,&Time_1,11);
          serverClients[i].write(s3_buf,11);
          delay(1);
          serverClients[i].write(",");
          serverClients[i].flush();
          delay(10);
          // Temparature
          char Temp[5];
          dtostrf(tempC,5, 2, Temp);
          uint8_t s2_buf[sizeof(Temp)];
          memcpy(s2_buf,&Temp,sizeof(Temp));
          serverClients[i].write(s2_buf,sizeof(Temp));
          serverClients[i].flush();
          serverClients[i].write("°C");
          delay(1);
          serverClients[i].write("\n");
          serverClients[i].flush();
          delay(10);
          }
    }
}

void Internet_Time(){
          // Get the time from time servers
          if (WiFi.status() == WL_CONNECTED) //Check WiFi connection status
            {   
              date = "";  // clear the variables
              t3 = "";
              // update the NTP client and get the UNIX UTC timestamp 
              timeClient.update();
              unsigned long epochTime =  timeClient.getEpochTime();
          
              // convert received time stamp to time_t object
              time_t local, utc;
              utc = epochTime;
          
              // Then convert the UTC UNIX timestamp to local time
              TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, +270};  //UTC - 5 hours - change this as needed
              //TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, +330};   //UTC - 6 hours - change this as needed
              Timezone usEastern(usEDT);
              //Timezone usEastern(usEDT, usEST);
              local = usEastern.toLocal(utc);
          
              // now format the Time variables into strings with proper names for month, day etc
              date += days[weekday(local)-1];
              date += ", ";
              date += months[month(local)-1];
              date += " ";
              date += day(local);
              date += ", ";
              date += year(local);
              
              //to display time on 4-digit LED Display
              t1 = hourFormat12(local);
              t2 = minute(local);
              
              // format the time to 12-hour format with AM/PM with seconds
              t3 += hourFormat12(local);
              t3 += ":";
              if(minute(local) < 10)  // add a zero if minute is under 10
                t3 += "0";
              t3 += minute(local);
              t3 += ":";
              if(second(local) < 10)  // add a zero if second is under 10
                t3 += "0";
              t3 += second(local);
              t3 += " ";
              t3 += ampm[isPM(local)];
              
           }
        // display time on 4-digit LED Display 
        display.clear();
        display.printTime(t1, t2, true);  
        delay(2000);
        display.clear();
        
        // Display the date and time on Serial Monitor
        Serial.println("");
        Serial.print("Local date: ");
        Serial.print(date);
        Serial.println("");
        Serial.print("Local time: ");
        Serial.println(t3);
    
        delay(3000);    //Send a request to update every 03 sec (= 3,000 ms)

}


void Temp(){
          //get temperatures from sensor
          String TEMP="";
          Serial.print("Requesting temperatures...");
          sensors.requestTemperatures(); // Send the command to get temperatures
          Serial.println("DONE");
          // After we got the temperatures, we can print them here.
          // We use the function ByIndex, and as an example get the temperature from the first sensor only.
           tempC = sensors.getTempCByIndex(0);
        
          // Check if reading was successful
          if(tempC != DEVICE_DISCONNECTED_C) 
          {
            Serial.print("Temperature for the device 1 (index 0) is: ");
            Serial.println(tempC);
            display.clear();
            
            // Create a string to display temparature on 4-digit LED Display
            float tempC1 = tempC-int(tempC);
            TEMP += int(tempC);
            TEMP += "c";
            TEMP += int((tempC1+0.05)*10);// Accurate to one decimal place
            
            display.print(TEMP);
            display.blink();    // blink temparature value (current)
          
            delay(2000);
            display.clear();
          } 
          else
          {
            Serial.println("Error: Could not read temperature data");
          }
        

  
}
