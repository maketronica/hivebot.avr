#include <DHT.h>
#include <EEPROM.h>
#include "HX711/HX711.h"

const byte HIVE_ID_PINS[3] = { 2, 3, 4 }; 
const byte DHT_BOT_PIN = 5;
const byte DHT_BROOD_PIN = 6;
const byte HX711_CLK = 8;
const byte HX711_DOUT = 9;
const byte ACTIVITY_INDICATOR_PIN = 13;

const unsigned long MAX_MILLIS = pow(2, sizeof(millis())*8 );
const unsigned long MILLIS_PER_DAY = 86400000;
const unsigned int MINUTES_PER_DAY = 1440;
const float DAYS_PER_ROLLOVER = MAX_MILLIS/(float)MILLIS_PER_DAY;

const float SCALE_CALIBRATION_FACTOR = 13500;
const float SCALE_ZERO_FACTOR = 103727;

const unsigned long MILLIS_PER_BROADCAST = 60000;

const byte BOT_ID = EEPROM.read(0);

String full_data_to_send;

unsigned long uptime_minutes = 0;
boolean uptime_impending_rollover = false;
unsigned int uptime_rollover_counter = 0;

DHT dhtbot(DHT_BOT_PIN, DHT22);                       //Electronics Cabinet Temp/Humidity Sensor
DHT dhtbrood(DHT_BROOD_PIN, DHT22);                   //Brood Box Temp/Humidity Sensor
HX711 scale(HX711_DOUT, HX711_CLK);

void setup()
{
    // >>> NEW BOT ID SETUP <<<
    //  Uncomment below lines and update to
    //  set botid on new board and then
    //  recomment out.
    //byte new_bot_id = 1;
    //EEPROM.update(0, new_bot_id);
    // >>> END NEW BOT ID SETUP <<<

    pinMode(ACTIVITY_INDICATOR_PIN, OUTPUT);
    Serial.begin(115200);
  
    dhtbot.begin();
    dhtbrood.begin();
    scale.set_scale(SCALE_CALIBRATION_FACTOR);
    scale.set_offset(SCALE_ZERO_FACTOR);
}

void loop()
{
    digitalWrite(ACTIVITY_INDICATOR_PIN, HIGH);
    updateUptime();
    
    full_data_to_send = "";
    add_pair(F("hive_id"), String(hive_id()));
    add_pair(F("bot_id"), String(BOT_ID));
    add_pair(F("bot_uptime"), String(uptime_minutes));
    add_pair(F("bot_temp"), String((int)(dhtbot.readTemperature()*10)));
    add_pair(F("bot_humidity"), String((int)(dhtbot.readHumidity()*10)));
    add_pair(F("brood_temp"), String((int)(dhtbrood.readTemperature()*10)));
    add_pair(F("brood_humidity"), String((int)(dhtbrood.readHumidity()*10)));
    add_pair(F("hive_lbs"), String((int)scale.get_units()*100));
    
    send_data(full_data_to_send);
    digitalWrite(ACTIVITY_INDICATOR_PIN, LOW);
    delay(MILLIS_PER_BROADCAST);
}

void updateUptime(){
  if( millis() >= (MAX_MILLIS/2)) { uptime_impending_rollover = true; }
  if( millis()< (MAX_MILLIS/2) && uptime_impending_rollover == true ) {
    uptime_rollover_counter++;
    uptime_impending_rollover = false;
  }
  uptime_minutes = ((uptime_rollover_counter * DAYS_PER_ROLLOVER) + 
                    ((float)millis() / MILLIS_PER_DAY)) * MINUTES_PER_DAY;
};

void add_pair(String key, String value) {
  full_data_to_send += key + "=" + value + '&';
}

void send_data(String message) {
  unsigned int computed_checksum = compute_checksum(message);
  message = "B|" + message + "%" + computed_checksum + "|E";
  Serial.println(message);
}

byte compute_checksum(String message) {
  byte sum = 0;
  char message_chra[message.length()+1];
  message.toCharArray(message_chra, message.length()+1);
  for(int i = 0; i <= strlen(message_chra); i++) {
    sum += int(message_chra[i]);
  }
  return sum;
}

byte hive_id() {
  byte place = 1;
  byte sum = 0;
  for(byte i = 0; i < sizeof(HIVE_ID_PINS); i++) {
    sum += place * digitalRead(HIVE_ID_PINS[i]);
    place *= 2;
  }
  return sum;
}

