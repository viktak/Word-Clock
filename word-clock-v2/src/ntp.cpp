#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

#define DEBUG_NTPClient
#define __debugSettings
#include <NTPClient.h>

#include "ntp.h"

#define NTP_UPDATE_INTERVAL_MS 360000 //  synchronize time with NTP server once an hour

namespace ntp
{
    WiFiUDP ntpUDP;

#ifdef __debugSettings
    char timeServer[] = "192.168.123.2";
#else
    char timeServer[] = "pool.ntp.org";
#endif

    NTPClient timeClient(ntpUDP, timeServer, 0, NTP_UPDATE_INTERVAL_MS);

    void setup()
    {
        timeClient.begin();
    }

    void loop()
    {
        timeClient.update();
        if (timeClient.isTimeSet())
        {
            setTime(timeClient.getEpochTime());
        }
    }
}