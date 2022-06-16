/*
 * Twilio SMS and MMS on ESP8266 Example.
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>

#include "twilio.hpp"

// Use software serial for debugging?
#define USE_SOFTWARE_SERIAL 0

// Print debug messages over serial?
#define USE_SERIAL 0

#define timeSeconds 60

// from PIRtest.ino
const int motionSensor = D2;
unsigned long now = millis();
unsigned long lastTrigger = 0;
boolean startTimer = false;

// from twilio_esp8266_arduino_example.ino
const char* ssid = "SBTM";
const char* password = "0nlyB1gPoop3r5";

const char fingerprint[] = "72 1C 17 31 85 E2 7E 0D F9 D4 C2 5B A0 0E BD B7 E2 06 26 ED";
const char* account_sid = "ACc76d8535d59198c9ab05aa5718f3b011";
const char* auth_token = "6d4f4047fe1202cb6db8067114c6ddb6";
String to_number    = "+18477671525";
String from_number = "+12242423024";
String master_number    = "+12242423024";
String media_url = "";

boolean sendMessage = false;

Twilio *twilio;
ESP8266WebServer twilio_server(8000);

#if USE_SOFTWARE_SERIAL == 1
#include <SoftwareSerial.h>
// You'll need to set pin numbers to match your setup if you
// do use Software Serial
extern SoftwareSerial swSer(14, 4, false, 256);
#else
#define swSer Serial
#endif

IRAM_ATTR void detectsMovement() {
  Serial.println("MOTION!!!!!!");
  if (not startTimer) {
    sendMessage = true;
  }
  startTimer = true;
  lastTrigger = millis();
}

void handle_message() {
        #if USE_SERIAL == 1
        swSer.println("Incoming connection!  Printing body:");
        #endif
        bool authorized = false;
        char command = '\0';

        // Parse Twilio's request to the ESP
        for (int i = 0; i < twilio_server.args(); ++i) {
                #if USE_SERIAL == 1
                swSer.print(twilio_server.argName(i));
                swSer.print(": ");
                swSer.println(twilio_server.arg(i));
                #endif

                if (twilio_server.argName(i) == "From" and
                    twilio_server.arg(i) == master_number) {
                    authorized = true;
                } else if (twilio_server.argName(i) == "Body") {
                        if (twilio_server.arg(i) == "?" or
                            twilio_server.arg(i) == "0" or
                            twilio_server.arg(i) == "1") {
                                command = twilio_server.arg(i)[0];
                        }
                }
        } // end for loop parsing Twilio's request

        // Logic to handle the incoming SMS
        // (Some board are active low so the light will have inverse logic)
        String response = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
        if (command != '\0') {
                if (authorized) {
                        switch (command) {
                        case '0':
                                digitalWrite(LED_BUILTIN, LOW);
                                response += "<Response><Message>"
                                "Turning light off!"
                                "</Message></Response>";
                                break;
                        case '1':
                                digitalWrite(LED_BUILTIN, HIGH);
                                response += "<Response><Message>"
                                "Turning light on!"
                                "</Message></Response>";
                                break;
                        case '?':
                        default:
                                response += "<Response><Message>"
                                "0 - Light off, 1 - Light On, "
                                "? - Help\n"
                                "The light is currently: ";
                                response += digitalRead(LED_BUILTIN);
                                response += "</Message></Response>";
                                break;
                        }
                } else {
                        response += "<Response><Message>"
                        "Unauthorized!"
                        "</Message></Response>";
                }

        } else {
                response += "<Response><Message>"
                "Look: a SMS response from an ESP8266!"
                "</Message></Response>";
        }

        twilio_server.send(200, "application/xml", response);
}

void setup() {
  // serial port for debugging purposes:
  Serial.begin(115200);

//  PIR motionSensor mode INPUT_PULLUP
  pinMode(motionSensor, INPUT_PULLUP);
//  set motionSensor pin as interrupt, assign interrupt fxn and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);
  Serial.println("hello!");

  WiFi.begin(ssid, password);
  twilio = new Twilio(account_sid, auth_token, fingerprint);

  #if USE_SERIAL == 1
    swSer.begin(115200);
    while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            swSer.print(".");
    }
    swSer.println("");
    swSer.println("Connected to WiFi, IP address: ");
    swSer.println(WiFi.localIP());
    #else
    while (WiFi.status() != WL_CONNECTED) delay(1000);
  #endif

  String response;
  String setup_message_body = "PIR sensor is fully setup";
  bool success = twilio->send_message(
          to_number,
          from_number,
          setup_message_body,
          response,
          media_url
  );

  twilio_server.on("/message", handle_message);
  twilio_server.begin();

  #if USE_SERIAL == 1
    swSer.println(response);
  #endif
}

void loop() {
  if(sendMessage) {
      String response;
      String message_body = "Motion detected by PIR sensor";
      bool success = twilio->send_message(
        to_number,
        from_number,
        message_body,
        response,
        media_url
      );
      sendMessage = false;
  }
  now = millis();
  if(startTimer && (now - lastTrigger > (timeSeconds*1000))) {
    Serial.println("Motion stopped...");
    startTimer = false;
  }
  twilio_server.handleClient();
}
