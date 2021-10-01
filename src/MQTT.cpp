// mqtt setup information

// #include "LIBRARIES.h"
#include "EXTERNS.h"
#include "DEFINES.h"

#include "Tinker_DEBUG.h"

#include "MQTT.h"
#include "neopixel.h"
#include "web_json.h"
#include "wifisave.h"

#include <PubSubClient.h> // For MQTT

#include "Tinker_SmartSwitch.h"
#include "DEVICE_SPECIFIC.h"

WiFiClient espClient;
PubSubClient MQTT_client(espClient);
extern char DEBUGtxt[92];

#define MQTT_BUFFER_SIZE (64) // This number is arbitrary
// MQTT_command can be up to 65,536 bytes
// MQTT_msg_in can be up to 268,435,456 bytes
// (it's 64 ATM just to make stuff fit in DEBUGtxt)

char MQTT_outTopic[MQTT_BUFFER_SIZE];
char MQTT_inTopic[MQTT_BUFFER_SIZE];
char MQTT_teleTopic[MQTT_BUFFER_SIZE];
char MQTT_statTopic[MQTT_BUFFER_SIZE];

// const char MQTT_ClientName[] = STR(DeviceName);
char MQTT_ClientName[32];

void MQTT_init()
{
  DEBUG_Init("MQTT");

  strcpy(MQTT_ClientName, host);

  if (strcmp(mqtt_broker, "0") == 0) // no broker declared
  {
    DEBUG_Trouble("no broker declared");
  }
  else // broker declared
  {
    MQTT_settopics(); // mqtt_setup.h - set mqtt topics
    MQTT_client.setServer(mqtt_broker, mqtt_port);
    MQTT_client.setCallback(MQTT_callback);
  }
}

void MQTT_settopics()
{ // sets mqtt topics based on hostname
  DEBUG_Init("MQTT Topics");

  //////////////////////////////////////////////////
  //  From TinkerLibs_MQTT
  //////////////////////////////////////////////////
  sprintf(DEBUGtxt, "ClientName: %s", MQTT_ClientName);
  DEBUG_LineOut(DEBUGtxt);

  //  Build the topic names
  strcpy(MQTT_heartbeat, MQTT_ClientName);
  strcat(MQTT_heartbeat, "/LWT");

  sprintf(DEBUGtxt, "MQTT_heartbeat: %s", MQTT_heartbeat);
  DEBUG_LineOut(DEBUGtxt);

  strcpy(MQTT_inTopic, "cmnd/"); //  in - Commands
  strcat(MQTT_inTopic, MQTT_ClientName);
  strcat(MQTT_inTopic, "/#");

  sprintf(DEBUGtxt, "MQTT_inTopic:   %s", MQTT_inTopic);
  DEBUG_LineOut(DEBUGtxt);

  strcpy(MQTT_teleTopic, "tele/"); // out - Telemetry
  strcat(MQTT_teleTopic, MQTT_ClientName);

  sprintf(DEBUGtxt, "MQTT_teleTopic: %s/#", MQTT_teleTopic);
  DEBUG_LineOut(DEBUGtxt);

  strcpy(MQTT_statTopic, "stat/"); // out - Status
  strcat(MQTT_statTopic, MQTT_ClientName);

  sprintf(DEBUGtxt, "MQTT_statTopic: %s/#", MQTT_statTopic);
  DEBUG_LineOut(DEBUGtxt);

  strcpy(MQTT_outTopic, "noti/"); // out - Notifications
  strcat(MQTT_outTopic, MQTT_ClientName);

  sprintf(DEBUGtxt, "MQTT_outTopic:  %s/#", MQTT_outTopic);
  DEBUG_LineOut(DEBUGtxt);
}

void MQTT_callback(char *MQTT_topic, byte *MQTT_payload, unsigned int length)
{
  sprintf(DEBUGtxt, "MQTT_msg_in arrived [%s] ", MQTT_topic);
  DEBUG_LineOut(DEBUGtxt);

  MQTT_payload[length] = '\0';
  sprintf(DEBUGtxt, "MQTT_payload:[%s] ", (char *)MQTT_payload);
  DEBUG_LineOut2(DEBUGtxt);

  //////////////////////////////////////////////////
  //  From TinkerLibs_MQTT
  //////////////////////////////////////////////////

  // char MQTT_msg_in[MQTT_BUFFER_SIZE];
  char MQTT_msg_in[28]; // Limiting size for DEBUG functions...

  char *MQTT_command = strrchr(MQTT_topic, '/');
  char CNasT[MQTT_BUFFER_SIZE];
  strcpy(CNasT, "/");
  strcat(CNasT, MQTT_ClientName); // "ClientName as MQTT_command"

  DEBUG_SectionTitle("MQTT_msg_in arrived");
  sprintf(DEBUGtxt, "MQTT_command: %30s", MQTT_command);
  DEBUG_LineOut(DEBUGtxt);

  // if (length < MQTT_BUFFER_SIZE)
  // if (length < 63)
  // Messages 63 characters long or bigger make BOOM
  // with a "Soft WDT reset"
  // MQTT_BUFFER_SIZE is 100
  // I R cornfoozed...
  if (length < 59) // & now 59 is bad...  :|
  {

    MQTT_msg_in[0] = '\0'; // start with an empty string!
    for (unsigned int i = 0; i < length; i++)
    {
      MQTT_msg_in[i] = (char)MQTT_payload[i];
      MQTT_msg_in[i + 1] = '\0';
    }

    sprintf(DEBUGtxt, "MQTT_msg_in: %28s", MQTT_msg_in);
    DEBUG_LineOut(DEBUGtxt);
    sprintf(DEBUGtxt, "MQTT_msg_in Size: %d", length);
    DEBUG_LineOut(DEBUGtxt);

    /////////////////////////////////////////////////////
    // MQTT_msg_in handling goes here...
    /////////////////////////////////////////////////////

    if (strcmp(MQTT_command, CNasT) == 0) // MQTT_ClientName as MQTT_command
    {
      DEBUG_Trouble("Missing topic...");
      MQTT_SendNOTI("Error", "Missing topic...");
    }

    /*********************************************************************
         * This is where you need to send the incoming message off
         * to be handled elsewhere.
         * Probably in your main project code would be best...
         * 
         * Need a standardised function call. (should be in main project...)
         * void MQTT_HandleMessages(const char *MQTT_command, const char *MQTT_msg_in)
         *********************************************************************/
    else
    {
      MQTT_HandleMessages(MQTT_command, MQTT_msg_in);
    }
  }
  else
  {
    DEBUG_Trouble("But, it's TOO Bloody Big!");
    MQTT_SendNOTI("Error", "MQTT_msg_in TOO Big!");
  }
  //////////////////////////////////////////////////

} // end of mqtt MQTT_callback

extern int SmartSwitch_RelayPin[4];
extern bool SmartSwitch_Relay_STATE[4];
extern int RelayCount;

extern int SmartSwitch_LEDPin[4];
extern bool SmartSwitch_LED_STATE[4];
extern int LEDCount;

/////////////////////////////////////////////////////
void MQTT_HandleMessages(const char *MQTT_command, const char *MQTT_msg_in)
{
  String TheTopic = MQTT_command;
  char NotiMSG[32];
  int CTRbegin, CTRfinish;

/////////////////////////////////////////////////////
  if (TheTopic.startsWith("/Power"))
  {
    TheTopic.remove(0, 6);
    if (TheTopic == "")
    {
      CTRbegin = 0;
      CTRfinish = RelayCount;
      MQTT_SendNOTI("triggered", "All Da Power!");
    }
    else
    {
      CTRbegin = TheTopic.toInt();
      CTRfinish = TheTopic.toInt() + 1;
      sprintf(NotiMSG, "Power%s", TheTopic.c_str());
      MQTT_SendNOTI("triggered", NotiMSG);
    }

    for (int CTR = CTRbegin; CTR < CTRfinish; CTR++)
    {
      if (TheTopic.toInt() < RelayCount)
      {
        if (strcmp(MQTT_msg_in, "on") == 0)
          DEVICE_RELAY(CTR, HIGH);
        if (strcmp(MQTT_msg_in, "off") == 0)
          DEVICE_RELAY(CTR, LOW);
        if (strcmp(MQTT_msg_in, "toggle") == 0)
          DEVICE_TOGGLE(CTR);
      }
      else
        MQTT_SendNOTI("triggered", "Unknown Power!");
    }
  }
  /////////////////////////////////////////////////////
  if (TheTopic.startsWith("/LED"))
  {
    TheTopic.remove(0, 6);
    if (TheTopic == "")
    {
      CTRbegin = 0;
      CTRfinish = LEDCount;
      MQTT_SendNOTI("triggered", "All Da LEDs!");
    }
    else
    {
      CTRbegin = TheTopic.toInt();
      CTRfinish = TheTopic.toInt() + 1;
      sprintf(NotiMSG, "LED%s", TheTopic.c_str());
      MQTT_SendNOTI("triggered", NotiMSG);
    }

    for (int CTR = CTRbegin; CTR < CTRfinish; CTR++)
    {
      if (TheTopic.toInt() < LEDCount)
      {
        if (strcmp(MQTT_msg_in, "on") == 0)
          DEVICE_LED(CTR, HIGH);
        if (strcmp(MQTT_msg_in, "off") == 0)
          DEVICE_LED(CTR, LOW);
        if (strcmp(MQTT_msg_in, "toggle") == 0)
          DEVICE_LED_TOGGLE(CTR);
      }
      else
        MQTT_SendNOTI("triggered", "Unknown LED!");
    }
  }
  /////////////////////////////////////////////////////
  // else if (strcmp(MQTT_command, "/LED00") == 0)
  // {
  //   // MQTT_SendTELE(MQTT_command, MQTT_command);
  //   MQTT_SendNOTI("triggered", "LED00!!!");
  //   if (strcmp(MQTT_msg_in, "on") == 0)
  //     DEVICE_LED(1, HIGH);
  //   if (strcmp(MQTT_msg_in, "off") == 0)
  //     DEVICE_LED(1, LOW);
  //   if (strcmp(MQTT_msg_in, "toggle") == 0)
  //     DEVICE_LED(1, !SmartSwitch_LED_STATE[0]);
  // }
 
  else if (strcmp(MQTT_command, "/Status") == 0)  ///// Needs complete rewrite
  {
    MQTT_SendNOTI("triggered", "Status!!!");
    DEBUG_LineOut("Status Requested");
    if (strcmp(MQTT_msg_in, "Power") == 0)
    {
      MQTT_SendSTAT("Power", SmartSwitch_Relay_STATE[0] ? "ON" : "OFF");
    }
    else if (strcmp(MQTT_msg_in, "LED01") == 0)
    {
      MQTT_SendSTAT("LED01", SmartSwitch_LED_STATE[0] ? "ON" : "OFF");
    }
    else if (strcmp(MQTT_msg_in, "LNKLD") == 0)
    {
      MQTT_SendSTAT("LNKLD", SmartSwitch_LED_STATE[1] ? "ON" : "OFF");
    }
    else if (strcmp(MQTT_msg_in, "All") == 0)
    {
      MQTT_SendSTAT("Power", SmartSwitch_Relay_STATE[0] ? "ON" : "OFF");
      MQTT_SendSTAT("LED01", SmartSwitch_LED_STATE[0] ? "ON" : "OFF");
      MQTT_SendSTAT("LNKLD", SmartSwitch_LED_STATE[1] ? "ON" : "OFF");
    }
    // else if (strcmp(MQTT_msg_in, "All") == 0)
  }

  else
  {
    DEBUG_Trouble("Dunno Whatcha want...");
    MQTT_SendNOTI("Error", "Dunno Whatcha want...");
  }

}

void MQTT_reconnect()
{

  // Loop until we're reconnected
  // while (!MQTT_client.connected())
  // {
  PIXELS_colorWipe(0, 0, 0, 220);

  DEBUG_Init("Attempting MQTT connection...");
  if (MQTT_client.connect(host, mqtt_username, mqtt_password, out_heart, mqtt_lwt_qos, mqtt_lwt_retain, "0"))
  {
    IPaddress = WiFi.localIP().toString();
    DEBUG_Success("mqtt connected");
    connected_update = true;
    // Once connected, publish an announcement...
    strcpy(connectphrase, "connected = ");
    strcat(connectphrase, host);
    strcat(connectphrase, "  IP Address = ");
    const char *c = IPaddress.c_str();
    strcat(connectphrase, c); // ip address
    const char *phrase = connectphrase;
    MQTT_client.publish("outTopic", phrase);
    byte lwt_payload[] = {'1'};
    MQTT_client.publish(out_heart, lwt_payload, 1, mqtt_lwt_retain);
    // ... and resubscribe

    MQTT_client.subscribe(MQTT_inTopic);
  }
  else
  {
    sprintf(DEBUGtxt, "failed, rc=%d try again in 5 seconds", MQTT_client.state());
    DEBUG_Trouble(DEBUGtxt);
    //       Wait 5 seconds before retrying
    delay(5000);
  }
  // }
}

void MQTT_loop()
{
  if (!MQTT_client.connected())
  {
    MQTT_reconnect();
  }
  MQTT_client.loop();
}

void MQTT_send(const char *MQTT_command, char Buffer[40])
{
  MQTT_client.publish(MQTT_command, Buffer);
}

/* Special for UT61E */
void MQTT_JSON_send(char g_mqtt_json_topic[50], size_t msg_length, bool TF, char g_json_message_buffer[512])
{
  MQTT_client.beginPublish(g_mqtt_json_topic, msg_length, TF);
  MQTT_client.print(g_json_message_buffer);
  MQTT_client.endPublish();
}

////////////////////////////////////////////////////////////////////////////
//  These translate the TinkerLibs MQTT function calls to AustinOTA calls //
////////////////////////////////////////////////////////////////////////////

void MQTT_SendSTAT(const char *MQTT_command, const char *MQTT_msg_in)
{
  char TheTopic[128];
  char TheMessage[128];
  strcpy(TheTopic, "stat/");
  strcat(TheTopic, host);
  strcat(TheTopic, "/");
  strcat(TheTopic, MQTT_command);
  //        MQTT_SendSTAT("Power", "ON");
  //        MQTT_SendSTAT("LED01", "ON");
  //        MQTT_SendSTAT("LED02", "ON");
  strcpy(TheMessage, MQTT_msg_in);
  MQTT_send(TheTopic, TheMessage);
}

void MQTT_SendNOTI(const char *MQTT_command, const char *MQTT_msg_in)
{
  char TheTopic[128];
  char TheMessage[128];
  strcpy(TheTopic, "noti/");
  strcat(TheTopic, host);
  strcat(TheTopic, "/");
  strcat(TheTopic, MQTT_command);
  //        MQTT_SendNOTI("triggered", "Power!!!");
  //        MQTT_SendNOTI("Error", "Dunno Whatcha want...");
  strcpy(TheMessage, MQTT_msg_in);
  MQTT_send(TheTopic, TheMessage);
}

void MQTT_SendTELE(const char *MQTT_command, const char *MQTT_msg_in)
{
  char TheTopic[128];
  char TheMessage[128];
  strcpy(TheTopic, "tele/");
  strcat(TheTopic, host);
  strcat(TheTopic, "/");
  strcat(TheTopic, MQTT_command);
  strcpy(TheMessage, MQTT_msg_in);
  MQTT_send(TheTopic, TheMessage);
}
