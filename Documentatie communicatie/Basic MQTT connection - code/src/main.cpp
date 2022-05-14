#include "WiFi.h"
#include "PubSubClient.h" //pio lib install "knolleary/PubSubClient"

#define SSID          "NETGEAR68"
#define PWD           "excitedtuba713"

#define MQTT_SERVER   "192.168.1.2"
#define MQTT_PORT     1883

#define LED_PIN       2

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


}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // CREATE UNIQUE client ID!
    // in Mosquitto broker enable anom. access
    if (client.connect("Luisteraar"))
    {
      Serial.println("connected");
      // Subscribe
      // Vul hieronder in naar welke directories je gaat luisteren.
      //Voor de communicatie tussen de puzzels, check "Datacommunicatie.docx". (terug tevinden in dezelfde repository) 
      client.subscribe("controlpanel/reset");
      client.subscribe("controlpanel/status");
      client.subscribe("trappenmaar/buffer");    }
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

  //client.publish("controlpanel/reset","Reset escaperoom"); delay(10000000000);
  
  //Of
  client.publish("garbage/eindcode", "5555"); delay(10000000000);
  
 
  

}