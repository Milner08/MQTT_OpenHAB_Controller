#include <string.h>
#include <FastLED.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include "DHT.h"

/*** Network Config ***/
#define ENABLE_DHCP true
static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
IPAddress ip(192,168,1,220);

/*** MQTT Config ***/
IPAddress broker(192,168,1,206);
const char* statusTopic  = "events";
char messageBuffer[100];
char topicBuffer[100];
char clientBuffer[50];

/*** Temp & Humidity Config ***/
#define DHTPIN 4
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);
unsigned long lastSend;
const char* temperatureTopic = "bedroom/temperature"; // MQTT topic for sensor reporting
const char* humidityTopic    = "bedroom/humidity";    // MQTT topic for sensor reporting

/*** OpenHab Details ***/
const char* openHabControlTopic = "openHAB/in/command/"; // MQTT topic for sensor reporting
const char* openHabStateTopic   = "openHAB/out/state/";  // MQTT topic for sensor reporting
int numItems = 6;
char *items[6];
char *states[6];
char stateHolder[100];

/*** Setup ***/
EthernetClient ethClient;
PubSubClient client(ethClient);

//MQTT Callback
void callback(char* topic, uint8_t* payload, unsigned int length)
{
 //Get the bit of the topic we care about
 topic += (strlen(openHabControlTopic) - 1);

 //Clean up the payload into a friendly char array.
 int i;
 for(i=0; i<length; i++) {
   stateHolder[i] = payload[i];
 } 
 stateHolder[i] = '\0';
 
 Serial.println("Topic is " + String(topic) + ", Payload = " + String(stateHolder));
 for(int i = 0; i < numItems; i++){
    if(strcmp(topic, items[i]) == 0){
      states[i] = strdup(stateHolder);
      Serial.println("Updated state for " + String(items[i]) + ", " + String(states[i]));
    }
 }
}

void setup()
{
  // Open serial communications
  Serial.begin(9600);
  
  if( ENABLE_DHCP == true )
  {
    Ethernet.begin(mac);      // Use DHCP
  } else {
    Ethernet.begin(mac, ip);  // Use static address defined above
  }

  //Setup MQTT Client
  client.setServer(broker ,1883);
  client.setCallback(callback);
  
  Serial.print("My address:");
  Serial.println(Ethernet.localIP());

  //Sensor config
  dht.begin();
  
  // Setup timer for sending updates.
  lastSend = 0;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    
    if (client.connect("arduino_BEDROOM")) 
    {  
      Serial.println("Connected!");
      client.publish(statusTopic,"The bedroom controller arduino is now online");
    } else {
      Serial.println("failed, rc=" + String(client.state()) +" try again in 5 seconds");
      delay(5000);
    }
  }
}

void getItemNames(){
  //This should be downloaded from OpenHAB.
  items[0] = "BedsideLamp_Switch";
  items[1] = "StandingLamp_Dimmer";
  items[2] = "Bedroom_Humidity";
  states[0] = "OFF";
  states[1] = "OFF";
  states[2] = "0";
  numItems = 3;
}

void subscribeToItems(){
  //Subscribe to all items.
  for( int i = 0; i < numItems; i++){
    char topic[sizeof(openHabStateTopic) + sizeof(items[i]) + 1];
    sprintf(topic,"%s%s", openHabStateTopic, items[i]);
    client.subscribe(topic);
  }
}

void loop()
{
  if ( !client.connected() ) {
    reconnect();
    getItemNames();
    subscribeToItems();
  }

  // Every second send an update for sesnors
  if ( millis() - lastSend > 1000 ) { 
    getAndSendTemperatureAndHumidityData();
    lastSend = millis();
  }

  client.loop();
}

void getAndSendTemperatureAndHumidityData()
{
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  char tempC[10];
  dtostrf(temperature,6,2,tempC);
  char relH[10];
  dtostrf(humidity,6,2,relH);

  Serial.println("Publishing new temp and humidity values! Temp = " + String(temperature, 2) 
  +", Humidity = " + String(humidity, 2));

  client.publish(temperatureTopic, tempC);
  client.publish(humidityTopic, relH);
}

