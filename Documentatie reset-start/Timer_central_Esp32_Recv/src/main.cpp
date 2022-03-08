#include "WiFi.h"
#include "PubSubClient.h" //pio lib install "knolleary/PubSubClient"

#define SSID          "NETGEAR68"
#define PWD           "excitedtuba713"

#define MQTT_SERVER   "192.168.1.20"
#define MQTT_PORT     1883

#define LED_PIN  2

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void callback(char *topic, byte *message, unsigned int length);

void setup_wifi()
{
  delay(10);
  Serial.println("Connecting to WiFi..");

  WiFi.begin(SSID, PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{

  Serial.begin(115200);

  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  pinMode(LED_PIN, OUTPUT);
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
   
  Serial.print(". Message: ");
  String messageTemp;

  
  
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
   
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == "esp32/output")
  {
    Serial.print("Changing output to ");
    if (messageTemp == "on")
    {
      Serial.println("on");
      digitalWrite(LED_PIN, HIGH);
    }
    else if (messageTemp == "off")
    {
      Serial.println("off");
      digitalWrite(LED_PIN, LOW);
    }
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Client"))
    {
      Serial.println("connected");
      // Subscribe
      client.subscribe("tag/#");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000)
  {
    lastMsg = now;
  }

  Serial.println("Waiting on input on esp32/output");
  delay(50000);
  
}


/*
// 


#include "WiFi.h"
#include "PubSubClient.h" //pio lib install "knolleary/PubSubClient"

#define SSID          "NETGEAR68"
#define PWD           "excitedtuba4"

#define MQTT_SERVER   "192.168.1.20"
#define MQTT_PORT     1883
#define LED_PIN  2

bool id1,id2,id3,id4,id5,id6,id7 = false;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void callback(char *topic, byte *message, unsigned int length);

void setup_wifi()
{
  delay(10);
  Serial.println("Connecting to WiFi..");

  WiFi.begin(SSID, PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{

  Serial.begin(115200);

  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  pinMode(LED_PIN, OUTPUT);
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
   
  Serial.print(". Message: ");
  String messageTemp;

  
  
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
   if(messageTemp=="4;107;15;226;80;90;128;"){
    id1=true;
    Serial.println("");
    Serial.println("id 1 herkend");
  }
    else if(messageTemp=="179;247;198;2;"){
      id2=true;
      Serial.println("id 2 herkend");
    }

    else if(messageTemp=="169;175;174;194;"){
      id3=true;
      Serial.println("id 3 herkend");
    }

    else if(messageTemp=="4;58;77;210;156;57;128;"){
      id4=true;
      Serial.println("id 4 herkend");
    }

    else if(messageTemp=="4;7;204;82;168;88;129;"){
      id5 = true;
      Serial.println("id 5 herkend");
    }

    else if(messageTemp=="105;66;162;184;"){
      id6 = true;
      Serial.println("id 6 herkend");
    }

    else if(messageTemp=="4;156;198;106;153;91;128;"){
      id7 = true;
      Serial.println("id 7 herkend");
    }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == "esp32/output")
  {
    Serial.print("Changing output to ");
    if (messageTemp == "on")
    {
      Serial.println("on");
      digitalWrite(LED_PIN, HIGH);
    }
    else if (messageTemp == "off")
    {
      Serial.println("off");
      digitalWrite(LED_PIN, LOW);
    }
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Client"))
    {
      Serial.println("connected");
      // Subscribe
      client.subscribe("tag/#");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000)
  {
    lastMsg = now;
  }

  if(id1 && id2 && id3 && id4 && id5 && id6 && id7){
    Serial.println("alles gescanned");
    const char *password  = "durbuy";
    client.publish("password/fdjsqh",password);
    id1=false;
    id2=false;
    id3=false;
    id4=false;
    id5=false;
    id6=false;
    id7=false;
  }



}
*/