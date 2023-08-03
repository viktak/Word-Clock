#define __debugSettings

#include "includes.h"

//  Select exactly one of the following: WS2812B, SK6812
#define WS2812B 

//  Select exactly one of the following:
// #include "vic-vik.h"
#include "vic-vik-W2812b.h"

//  Web server
ESP8266WebServer server(80);

//  Initialize Wifi
WiFiClient wclient;
PubSubClient PSclient(wclient);

//  Timers and their flags
os_timer_t heartbeatTimer;
os_timer_t accessPointTimer;

//  Flags
bool needsHeartbeat = false;
bool timeDirty = true;

//  NeoPixel
#ifdef WS2812B
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(TOTAL_LEDS);
#endif
#ifdef SK6812
NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(TOTAL_LEDS);
#endif
NeoGamma<NeoGammaTableMethod> colorGamma;
NeoPixelAnimator animations(TOTAL_LEDS);
segment segments[ANIMATION_CHANNEL::SIZE_OF_ENUM];

//  Other global variables
const char *months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
config appConfig;
bool isAccessPoint = false;
bool isAccessPointCreated = false;

char localHost[32];
TimeChangeRule *tcr; // Pointer to the time change rule

bool semaphores[6] = {false, false, false, false, false, false};

unsigned long oldMillis = 0;
unsigned long oldMillisGreetings = 0;
unsigned long greetingsTime = 0;
bool isGreetingsModeOn = false;

bool ntpInitialized = false;

enum CONNECTION_STATE connectionState;

const char *internetServerName = "diy.viktak.com";

WiFiUDP Udp;

void LogEvent(int Category, int ID, String Title, String Data)
{
    if (PSclient.connected())
    {

        String msg = "{";

        msg += "\"Node\":" + (String)ESP.getChipId() + ",";
        msg += "\"Category\":" + (String)Category + ",";
        msg += "\"ID\":" + (String)ID + ",";
        msg += "\"Title\":\"" + Title + "\",";
        msg += "\"Data\":\"" + Data + "\"}";

        debugln(msg);

        PSclient.publish((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/log").c_str(), msg.c_str(), false);
    }
}

void SetRandomSeed()
{
    uint32_t seed;

    // random works best with a seed that can use 31 bits
    // analogRead on a unconnected pin tends toward less than four bits
    seed = analogRead(0);
    delay(1);

    for (int shifts = 3; shifts < 31; shifts += 3)
    {
        seed ^= analogRead(0) << shifts;
        delay(1);
    }

    randomSeed(seed);
}

String GetDeviceMAC()
{
    String s = WiFi.macAddress();

    for (size_t i = 0; i < 5; i++)
        s.remove(14 - i * 3, 1);

    return s;
}

boolean checkInternetConnection()
{
    IPAddress timeServerIP;
    int result = WiFi.hostByName(internetServerName, timeServerIP);
    return (result == 1);
}

void InitSegments()
{

    for (uint16_t i = 0; i < ANIMATION_CHANNEL::SIZE_OF_ENUM; i++)
    {
        segments[i].fadeDirection = false;
        segments[i].id = i;
        segments[i].isActive = false;

        switch (i)
        {
        case ANIMATION_CHANNEL::SENTENCE:
            segments[i].length = sizeof(wSentence) / sizeof(unsigned int);
            for (uint16_t j = 0; j < segments[i].length; j++)
            {
                segments[i].positions[j] = wSentence[j];
            }
            break;
        case ANIMATION_CHANNEL::TEN_MINUTES:
            for (uint16_t j = 0; j < sizeof(wTenMinutes) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wTenMinutes[j];
            }
            break;
        case ANIMATION_CHANNEL::HALF:
            for (uint16_t j = 0; j < sizeof(wHalf) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wHalf[j];
            }
            break;
        case ANIMATION_CHANNEL::QUARTER:
            for (uint16_t j = 0; j < sizeof(wQuarter) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wQuarter[j];
            }
            break;
        case ANIMATION_CHANNEL::TWENTY:
            for (uint16_t j = 0; j < sizeof(wTwenty) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wTwenty[j];
            }
            break;
        case ANIMATION_CHANNEL::FIVE_MINUTES:
            for (uint16_t j = 0; j < sizeof(wFiveMinutes) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wFiveMinutes[j];
            }
            break;
        case ANIMATION_CHANNEL::MINUTES:
            for (uint16_t j = 0; j < sizeof(wMinutes) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wMinutes[j];
            }
            break;
        case ANIMATION_CHANNEL::HAPPY:
            for (uint16_t j = 0; j < sizeof(wHappy) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wHappy[j];
            }
            break;
        case ANIMATION_CHANNEL::PAST:
            for (uint16_t j = 0; j < sizeof(wPast) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wPast[j];
            }
            break;
        case ANIMATION_CHANNEL::TO:
            for (uint16_t j = 0; j < sizeof(wTo) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wTo[j];
            }
            break;
        case ANIMATION_CHANNEL::BIRTH:
            for (uint16_t j = 0; j < sizeof(wBirth) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wBirth[j];
            }
            break;
        case ANIMATION_CHANNEL::DAY:
            for (uint16_t j = 0; j < sizeof(wDay) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wDay[j];
            }
            break;
        case ANIMATION_CHANNEL::OCLOCK:
            for (uint16_t j = 0; j < sizeof(woClock) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = woClock[j];
            }
            break;
        case ANIMATION_CHANNEL::ONE:
            for (uint16_t j = 0; j < sizeof(wOne) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wOne[j];
            }
            break;
        case ANIMATION_CHANNEL::TWO:
            for (uint16_t j = 0; j < sizeof(wTwo) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wTwo[j];
            }
            break;
        case ANIMATION_CHANNEL::THREE:
            for (uint16_t j = 0; j < sizeof(wThree) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wThree[j];
            }
            break;
        case ANIMATION_CHANNEL::FOUR:
            for (uint16_t j = 0; j < sizeof(wFour) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wFour[j];
            }
            break;
        case ANIMATION_CHANNEL::FIVE:
            for (uint16_t j = 0; j < sizeof(wFive) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wFive[j];
            }
            break;
        case ANIMATION_CHANNEL::SIX:
            for (uint16_t j = 0; j < sizeof(wSix) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wSix[j];
            }
            break;
        case ANIMATION_CHANNEL::SEVEN:
            for (uint16_t j = 0; j < sizeof(wSeven) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wSeven[j];
            }
            break;
        case ANIMATION_CHANNEL::EIGHT:
            for (uint16_t j = 0; j < sizeof(wEight) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wEight[j];
            }
            break;
        case ANIMATION_CHANNEL::NINE:
            for (uint16_t j = 0; j < sizeof(wNine) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wNine[j];
            }
            break;
        case ANIMATION_CHANNEL::TEN:
            for (uint16_t j = 0; j < sizeof(wTen) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wTen[j];
            }
            break;
        case ANIMATION_CHANNEL::ELEVEN:
            for (uint16_t j = 0; j < sizeof(wEleven) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wEleven[j];
            }
            break;
        case ANIMATION_CHANNEL::TWELVE:
            for (uint16_t j = 0; j < sizeof(wTwelve) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wTwelve[j];
            }
            break;

        case ANIMATION_CHANNEL::NAME0:
            for (uint16_t j = 0; j < sizeof(wName0) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wName0[j];
            }
            break;
        case ANIMATION_CHANNEL::NAME1:
            for (uint16_t j = 0; j < sizeof(wName1) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wName1[j];
            }
            break;
        case ANIMATION_CHANNEL::LOGO:
            for (uint16_t j = 0; j < sizeof(wLogo) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wLogo[j];
            }
            break;

        case ANIMATION_CHANNEL::SUNDAY:
            for (uint16_t j = 0; j < sizeof(wSunday) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wSunday[j];
            }
            break;

        case ANIMATION_CHANNEL::MONDAY:
            for (uint16_t j = 0; j < sizeof(wMonday) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wMonday[j];
            }
            break;

        case ANIMATION_CHANNEL::TUESDAY:
            for (uint16_t j = 0; j < sizeof(wTuesday) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wTuesday[j];
            }
            break;

        case ANIMATION_CHANNEL::WEDNESDAY:
            for (uint16_t j = 0; j < sizeof(wWednesday) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wWednesday[j];
            }
            break;

        case ANIMATION_CHANNEL::THURSDAY:
            for (uint16_t j = 0; j < sizeof(wThursday) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wThursday[j];
            }
            break;

        case ANIMATION_CHANNEL::FRIDAY:
            for (uint16_t j = 0; j < sizeof(wFriday) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wFriday[j];
            }
            break;

        case ANIMATION_CHANNEL::SATURDAY:
            for (uint16_t j = 0; j < sizeof(wSaturday) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wSaturday[j];
            }
            break;

        case ANIMATION_CHANNEL::SEMAPHORE0:
            for (uint16_t j = 0; j < sizeof(wSemaphore0) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wSemaphore0[j];
            }
            break;

        case ANIMATION_CHANNEL::SEMAPHORE1:
            for (uint16_t j = 0; j < sizeof(wSemaphore1) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wSemaphore1[j];
            }
            break;

        case ANIMATION_CHANNEL::SEMAPHORE2:
            for (uint16_t j = 0; j < sizeof(wSemaphore2) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wSemaphore2[j];
            }
            break;

        case ANIMATION_CHANNEL::SEMAPHORE3:
            for (uint16_t j = 0; j < sizeof(wSemaphore3) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wSemaphore3[j];
            }
            break;

        case ANIMATION_CHANNEL::SEMAPHORE4:
            for (uint16_t j = 0; j < sizeof(wSemaphore4) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wSemaphore4[j];
            }
            break;

        case ANIMATION_CHANNEL::SEMAPHORE5:
            for (uint16_t j = 0; j < sizeof(wSemaphore5) / sizeof(unsigned int); j++)
            {
                segments[i].positions[j] = wSemaphore5[j];
            }
            break;

        default:
            break;
        }
    }
}

void SetColorScheme(OPERATION_MODE mode)
{
    RgbColor color = HslColor(random(360) / 360.0f, 1.0f, 0.5f);

    for (unsigned int i = 0; i < ANIMATION_CHANNEL::SIZE_OF_ENUM; i++)
    {
        switch (mode)
        {
        case OPERATION_MODE::GLOWING_COLOR:
            segments[i].color = HslColor(random(360) / 360.0f, 1.0f, 0.5f);
            break;
        case OPERATION_MODE::GLOWING_MONO:
            segments[i].color = color;
            break;
        case OPERATION_MODE::STATIC_COLOR:
            segments[i].color = HslColor(random(360) / 360.0f, 1.0f, 0.5f);
            break;
        case OPERATION_MODE::STATIC_MONO:
            segments[i].color = color;
            break;

        default:
            break;
        }
    }
}

void BlendAnimUpdate(const AnimationParam &param)
{
    RgbColor updatedColor = RgbColor::LinearBlend(segments[param.index].animationState.StartingColor, segments[param.index].animationState.EndingColor, param.progress);

    switch (param.index)
    {
    case ANIMATION_CHANNEL::SENTENCE:
        for (unsigned long i = 0; i < sizeof(wSentence) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wSentence[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::TEN_MINUTES:
        for (unsigned long i = 0; i < sizeof(wTenMinutes) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wTenMinutes[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::HALF:
        for (unsigned long i = 0; i < sizeof(wHalf) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wHalf[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::QUARTER:
        for (unsigned long i = 0; i < sizeof(wQuarter) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wQuarter[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::TWENTY:
        for (unsigned long i = 0; i < sizeof(wTwenty) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wTwenty[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::FIVE_MINUTES:
        for (unsigned long i = 0; i < sizeof(wFiveMinutes) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wFiveMinutes[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::MINUTES:
        for (unsigned long i = 0; i < sizeof(wMinutes) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wMinutes[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::HAPPY:
        for (unsigned long i = 0; i < sizeof(wHappy) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wHappy[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::PAST:
        for (unsigned long i = 0; i < sizeof(wPast) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wPast[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::TO:
        for (unsigned long i = 0; i < sizeof(wTo) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wTo[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::BIRTH:
        for (unsigned long i = 0; i < sizeof(wBirth) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wBirth[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::DAY:
        for (unsigned long i = 0; i < sizeof(wDay) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wDay[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::OCLOCK:
        for (unsigned long i = 0; i < sizeof(woClock) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(woClock[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::ONE:
        for (unsigned long i = 0; i < sizeof(wOne) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wOne[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::TWO:
        for (unsigned long i = 0; i < sizeof(wTwo) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wTwo[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::THREE:
        for (unsigned long i = 0; i < sizeof(wThree) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wThree[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::FOUR:
        for (unsigned long i = 0; i < sizeof(wFour) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wFour[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::FIVE:
        for (unsigned long i = 0; i < sizeof(wFive) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wFive[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::SIX:
        for (unsigned long i = 0; i < sizeof(wSix) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wSix[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::SEVEN:
        for (unsigned long i = 0; i < sizeof(wSeven) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wSeven[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::EIGHT:
        for (unsigned long i = 0; i < sizeof(wEight) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wEight[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::NINE:
        for (unsigned long i = 0; i < sizeof(wNine) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wNine[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::TEN:
        for (unsigned long i = 0; i < sizeof(wTen) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wTen[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::ELEVEN:
        for (unsigned long i = 0; i < sizeof(wEleven) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wEleven[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::TWELVE:
        for (unsigned long i = 0; i < sizeof(wTwelve) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wTwelve[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::NAME0:
        for (unsigned long i = 0; i < sizeof(wName0) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wName0[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::NAME1:
        for (unsigned long i = 0; i < sizeof(wName1) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wName1[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::LOGO:
        for (unsigned long i = 0; i < sizeof(wLogo) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wLogo[i], colorGamma.Correct(updatedColor));
        }
        break;

    case ANIMATION_CHANNEL::SUNDAY:
        for (unsigned long i = 0; i < sizeof(wSunday) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wSunday[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::MONDAY:
        for (unsigned long i = 0; i < sizeof(wMonday) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wMonday[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::TUESDAY:
        for (unsigned long i = 0; i < sizeof(wTuesday) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wTuesday[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::WEDNESDAY:
        for (unsigned long i = 0; i < sizeof(wWednesday) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wWednesday[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::THURSDAY:
        for (unsigned long i = 0; i < sizeof(wThursday) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wThursday[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::FRIDAY:
        for (unsigned long i = 0; i < sizeof(wFriday) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wFriday[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::SATURDAY:
        for (unsigned long i = 0; i < sizeof(wSaturday) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wSaturday[i], colorGamma.Correct(updatedColor));
        }
        break;

    case ANIMATION_CHANNEL::SEMAPHORE0:
        for (unsigned long i = 0; i < sizeof(wSemaphore0) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wSemaphore0[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::SEMAPHORE1:
        for (unsigned long i = 0; i < sizeof(wSemaphore1) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wSemaphore1[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::SEMAPHORE2:
        for (unsigned long i = 0; i < sizeof(wSemaphore2) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wSemaphore2[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::SEMAPHORE3:
        for (unsigned long i = 0; i < sizeof(wSemaphore3) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wSemaphore3[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::SEMAPHORE4:
        for (unsigned long i = 0; i < sizeof(wSemaphore4) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wSemaphore4[i], colorGamma.Correct(updatedColor));
        }
        break;
    case ANIMATION_CHANNEL::SEMAPHORE5:
        for (unsigned long i = 0; i < sizeof(wSemaphore5) / sizeof(unsigned int); i++)
        {
            strip.SetPixelColor(wSemaphore5[i], colorGamma.Correct(updatedColor));
        }
        break;

    default:
        break;
    }
}

void GlowSegment(uint16_t animChannel, float luminance)
{
    uint16_t time = isGreetingsModeOn ? random(100, 300) : random(1500, 2500);
    float darkeningValue = 1;
    HslColor color1 = segments[animChannel].color;
    HslColor color2 = segments[animChannel].color;

    color1.L = luminance;
    color2.L = luminance;

    switch (appConfig.mode)
    {
    case OPERATION_MODE::STATIC_COLOR:
        darkeningValue = 1;
        break;
    case OPERATION_MODE::STATIC_MONO:
        darkeningValue = 1;
        break;
    case OPERATION_MODE::GLOWING_COLOR:
        darkeningValue = DARKENING_FACTOR;
        break;
    case OPERATION_MODE::GLOWING_MONO:
        darkeningValue = DARKENING_FACTOR;
        break;
    default:
        break;
    }

    if (segments[animChannel].fadeDirection)
        color1.L = color1.L * darkeningValue;
    else
        color2.L = color2.L * darkeningValue;

    segments[animChannel].animationState.StartingColor = color1;
    segments[animChannel].animationState.EndingColor = color2;

    animations.StartAnimation(animChannel, time, BlendAnimUpdate);

    segments[animChannel].fadeDirection = !segments[animChannel].fadeDirection;
}

void DoBootAnimation()
{
    int speedDelay = 30;

    //  Vertical sweep
    for (size_t i = 0; i < NUMBER_OF_ROWS; i++)
    {
        for (size_t j = 0; j < NUMBER_OF_COLUMNS; j++)
        {
            strip.SetPixelColor(i * NUMBER_OF_COLUMNS + j, RgbColor(0xff, 0xff, 0xff));
        }
        strip.Show();
        delay(speedDelay);
        for (size_t j = 0; j < NUMBER_OF_COLUMNS; j++)
        {
            strip.SetPixelColor(i * NUMBER_OF_COLUMNS + j, RgbColor(0));
        }
    }

    for (size_t i = 0; i < NUMBER_OF_ROWS; i++)
    {
        for (size_t j = 0; j < NUMBER_OF_COLUMNS; j++)
        {
            strip.SetPixelColor((NUMBER_OF_ROWS - i) * NUMBER_OF_COLUMNS + j, RgbColor(0xff, 0xff, 0xff));
        }
        strip.Show();
        delay(speedDelay);
        for (size_t j = 0; j < NUMBER_OF_COLUMNS; j++)
        {
            strip.SetPixelColor((NUMBER_OF_ROWS - i) * NUMBER_OF_COLUMNS + j, RgbColor(0));
        }
    }

    //  Horizontal sweep
    size_t pos = 0;
    for (size_t c = 0; c < NUMBER_OF_COLUMNS; c++)
    {
        for (size_t r = 0; r < NUMBER_OF_ROWS; r++)
        {
            if (r % 2 == 0)
            {
                pos = r * NUMBER_OF_COLUMNS + c;
            }
            else
            {
                pos = (r + 1) * NUMBER_OF_COLUMNS - c - 1;
            }
            strip.SetPixelColor(pos, RgbColor(0xff, 0xff, 0xff));
        }
        strip.Show();
        delay(speedDelay);

        for (size_t r = 0; r < NUMBER_OF_ROWS; r++)
        {
            if (r % 2 == 0)
            {
                pos = r * NUMBER_OF_COLUMNS + c;
            }
            else
            {
                pos = (r + 1) * NUMBER_OF_COLUMNS - (c + 1);
            }
            strip.SetPixelColor(pos, RgbColor(0));
        }
    }

    for (size_t c = 0; c < NUMBER_OF_COLUMNS; c++)
    {
        for (size_t r = 0; r < NUMBER_OF_ROWS; r++)
        {
            if (r % 2 == 0)
            {
                pos = (r + 1) * NUMBER_OF_COLUMNS - (c + 1);
            }
            else
            {
                pos = r * NUMBER_OF_COLUMNS + c;
            }
            strip.SetPixelColor(pos, RgbColor(0xff, 0xff, 0xff));
        }
        strip.Show();
        delay(speedDelay);

        for (size_t r = 0; r < NUMBER_OF_ROWS; r++)
        {
            if (r % 2 == 0)
            {
                pos = (r + 1) * NUMBER_OF_COLUMNS - (c + 1);
            }
            else
            {
                pos = r * NUMBER_OF_COLUMNS + c;
            }
            strip.SetPixelColor(pos, RgbColor(0));
        }
    }

    HslColor c = HslColor(random(360) / 360.0f, 1.0f, 0.5f);
    for (unsigned long i = 0; i < sizeof(wLogo) / sizeof(unsigned int); i++)
    {
        strip.SetPixelColor(wLogo[i], c);
    }
    strip.Show();

    delay(3000);

    for (unsigned long i = 0; i < sizeof(wLogo) / sizeof(unsigned int); i++)
    {
        strip.SetPixelColor(wLogo[i], RgbColor(0));
    }

    c = HslColor(random(360) / 360.0f, 1.0f, 0.5f);
    for (unsigned long i = 0; i < sizeof(wName0) / sizeof(unsigned int); i++)
    {
        strip.SetPixelColor(wName0[i], c);
    }

    c = HslColor(random(360) / 360.0f, 1.0f, 0.5f);
    for (unsigned long i = 0; i < sizeof(wName1) / sizeof(unsigned int); i++)
    {
        strip.SetPixelColor(wName1[i], c);
    }

    strip.Show();
}

void accessPointTimerCallback(void *pArg)
{
    ESP.reset();
}

void heartbeatTimerCallback(void *pArg)
{
    needsHeartbeat = true;
}

bool loadSettings(config &data)
{
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile)
    {
        debugln("Failed to open config file");
        LogEvent(EVENTCATEGORIES::System, 1, "FS failure", "Failed to open config file.");
        return false;
    }

    size_t size = configFile.size();
    if (size > 1024)
    {
        debugln("Config file size is too large");
        LogEvent(EVENTCATEGORIES::System, 2, "FS failure", "Config file size is too large.");
        return false;
    }

    // Allocate a buffer to store contents of the file.
    std::unique_ptr<char[]> buf(new char[size]);

    // We don't use String here because ArduinoJson library requires the input
    // buffer to be mutable. If you don't use ArduinoJson, you may as well
    // use configFile.readString instead.
    configFile.readBytes(buf.get(), size);
    configFile.close();

    StaticJsonDocument<JSON_SETTINGS_SIZE> doc;
    DeserializationError error = deserializeJson(doc, buf.get());

    if (error)
    {
        debugln("Failed to parse config file");
        LogEvent(EVENTCATEGORIES::System, 3, "FS failure", "Failed to parse config file.");
        debugln(error.c_str());
        return false;
    }

#ifdef __debugSettings
    serializeJsonPretty(doc, Serial);
    debugln();
#endif

    if (doc["ssid"])
    {
        strcpy(appConfig.ssid, doc["ssid"]);
    }
    else
    {
        sprintf(appConfig.ssid, "%s-%s", DEFAULT_MQTT_TOPIC, GetDeviceMAC().substring(6).c_str());
    }

    if (doc["password"])
    {
        strcpy(appConfig.password, doc["password"]);
    }
    else
    {
        strcpy(appConfig.password, DEFAULT_PASSWORD);
    }

    if (doc["mqttServer"])
    {
        strcpy(appConfig.mqttServer, doc["mqttServer"]);
    }
    else
    {
        strcpy(appConfig.mqttServer, DEFAULT_MQTT_SERVER);
    }

    if (doc["mqttPort"])
    {
        appConfig.mqttPort = doc["mqttPort"];
    }
    else
    {
        appConfig.mqttPort = DEFAULT_MQTT_PORT;
    }

    sprintf(localHost, "%s-%s", DEFAULT_MQTT_TOPIC, GetDeviceMAC().substring(6).c_str());

    if (doc["mqttTopic"])
    {
        strcpy(appConfig.mqttTopic, doc["mqttTopic"]);

        if (strcmp(localHost, appConfig.mqttTopic) != 0)
        {
            sprintf(localHost, "%s-%s", appConfig.mqttTopic, GetDeviceMAC().substring(6).c_str());
        }
    }
    else
    {
        sprintf(appConfig.mqttTopic, localHost);
    }

    if (doc["friendlyName"])
    {
        strcpy(appConfig.friendlyName, doc["friendlyName"]);
    }
    else
    {
        strcpy(appConfig.friendlyName, NODE_DEFAULT_FRIENDLY_NAME);
    }

    if (doc["timezone"])
    {
        appConfig.timeZone = doc["timezone"];
    }
    else
    {
        appConfig.timeZone = 0;
    }

    if (doc["heartbeatInterval"])
    {
        appConfig.heartbeatInterval = doc["heartbeatInterval"];
    }
    else
    {
        appConfig.heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;
    }

    if (doc["birthday0month"])
    {
        appConfig.birthDays[0].month = doc["birthday0month"];
    }
    else
    {
        appConfig.birthDays[0].month = 0;
    }

    if (doc["birthday0day"])
    {
        appConfig.birthDays[0].day = doc["birthday0day"];
    }
    else
    {
        appConfig.birthDays[0].day = 0;
    }

    if (doc["birthday1month"])
    {
        appConfig.birthDays[1].month = doc["birthday1month"];
    }
    else
    {
        appConfig.birthDays[1].month = 0;
    }

    if (doc["birthday1day"])
    {
        appConfig.birthDays[1].day = doc["birthday1day"];
    }
    else
    {
        appConfig.birthDays[1].day = 0;
    }

    if (doc["mode"])
    {
        appConfig.mode = doc["mode"];
    }
    else
    {
        appConfig.mode = OPERATION_MODE::GLOWING_COLOR;
    }

    return true;
}

bool saveSettings()
{
    StaticJsonDocument<JSON_SETTINGS_SIZE> doc;

    doc["ssid"] = appConfig.ssid;
    doc["password"] = appConfig.password;

    doc["heartbeatInterval"] = appConfig.heartbeatInterval;

    doc["timezone"] = appConfig.timeZone;

    doc["mqttServer"] = appConfig.mqttServer;
    doc["mqttPort"] = appConfig.mqttPort;
    doc["mqttTopic"] = appConfig.mqttTopic;

    doc["friendlyName"] = appConfig.friendlyName;

    doc["birthday0month"] = appConfig.birthDays[0].month;
    doc["birthday0day"] = appConfig.birthDays[0].day;

    doc["birthday1month"] = appConfig.birthDays[1].month;
    doc["birthday1day"] = appConfig.birthDays[1].day;

    doc["mode"] = appConfig.mode;

#ifdef __debugSettings
    serializeJsonPretty(doc, Serial);
    Serial.println();
#endif

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile)
    {
        debugln("Failed to open config file for writing");
        LogEvent(System, 4, "FS failure", "Failed to open config file for writing.");
        return false;
    }
    serializeJson(doc, configFile);
    configFile.close();

    return true;
}

void defaultSettings()
{
    sprintf(localHost, "%s-%s", DEFAULT_MQTT_TOPIC, GetDeviceMAC().substring(6).c_str());

    strcpy(appConfig.ssid, localHost);
    strcpy(appConfig.password, DEFAULT_AP_PASSWORD);
    strcpy(appConfig.mqttServer, DEFAULT_MQTT_SERVER);

    appConfig.mqttPort = DEFAULT_MQTT_PORT;

    strcpy(appConfig.mqttTopic, localHost);

    appConfig.timeZone = 2;

    strcpy(appConfig.friendlyName, NODE_DEFAULT_FRIENDLY_NAME);
    appConfig.heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;

    appConfig.birthDays[0].month = 0;
    appConfig.birthDays[0].day = 0;

    appConfig.birthDays[1].month = 0;
    appConfig.birthDays[1].day = 0;

    appConfig.mode = OPERATION_MODE::GLOWING_COLOR;

    if (!saveSettings())
    {
        debugln("Failed to save config");
    }
    else
    {
        debugln("Config saved");
    }
}

String DateTimeToString(time_t time)
{

    String myTime = "";
    char s[2];

    //  years
    itoa(year(time), s, DEC);
    myTime += s;
    myTime += "-";

    //  months
    itoa(month(time), s, DEC);
    myTime += s;
    myTime += "-";

    //  days
    itoa(day(time), s, DEC);
    myTime += s;

    myTime += " ";

    //  hours
    itoa(hour(time), s, DEC);
    myTime += s;
    myTime += ":";

    //  minutes
    if (minute(time) < 10)
        myTime += "0";

    itoa(minute(time), s, DEC);
    myTime += s;
    myTime += ":";

    //  seconds
    if (second(time) < 10)
        myTime += "0";

    itoa(second(time), s, DEC);
    myTime += s;

    return myTime;
}

String TimeIntervalToString(time_t time)
{

    String myTime = "";
    char s[2];

    //  hours
    itoa((time / 3600), s, DEC);
    myTime += s;
    myTime += ":";

    //  minutes
    if (minute(time) < 10)
        myTime += "0";

    itoa(minute(time), s, DEC);
    myTime += s;
    myTime += ":";

    //  seconds
    if (second(time) < 10)
        myTime += "0";

    itoa(second(time), s, DEC);
    myTime += s;
    return myTime;
}

bool is_authenticated()
{
#ifdef __debugSettings
    return true;
#endif
    if (server.hasHeader("Cookie"))
    {
        String cookie = server.header("Cookie");
        if (cookie.indexOf("EspAuth=1") != -1)
        {
            LogEvent(EVENTCATEGORIES::Authentication, 1, "Success", "");
            return true;
        }
    }
    LogEvent(EVENTCATEGORIES::Authentication, 2, "Failure", "");
    return false;
}

void handleLogin()
{
    String msg = "";
    if (server.hasHeader("Cookie"))
    {
        String cookie = server.header("Cookie");
    }
    if (server.hasArg("DISCONNECT"))
    {
        String header = "HTTP/1.1 301 OK\r\nSet-Cookie: EspAuth=0\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
        server.sendContent(header);
        LogEvent(EVENTCATEGORIES::Login, 1, "Logout", "");
        return;
    }
    if (server.hasArg("username") && server.hasArg("password"))
    {
        if (server.arg("username") == ADMIN_USERNAME && server.arg("password") == ADMIN_PASSWORD)
        {
            String header = "HTTP/1.1 301 OK\r\nSet-Cookie: EspAuth=1\r\nLocation: /status.html\r\nCache-Control: no-cache\r\n\r\n";
            server.sendContent(header);
            LogEvent(EVENTCATEGORIES::Login, 2, "Success", "User name: " + server.arg("username"));
            return;
        }
        msg = "<div class=\"alert alert-danger\"><strong>Error!</strong> Wrong user name and/or password specified.<a href=\"#\" class=\"close\" data-dismiss=\"alert\" aria-label=\"close\">&times;</a></div>";
        LogEvent(EVENTCATEGORIES::Login, 2, "Failure", "User name: " + server.arg("username") + " - Password: " + server.arg("password"));
    }

    time_t localTime = timechangerules::timezones[appConfig.timeZone]->toLocal(now(), &tcr);

    File f = LittleFS.open("/login.html", "r");

    String htmlString;

    if (f.available())
    {
        htmlString = f.readString();
    }
    f.close();

    htmlString.replace("%year%", (String)year(localTime));
    htmlString.replace("%alert%", msg);

    server.send(200, "text/html", htmlString);
    LogEvent(PageHandler, 2, "Page served", "/");
}

void handleStatus()
{

    LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "status.html");
    if (!is_authenticated())
    {
        String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
        server.sendContent(header);
        return;
    }

    time_t localTime = timechangerules::timezones[appConfig.timeZone]->toLocal(now(), &tcr);

    String FirmwareVersionString = String(FIRMWARE_VERSION);

    File f = LittleFS.open("/status.html", "r");

    String htmlString;

    if (f.available())
    {
        htmlString = f.readString();
    }
    f.close();

    //  System information
    htmlString.replace("%year%", (String)year(localTime));
    htmlString.replace("%chipid%", (String)ESP.getChipId());
    htmlString.replace("%hardwareid%", HARDWARE_ID);
    htmlString.replace("%hardwareversion%", HARDWARE_VERSION);
    htmlString.replace("%firmwareid%", SOFTWARE_ID);
    htmlString.replace("%firmwareversion%", FirmwareVersionString);
    htmlString.replace("%uptime%", TimeIntervalToString(millis() / 1000));
    htmlString.replace("%currenttime%", DateTimeToString(localTime));
    htmlString.replace("%lastresetreason%", ESP.getResetReason());
    htmlString.replace("%flashchipsize%", String(ESP.getFlashChipSize()));
    htmlString.replace("%flashchipspeed%", String(ESP.getFlashChipSpeed()));
    htmlString.replace("%freeheapsize%", String(ESP.getFreeHeap()));
    htmlString.replace("%freesketchspace%", String(ESP.getFreeSketchSpace()));
    htmlString.replace("%friendlyname%", appConfig.friendlyName);
    htmlString.replace("%mqtt-topic%", appConfig.mqttTopic);

    //  Network settings
    switch (WiFi.getMode())
    {
    case WIFI_AP:
        htmlString.replace("%wifimode%", "Access Point");
        htmlString.replace("%macaddress%", String(WiFi.softAPmacAddress()));
        htmlString.replace("%networkaddress%", WiFi.softAPIP().toString());
        htmlString.replace("%ssid%", String(WiFi.SSID()));
        htmlString.replace("%subnetmask%", "n/a");
        htmlString.replace("%gateway%", "n/a");
        break;
    case WIFI_STA:
        htmlString.replace("%wifimode%", "Station");
        htmlString.replace("%macaddress%", String(WiFi.macAddress()));
        htmlString.replace("%networkaddress%", WiFi.localIP().toString());
        htmlString.replace("%ssid%", String(WiFi.SSID()));
        htmlString.replace("%channel%", String(WiFi.channel()));
        htmlString.replace("%subnetmask%", WiFi.subnetMask().toString());
        htmlString.replace("%gateway%", WiFi.gatewayIP().toString());
        break;
    default:
        //  This should not happen...
        break;
    }

    server.send(200, "text/html", htmlString);
    LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "status.html");
}

void handleGeneralSettings()
{
    LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "generalsettings.html");

    if (!is_authenticated())
    {
        String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
        server.sendContent(header);
        return;
    }

    if (server.method() == HTTP_POST)
    { //  POST

        bool mqttDirty = false;

        if (server.hasArg("timezoneselector"))
        {
            signed char oldTimeZone = appConfig.timeZone;
            appConfig.timeZone = atoi(server.arg("timezoneselector").c_str());

            adjustTime((appConfig.timeZone - oldTimeZone) * SECS_PER_HOUR);

            LogEvent(EVENTCATEGORIES::TimeZoneChange, 1, "New time zone", "UTC " + server.arg("timezoneselector"));
        }

        if (server.hasArg("friendlyname"))
        {
            strcpy(appConfig.friendlyName, server.arg("friendlyname").c_str());
            LogEvent(EVENTCATEGORIES::FriendlyNameChange, 1, "New friendly name", appConfig.friendlyName);
        }

        if (server.hasArg("heartbeatinterval"))
        {
            os_timer_disarm(&heartbeatTimer);
            appConfig.heartbeatInterval = server.arg("heartbeatinterval").toInt();
            LogEvent(EVENTCATEGORIES::HeartbeatIntervalChange, 1, "New Heartbeat interval", (String)appConfig.heartbeatInterval);
            os_timer_arm(&heartbeatTimer, appConfig.heartbeatInterval * 1000, true);
        }

        //  MQTT settings
        if (server.hasArg("mqttbroker"))
        {
            if ((String)appConfig.mqttServer != server.arg("mqttbroker"))
            {
                mqttDirty = true;
                sprintf(appConfig.mqttServer, "%s", server.arg("mqttbroker").c_str());
                LogEvent(EVENTCATEGORIES::MqttParamChange, 1, "New MQTT broker", appConfig.mqttServer);
            }
        }

        if (server.hasArg("mqttport"))
        {
            if (appConfig.mqttPort != atoi(server.arg("mqttport").c_str()))
            {
                mqttDirty = true;
                appConfig.mqttPort = atoi(server.arg("mqttport").c_str());
                LogEvent(EVENTCATEGORIES::MqttParamChange, 2, "New MQTT port", server.arg("mqttport").c_str());
            }
        }

        if (server.hasArg("mqtttopic"))
        {
            if ((String)appConfig.mqttTopic != server.arg("mqtttopic"))
            {
                mqttDirty = true;
                sprintf(appConfig.mqttTopic, "%s", server.arg("mqtttopic").c_str());
                LogEvent(EVENTCATEGORIES::MqttParamChange, 1, "New MQTT topic", appConfig.mqttTopic);
            }
        }

        if (mqttDirty)
            PSclient.disconnect();

        saveSettings();
        ESP.reset();
    }

    time_t localTime = timechangerules::timezones[appConfig.timeZone]->toLocal(now(), &tcr);

    File f = LittleFS.open("/generalsettings.html", "r");

    String htmlString, timezoneslist, monthlist0, daylist0, monthlist1, daylist1;

    char ss[sizeof(int)];

    for (unsigned long i = 0; i < sizeof(timechangerules::tzDescriptions) / sizeof(timechangerules::tzDescriptions[0]); i++)
    {
        itoa(i, ss, DEC);
        timezoneslist += "<option ";
        if (appConfig.timeZone == (signed char)i)
        {
            timezoneslist += "selected ";
        }
        timezoneslist += "value=\"";
        timezoneslist += ss;
        timezoneslist += "\">";

        timezoneslist += timechangerules::tzDescriptions[i];

        timezoneslist += "</option>";
        timezoneslist += "\n";
    }

    while (f.available())
    {
        htmlString = f.readString();
    }
    f.close();

    htmlString.replace("%year%", (String)year(localTime));
    htmlString.replace("%mqtt-servername%", appConfig.mqttServer);
    htmlString.replace("%mqtt-port%", String(appConfig.mqttPort));
    htmlString.replace("%mqtt-topic%", appConfig.mqttTopic);
    htmlString.replace("%timezoneslist%", timezoneslist);
    htmlString.replace("%friendlyname%", appConfig.friendlyName);
    htmlString.replace("%heartbeatinterval%", (String)appConfig.heartbeatInterval);

    server.send(200, "text/html", htmlString);

    LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "generalsettings.html");
}

void handleLightSettings()
{
    LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "lightsettings.html");

    if (!is_authenticated())
    {
        String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
        server.sendContent(header);
        return;
    }

    if (server.method() == HTTP_POST)
    { //  POST

        // for (size_t i = 0; i < server.args(); i++) {
        //   debug(server.argName(i));
        //   debug(": ");
        //   debugln(server.arg(i));
        // }

        //  Mode
        appConfig.mode = server.arg("optSelectMode").toInt();

        //  Special days
        if (server.hasArg("birthday0-month-selector"))
        {
            appConfig.birthDays[0].month = server.arg("birthday0-month-selector").toInt();
            LogEvent(EVENTCATEGORIES::Clock, 1, "New birthday", (String)appConfig.birthDays[0].month);
        }

        if (server.hasArg("birthday0-day-selector"))
        {
            appConfig.birthDays[0].day = (int)(server.arg("birthday0-day-selector").toInt() - 1);
            LogEvent(EVENTCATEGORIES::Clock, 1, "New birthday", (String)appConfig.birthDays[0].day);
        }

        if (server.hasArg("birthday1-month-selector"))
        {
            appConfig.birthDays[1].month = server.arg("birthday1-month-selector").toInt();
            LogEvent(EVENTCATEGORIES::Clock, 1, "New birthday", (String)appConfig.birthDays[1].month);
        }

        if (server.hasArg("birthday1-day-selector"))
        {
            appConfig.birthDays[1].day = (int)(server.arg("birthday1-day-selector").toInt() - 1);
            LogEvent(EVENTCATEGORIES::Clock, 1, "New birthday", (String)appConfig.birthDays[1].day);
        }

        saveSettings();
        SetColorScheme((OPERATION_MODE)appConfig.mode);
    }

    time_t localTime = timechangerules::timezones[appConfig.timeZone]->toLocal(now(), &tcr);

    File f = LittleFS.open("/lightsettings.html", "r");

    String htmlString, timezoneslist, monthlist0, daylist0, monthlist1, daylist1;

    char ss[sizeof(int)], dd[sizeof(int)];

    for (int i = 0; i < 12; i++)
    {
        itoa(i, ss, DEC);
        monthlist0 += "<option ";
        if (appConfig.birthDays[0].month == i)
        {
            monthlist0 += "selected ";
        }
        monthlist0 += "value=\"";
        monthlist0 += ss;
        monthlist0 += "\">";
        monthlist0 += months[i];
        monthlist0 += "</option>";
        monthlist0 += "\n";
    }

    for (int i = 0; i < 31; i++)
    {
        itoa(i, ss, DEC);
        itoa(i + 1, dd, DEC);
        daylist0 += "<option ";
        if (appConfig.birthDays[0].day == i)
        {
            daylist0 += "selected ";
        }
        daylist0 += "value=\"";
        daylist0 += ss;
        daylist0 += "\">";
        daylist0 += dd;
        daylist0 += "</option>";
        daylist0 += "\n";
    }

    for (int i = 0; i < 12; i++)
    {
        itoa(i, ss, DEC);
        monthlist1 += "<option ";
        if (appConfig.birthDays[1].month == i)
        {
            monthlist1 += "selected ";
        }
        monthlist1 += "value=\"";
        monthlist1 += ss;
        monthlist1 += "\">";
        monthlist1 += months[i];
        monthlist1 += "</option>";
        monthlist1 += "\n";
    }

    for (int i = 0; i < 31; i++)
    {
        itoa(i, ss, DEC);
        itoa(i + 1, dd, DEC);
        daylist1 += "<option ";
        if (appConfig.birthDays[1].day == i)
        {
            daylist1 += "selected ";
        }
        daylist1 += "value=\"";
        daylist1 += ss;
        daylist1 += "\">";
        daylist1 += dd;
        daylist1 += "</option>";
        daylist1 += "\n";
    }

    if (f.available())
    {
        htmlString = f.readString();
    }
    f.close();

    htmlString.replace("%year%", (String)year(localTime));
    htmlString.replace("%mqtt-servername%", appConfig.mqttServer);
    htmlString.replace("%mqtt-port%", String(appConfig.mqttPort));
    htmlString.replace("%mqtt-topic%", appConfig.mqttTopic);
    htmlString.replace("%friendlyname%", appConfig.friendlyName);
    htmlString.replace("%heartbeatinterval%", (String)appConfig.heartbeatInterval);

    htmlString.replace("%checked0%", appConfig.mode == OPERATION_MODE::STATIC_COLOR ? "checked" : "");
    htmlString.replace("%checked1%", appConfig.mode == OPERATION_MODE::STATIC_MONO ? "checked" : "");
    htmlString.replace("%checked2%", appConfig.mode == OPERATION_MODE::GLOWING_COLOR ? "checked" : "");
    htmlString.replace("%checked3%", appConfig.mode == OPERATION_MODE::GLOWING_MONO ? "checked" : "");

    htmlString.replace("%birthday0-day%", (String)(appConfig.birthDays[0].day + 1));
    htmlString.replace("%birthday1-day%", (String)(appConfig.birthDays[1].day + 1));

    htmlString.replace("%birthday0-month-list%", monthlist0);
    htmlString.replace("%birthday1-month-list%", monthlist1);

    server.send(200, "text/html", htmlString);

    LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "lightsettings.html");
}

void handleBirthdaySettings()
{
    LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "birthdaysettings.html");

    if (!is_authenticated())
    {
        String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
        server.sendContent(header);
        return;
    }

    if (server.method() == HTTP_POST)
    {
        // for (size_t i = 0; i < server.args(); i++)
        // {
        //     Serial.print(server.argName(i));
        //     Serial.print(": ");
        //     Serial.println(server.arg(i));
        // }

        if (server.hasArg("birthday0-month-selector"))
        {
            appConfig.birthDays[0].month = server.arg("birthday0-month-selector").toInt();
        }

        if (server.hasArg("birthday0-day-selector"))
        {
            appConfig.birthDays[0].day = (int)(server.arg("birthday0-day-selector").toInt() - 1);
        }

        if (server.hasArg("birthday1-month-selector"))
        {
            appConfig.birthDays[1].month = server.arg("birthday1-month-selector").toInt();
        }

        if (server.hasArg("birthday1-day-selector"))
        {
            appConfig.birthDays[1].day = (int)(server.arg("birthday1-day-selector").toInt() - 1);
        }

        saveSettings();
    }

    time_t localTime = timechangerules::timezones[appConfig.timeZone]->toLocal(now(), &tcr);

    String htmlString, monthlist0, daylist0, monthlist1, daylist1;

    char ss[sizeof(int)], dd[sizeof(int)];

    for (int i = 0; i < 12; i++)
    {
        itoa(i, ss, DEC);
        monthlist0 += "<option ";
        if (appConfig.birthDays[0].month == i)
        {
            monthlist0 += "selected ";
        }
        monthlist0 += "value=\"";
        monthlist0 += ss;
        monthlist0 += "\">";
        monthlist0 += months[i];
        monthlist0 += "</option>";
        monthlist0 += "\n";
    }

    for (int i = 0; i < 31; i++)
    {
        itoa(i, ss, DEC);
        itoa(i + 1, dd, DEC);
        daylist0 += "<option ";
        if (appConfig.birthDays[0].day == i)
        {
            daylist0 += "selected ";
        }
        daylist0 += "value=\"";
        daylist0 += ss;
        daylist0 += "\">";
        daylist0 += dd;
        daylist0 += "</option>";
        daylist0 += "\n";
    }

    for (int i = 0; i < 12; i++)
    {
        itoa(i, ss, DEC);
        monthlist1 += "<option ";
        if (appConfig.birthDays[1].month == i)
        {
            monthlist1 += "selected ";
        }
        monthlist1 += "value=\"";
        monthlist1 += ss;
        monthlist1 += "\">";
        monthlist1 += months[i];
        monthlist1 += "</option>";
        monthlist1 += "\n";
    }

    for (int i = 0; i < 31; i++)
    {
        itoa(i, ss, DEC);
        itoa(i + 1, dd, DEC);
        daylist1 += "<option ";
        if (appConfig.birthDays[1].day == i)
        {
            daylist1 += "selected ";
        }
        daylist1 += "value=\"";
        daylist1 += ss;
        daylist1 += "\">";
        daylist1 += dd;
        daylist1 += "</option>";
        daylist1 += "\n";
    }

    File f = LittleFS.open("/birthdaysettings.html", "r");
    if (f.available())
    {
        htmlString = f.readString();
    }
    f.close();

    htmlString.replace("%year%", (String)year(localTime));

    htmlString.replace("%birthday0-day%", (String)(appConfig.birthDays[0].day + 1));
    htmlString.replace("%birthday1-day%", (String)(appConfig.birthDays[1].day + 1));

    htmlString.replace("%birthday0-month-list%", monthlist0);
    htmlString.replace("%birthday1-month-list%", monthlist1);

    server.send(200, "text/html", htmlString);
    LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "birthdaysettings.html");
}

void handleNetworkSettings()
{
    LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "networksettings.html");

    if (!is_authenticated())
    {
        String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
        server.sendContent(header);
        return;
    }

    if (server.method() == HTTP_POST)
    { //  POST
        if (server.hasArg("ssid"))
        {
            strcpy(appConfig.ssid, server.arg("ssid").c_str());
            strcpy(appConfig.password, server.arg("password").c_str());
            saveSettings();

            isAccessPoint = false;
            connectionState = CONNECTION_STATE::STATE_CHECK_WIFI_CONNECTION;
            WiFi.disconnect(false);

            ESP.reset();
        }
    }

    time_t localTime = timechangerules::timezones[appConfig.timeZone]->toLocal(now(), &tcr);

    File f = LittleFS.open("/networksettings.html", "r");
    String htmlString, wifiList;

    byte numberOfNetworks = WiFi.scanNetworks();
    for (size_t i = 0; i < numberOfNetworks; i++)
    {
        wifiList += "<div class=\"radio\"><label><input ";
        if (i == 0)
            wifiList += "id=\"ssid\" ";

        wifiList += "type=\"radio\" name=\"ssid\" value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</label></div>";
    }

    while (f.available())
    {
        htmlString = f.readString();
    }
    f.close();

    htmlString.replace("%year%", (String)year(localTime));
    htmlString.replace("%wifilist%", wifiList);

    server.send(200, "text/html", htmlString);

    LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "networksettings.html");
}

void handleTools()
{
    LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "tools.html");

    if (!is_authenticated())
    {
        String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
        server.sendContent(header);
        return;
    }

    if (server.method() == HTTP_POST)
    { //  POST

        if (server.hasArg("reset"))
        {
            LogEvent(EVENTCATEGORIES::Reboot, 1, "Reset", "");
            defaultSettings();
            ESP.reset();
        }

        if (server.hasArg("restart"))
        {
            LogEvent(EVENTCATEGORIES::Reboot, 2, "Restart", "");
            ESP.reset();
        }
    }

    time_t localTime = timechangerules::timezones[appConfig.timeZone]->toLocal(now(), &tcr);

    File f = LittleFS.open("/tools.html", "r");

    String htmlString;

    if (f.available())
    {
        htmlString = f.readString();
    }
    f.close();

    htmlString.replace("%year%", (String)year(localTime));

    server.send(200, "text/html", htmlString);

    LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "tools.html");
}

/*
    for (size_t i = 0; i < server.args(); i++) {
      debug(server.argName(i));
      debug(": ");
      debugln(server.arg(i));
    }
*/

void handleNotFound()
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

void SendHeartbeat()
{

    if (PSclient.connected())
    {

        time_t localTime = timechangerules::timezones[appConfig.timeZone]->toLocal(now(), &tcr);

        const size_t capacity = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + 200;
        DynamicJsonDocument doc(capacity);

        JsonObject sysDetails = doc.createNestedObject("System");
        sysDetails["Time"] = DateTimeToString(localTime);
        sysDetails["Node"] = ESP.getChipId();
        sysDetails["Freeheap"] = ESP.getFreeHeap();
        sysDetails["FriendlyName"] = appConfig.friendlyName;
        sysDetails["HeartbeatInterval"] = appConfig.heartbeatInterval;

        JsonObject wifiDetails = doc.createNestedObject("Wifi");
        wifiDetails["SSId"] = String(WiFi.SSID());
        wifiDetails["MACAddress"] = String(WiFi.macAddress());
        wifiDetails["IPAddress"] = WiFi.localIP().toString();

        JsonObject specialDayDetails = doc.createNestedObject("SpecialDays");
        char SpecialDay[20];
        sprintf(SpecialDay, "%i, %s", appConfig.birthDays[0].day + 1, months[appConfig.birthDays[0].month]);
        specialDayDetails["First"] = SpecialDay;
        sprintf(SpecialDay, "%i, %s", appConfig.birthDays[1].day + 1, months[appConfig.birthDays[1].month]);
        specialDayDetails["Second"] = SpecialDay;

#ifdef __debugSettings
        serializeJsonPretty(doc, Serial);
        Serial.println();
#endif

        String myJsonString;

        serializeJson(doc, myJsonString);
        PSclient.publish((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/HEARTBEAT").c_str(), myJsonString.c_str(), false);
    }

    needsHeartbeat = false;
}

void RefreshDisplay()
{

    //  Calculate time
    time_t localTime = timechangerules::timezones[appConfig.timeZone]->toLocal(now(), &tcr);
    int8_t myHours = hourFormat12(localTime);
    int8_t myMinutes = minute(localTime);
    // int8_t mySeconds = second(localTime);
    int8_t myDayOfWeek = weekday(localTime);

    // myHours = 12;
    // myMinutes = 46;

    // Serial.printf("%i:%i:%i:%i\r\n", myHours, myMinutes, mySeconds, myDayOfWeek);

    //  Clear screen
    strip.ClearTo(RgbColor(0));
    for (unsigned int i = 0; i < sizeof(segments) / sizeof(segments[0]); i++)
        segments[i].isActive = false;

    //  IT IS
    segments[ANIMATION_CHANNEL::SENTENCE].isActive = true;
    segments[ANIMATION_CHANNEL::MONDAY].isActive = true;

    //  Minutes
    if (myMinutes >= 0 && myMinutes < 5)
    {
        segments[ANIMATION_CHANNEL::OCLOCK].isActive = true;
        // for (unsigned long i = 0; i < sizeof(woClock)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(woClock[i], colorGamma.Correct(colSentence));
    }

    if (myMinutes >= 5 && myMinutes < 10)
    {
        segments[ANIMATION_CHANNEL::FIVE_MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::PAST].isActive = true;
        // for (unsigned long i = 0; i < sizeof(wFiveMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wFiveMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wPast)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wPast[i], colorGamma.Correct(colPastTo));
    }

    if (myMinutes >= 10 && myMinutes < 15)
    {
        segments[ANIMATION_CHANNEL::TEN_MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::PAST].isActive = true;
        // for (unsigned long i = 0; i < sizeof(wTenMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wTenMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wPast)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wPast[i], colorGamma.Correct(colPastTo));
    }

    if (myMinutes >= 15 && myMinutes < 20)
    {
        segments[ANIMATION_CHANNEL::QUARTER].isActive = true;
        segments[ANIMATION_CHANNEL::PAST].isActive = true;
        // for (unsigned long i = 0; i < sizeof(wQuarter)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wQuarter[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wPast)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wPast[i], colorGamma.Correct(colPastTo));
    }

    if (myMinutes >= 20 && myMinutes < 25)
    {
        segments[ANIMATION_CHANNEL::TWENTY].isActive = true;
        segments[ANIMATION_CHANNEL::MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::PAST].isActive = true;
        // for (unsigned long i = 0; i < sizeof(wTwenty)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wTwenty[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wPast)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wPast[i], colorGamma.Correct(colPastTo));
    }

    if (myMinutes >= 25 && myMinutes < 30)
    {
        segments[ANIMATION_CHANNEL::TWENTY].isActive = true;
        segments[ANIMATION_CHANNEL::FIVE_MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::PAST].isActive = true;
        // for (unsigned long i = 0; i < sizeof(wTwenty)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wTwenty[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wFiveMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wFiveMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wPast)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wPast[i], colorGamma.Correct(colPastTo));
    }

    if (myMinutes >= 30 && myMinutes < 35)
    {
        segments[ANIMATION_CHANNEL::HALF].isActive = true;
        segments[ANIMATION_CHANNEL::PAST].isActive = true;
        // for (unsigned long i = 0; i < sizeof(wHalf)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wHalf[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wPast)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wPast[i], colorGamma.Correct(colPastTo));
    }

    if (myMinutes >= 35 && myMinutes < 40)
    {
        segments[ANIMATION_CHANNEL::TWENTY].isActive = true;
        segments[ANIMATION_CHANNEL::FIVE_MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::TO].isActive = true;
        // for (unsigned long i = 0; i < sizeof(wTwenty)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wTwenty[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wFiveMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wFiveMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wTo)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wTo[i], colorGamma.Correct(colPastTo));
    }

    if (myMinutes >= 40 && myMinutes < 45)
    {
        segments[ANIMATION_CHANNEL::TWENTY].isActive = true;
        segments[ANIMATION_CHANNEL::MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::TO].isActive = true;
        // for (unsigned long i = 0; i < sizeof(wTwenty)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wTwenty[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wTo)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wTo[i], colorGamma.Correct(colPastTo));
    }

    if (myMinutes >= 45 && myMinutes < 50)
    {
        segments[ANIMATION_CHANNEL::QUARTER].isActive = true;
        segments[ANIMATION_CHANNEL::TO].isActive = true;
        // for (unsigned long i = 0; i < sizeof(wQuarter)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wQuarter[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wTo)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wTo[i], colorGamma.Correct(colPastTo));
    }

    if (myMinutes >= 50 && myMinutes < 55)
    {
        segments[ANIMATION_CHANNEL::TEN_MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::TO].isActive = true;
        // for (unsigned long i = 0; i < sizeof(wTenMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wTenMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wTo)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wTo[i], colorGamma.Correct(colPastTo));
    }

    if (myMinutes >= 55 && myMinutes < 60)
    {
        segments[ANIMATION_CHANNEL::FIVE_MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::MINUTES].isActive = true;
        segments[ANIMATION_CHANNEL::TO].isActive = true;
        // for (unsigned long i = 0; i < sizeof(wFiveMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wFiveMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wMinutes)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wMinutes[i], colorGamma.Correct(colMinutes));
        // for (unsigned long i = 0; i < sizeof(wTo)/sizeof(unsigned int); i++)
        //     strip.SetPixelColor(wTo[i], colorGamma.Correct(colPastTo));
    }

    //  Hours
    if (myMinutes >= 0 && myMinutes < 35)
    {
        switch (myHours)
        {
        case 1:
            segments[ANIMATION_CHANNEL::ONE].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wOne)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wOne[i], colorGamma.Correct(colHours));
            break;
        case 2:
            segments[ANIMATION_CHANNEL::TWO].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wTwo)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wTwo[i], colorGamma.Correct(colHours));
            break;
        case 3:
            segments[ANIMATION_CHANNEL::THREE].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wThree)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wThree[i], colorGamma.Correct(colHours));
            break;
        case 4:
            segments[ANIMATION_CHANNEL::FOUR].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wFour)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wFour[i], colorGamma.Correct(colHours));
            break;
        case 5:
            segments[ANIMATION_CHANNEL::FIVE].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wFive)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wFive[i], colorGamma.Correct(colHours));
            break;
        case 6:
            segments[ANIMATION_CHANNEL::SIX].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wSix)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wSix[i], colorGamma.Correct(colHours));
            break;
        case 7:
            segments[ANIMATION_CHANNEL::SEVEN].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wSeven)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wSeven[i], colorGamma.Correct(colHours));
            break;
        case 8:
            segments[ANIMATION_CHANNEL::EIGHT].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wEight)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wEight[i], colorGamma.Correct(colHours));
            break;
        case 9:
            segments[ANIMATION_CHANNEL::NINE].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wNine)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wNine[i], colorGamma.Correct(colHours));
            break;
        case 10:
            segments[ANIMATION_CHANNEL::TEN].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wTen)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wTen[i], colorGamma.Correct(colHours));
            break;
        case 11:
            segments[ANIMATION_CHANNEL::ELEVEN].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wEleven)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wEleven[i], colorGamma.Correct(colHours));
            break;
        case 12:
            segments[ANIMATION_CHANNEL::TWELVE].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wTwelve)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wTwelve[i], colorGamma.Correct(colHours));
            break;

        default:
            break;
        }
    }
    else
    {
        switch ((myHours + 1) % 12)
        {
        case 0:
            segments[ANIMATION_CHANNEL::TWELVE].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wTwelve)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wTwelve[i], colorGamma.Correct(colHours));
            break;
        case 1:
            segments[ANIMATION_CHANNEL::ONE].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wOne)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wOne[i], colorGamma.Correct(colHours));
            break;
        case 2:
            segments[ANIMATION_CHANNEL::TWO].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wTwo)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wTwo[i], colorGamma.Correct(colHours));
            break;
        case 3:
            segments[ANIMATION_CHANNEL::THREE].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wThree)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wThree[i], colorGamma.Correct(colHours));
            break;
        case 4:
            segments[ANIMATION_CHANNEL::FOUR].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wFour)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wFour[i], colorGamma.Correct(colHours));
            break;
        case 5:
            segments[ANIMATION_CHANNEL::FIVE].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wFive)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wFive[i], colorGamma.Correct(colHours));
            break;
        case 6:
            segments[ANIMATION_CHANNEL::SIX].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wSix)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wSix[i], colorGamma.Correct(colHours));
            break;
        case 7:
            segments[ANIMATION_CHANNEL::SEVEN].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wSeven)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wSeven[i], colorGamma.Correct(colHours));
            break;
        case 8:
            segments[ANIMATION_CHANNEL::EIGHT].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wEight)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wEight[i], colorGamma.Correct(colHours));
            break;
        case 9:
            segments[ANIMATION_CHANNEL::NINE].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wNine)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wNine[i], colorGamma.Correct(colHours));
            break;
        case 10:
            segments[ANIMATION_CHANNEL::TEN].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wTen)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wTen[i], colorGamma.Correct(colHours));
            break;
        case 11:
            segments[ANIMATION_CHANNEL::ELEVEN].isActive = true;
            // for (unsigned long i = 0; i < sizeof(wEleven)/sizeof(unsigned int); i++)
            //     strip.SetPixelColor(wEleven[i], colorGamma.Correct(colHours));
            break;

        default:
            break;
        }
    }

    //  Days
    for (int8_t i = 0; i < 7; i++)
    {
        segments[ANIMATION_CHANNEL::SUNDAY + i].isActive = myDayOfWeek == i + 1;
    }

    //  Semaphores
    if (semaphores[SEMAPHOR::sem0])
        segments[ANIMATION_CHANNEL::SEMAPHORE0].isActive = true;
    if (semaphores[SEMAPHOR::sem1])
        segments[ANIMATION_CHANNEL::SEMAPHORE1].isActive = true;
    if (semaphores[SEMAPHOR::sem2])
        segments[ANIMATION_CHANNEL::SEMAPHORE2].isActive = true;
    if (semaphores[SEMAPHOR::sem3])
        segments[ANIMATION_CHANNEL::SEMAPHORE3].isActive = true;
    if (semaphores[SEMAPHOR::sem4])
        segments[ANIMATION_CHANNEL::SEMAPHORE4].isActive = true;
    if (semaphores[SEMAPHOR::sem5])
        segments[ANIMATION_CHANNEL::SEMAPHORE5].isActive = true;
}

void DisplayGreetings(int BirthDayNumber)
{

    //  Clear screen
    strip.ClearTo(RgbColor(0));
    for (unsigned int i = 0; i < sizeof(segments) / sizeof(segments[0]); i++)
        segments[i].isActive = false;

    segments[ANIMATION_CHANNEL::HAPPY].isActive = true;
    segments[ANIMATION_CHANNEL::BIRTH].isActive = true;
    segments[ANIMATION_CHANNEL::DAY].isActive = true;
    switch (BirthDayNumber)
    {
    case 0:
        segments[ANIMATION_CHANNEL::NAME0].isActive = true;
        break;
    case 1:
        segments[ANIMATION_CHANNEL::NAME1].isActive = true;
        break;

    default:
        break;
    }
}

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{

    debug("Topic:\t\t");
    debugln(topic);

    debug("Payload:\t");
    for (unsigned int i = 0; i < length; i++)
    {
        debug((char)payload[i]);
    }
    debugln();

    StaticJsonDocument<JSON_MQTT_COMMAND_SIZE> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        debugln("Failed to parse incoming string.");
        debugln(error.c_str());
        for (size_t i = 0; i < 10; i++)
        {
            digitalWrite(CONNECTION_STATUS_LED_GPIO, !digitalRead(CONNECTION_STATUS_LED_GPIO));
            delay(50);
        }
    }
    else
    {
        //  It IS a JSON string

#ifdef __debugSettings
        serializeJsonPretty(doc, Serial);
        debugln();
#endif

        //  Semaphores
        if (doc.containsKey("SEMAPHORE0"))
        {
            semaphores[SEMAPHOR::sem0] = (bool)doc["SEMAPHORE0"] == true;
        }
        if (doc.containsKey("SEMAPHORE1"))
        {
            semaphores[SEMAPHOR::sem1] = (bool)doc["SEMAPHORE1"] == true;
        }
        if (doc.containsKey("SEMAPHORE2"))
        {
            semaphores[SEMAPHOR::sem2] = (bool)doc["SEMAPHORE2"] == true;
        }
        if (doc.containsKey("SEMAPHORE3"))
        {
            semaphores[SEMAPHOR::sem3] = (bool)doc["SEMAPHORE3"] == true;
        }
        if (doc.containsKey("SEMAPHORE4"))
        {
            semaphores[SEMAPHOR::sem4] = (bool)doc["SEMAPHORE4"] == true;
        }
        if (doc.containsKey("SEMAPHORE5"))
        {
            semaphores[SEMAPHOR::sem5] = (bool)doc["SEMAPHORE5"] == true;
        }

        //  reset
        if (doc.containsKey("reset"))
        {
            LogEvent(EVENTCATEGORIES::MqttMsg, 1, "Reset", "");
            defaultSettings();
            ESP.reset();
        }

        //  restart
        if (doc.containsKey("restart"))
        {
            LogEvent(EVENTCATEGORIES::MqttMsg, 2, "Restart", "");
            ESP.reset();
        }
    }
}

void setup()
{
    delay(1); //  Needed for PlatformIO serial monitor
    Serial.begin(DEBUG_SPEED);
    Serial.setDebugOutput(false);
    Serial.printf("\n\n\n\rBooting node:\t%u...\r\n", ESP.getChipId());
    Serial.printf("Hardware ID:\t\t%s\r\nHardware version:\t%s\r\nSoftware ID:\t\t%s\r\nSoftware version:\t%s\r\n\n", HARDWARE_ID, HARDWARE_VERSION, SOFTWARE_ID, FIRMWARE_VERSION);

    //  File system
    if (!LittleFS.begin())
    {
        Serial.printf("Error: Failed to initialize the filesystem!\r\n");
    }

    if (!loadSettings(appConfig))
    {
        Serial.printf("Failed to load config, creating default settings...");
        defaultSettings();
    }
    else
    {
        debugln("Config loaded.");
    }

    //  GPIO
    pinMode(CONNECTION_STATUS_LED_GPIO, OUTPUT);
    digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);

    //  OTA
    ArduinoOTA.onStart([]()
                       { debugln("OTA started."); });

    ArduinoOTA.onEnd([]()
                     { debugln("\nOTA finished."); });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        if (progress % OTA_BLINKING_RATE == 0){
        if (digitalRead(CONNECTION_STATUS_LED_GPIO)==HIGH)
            digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
            else
            digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
        } });

    ArduinoOTA.onError([](ota_error_t error)
                       {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) debugln("Authentication failed.");
        else if (error == OTA_BEGIN_ERROR) debugln("Begin failed.");
        else if (error == OTA_CONNECT_ERROR) debugln("Connect failed.");
        else if (error == OTA_RECEIVE_ERROR) debugln("Receive failed.");
        else if (error == OTA_END_ERROR) debugln("End failed."); });

    ArduinoOTA.begin();

    debugln();

    server.on("/", handleStatus);
    server.on("/status.html", handleStatus);
    server.on("/generalsettings.html", handleGeneralSettings);
    server.on("/lightsettings.html", handleLightSettings);
    server.on("/birthdaysettings.html", handleBirthdaySettings);
    server.on("/networksettings.html", handleNetworkSettings);
    server.on("/tools.html", handleTools);
    server.on("/login.html", handleLogin);

    server.onNotFound(handleNotFound);

    //  Start HTTP (web) server
    server.begin();
    debugln("HTTP server started.");

    //  Authenticate HTTP requests
    const char *headerkeys[] = {"User-Agent", "Cookie"};
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
    server.collectHeaders(headerkeys, headerkeyssize);

    //  Timers
    os_timer_setfn(&heartbeatTimer, heartbeatTimerCallback, NULL);
    os_timer_arm(&heartbeatTimer, appConfig.heartbeatInterval * 1000, true);

    //  Randomizer
    SetRandomSeed();

    //  Neopixel
    strip.Begin();
    InitSegments();
    SetColorScheme((OPERATION_MODE)appConfig.mode);

    DoBootAnimation();

    // Set the initial connection state
    connectionState = CONNECTION_STATE::STATE_CHECK_WIFI_CONNECTION;

    Serial.printf("Setup completed.\r\n\r\n");
}

void loop()
{

    if (isAccessPoint)
    {
        if (!isAccessPointCreated)
        {
            Serial.printf("Could not connect to WiFI network %s.\r\nReverting to Access Point mode.\r\n", appConfig.ssid);

            delay(500);

            WiFi.mode(WiFiMode::WIFI_AP);
            WiFi.softAP(localHost, DEFAULT_PASSWORD);

            IPAddress myIP;
            myIP = WiFi.softAPIP();
            isAccessPointCreated = true;

            if (MDNS.begin(appConfig.mqttTopic))
                debugln("MDNS responder started.");

            Serial.println("Access point created. Use the following information to connect to the ESP device, then follow the on-screen instructions to connect to a different wifi network:");

            Serial.print("SSID:\t\t\t");
            Serial.println(localHost);

            Serial.print("Password:\t\t");
            Serial.println(DEFAULT_PASSWORD);

            Serial.print("Access point address:\t");
            Serial.println(myIP);

            Serial.println();
            Serial.println("Note: The device will reset in 5 minutes.");

            os_timer_setfn(&accessPointTimer, accessPointTimerCallback, NULL);
            os_timer_arm(&accessPointTimer, ACCESS_POINT_TIMEOUT, true);
            os_timer_disarm(&heartbeatTimer);
        }
        server.handleClient();
    }
    else
    {
        switch (connectionState)
        {

        // Check the WiFi connection
        case STATE_CHECK_WIFI_CONNECTION:

            // Are we connected ?
            if (WiFi.status() != WL_CONNECTED)
            {
                // Wifi is NOT connected
                digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
                connectionState = CONNECTION_STATE::STATE_WIFI_CONNECT;
            }
            else
            {
                // Wifi is connected so check Internet
                digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
                connectionState = CONNECTION_STATE::STATE_CHECK_INTERNET_CONNECTION;

                server.handleClient();
            }
            break;

        // No Wifi so attempt WiFi connection
        case STATE_WIFI_CONNECT:
        {
            // Indicate NTP no yet initialized
            ntpInitialized = false;

            digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
            Serial.printf("Trying to connect to WIFI network: %s", appConfig.ssid);

            // Set station mode
            WiFi.mode(WIFI_STA);

            // Start connection process
            WiFi.hostname(localHost);
            WiFi.begin(appConfig.ssid, appConfig.password);

            // Initialize iteration counter
            uint8_t attempt = 0;

            while ((WiFi.status() != WL_CONNECTED) && (attempt++ < WIFI_CONNECTION_TIMEOUT))
            {
                digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
                Serial.printf(".");
                delay(50);
                digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
                delay(950);
            }
            if (attempt >= WIFI_CONNECTION_TIMEOUT)
            {
                Serial.printf("\r\nCould not connect to WiFi");
                delay(100);

                isAccessPoint = true;

                break;
            }
            digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
            Serial.printf(" Success!\r\n");
            Serial.printf("IP address: ");
            Serial.println(WiFi.localIP());
            if (MDNS.begin(appConfig.mqttTopic))
                debugln("MDNS responder started.");

            connectionState = CONNECTION_STATE::STATE_CHECK_INTERNET_CONNECTION;
        }
        break;

        case STATE_CHECK_INTERNET_CONNECTION:

            // Do we have a connection to the Internet ?
            if (checkInternetConnection())
            {
                // We have an Internet connection

                if (!ntpInitialized)
                {
                    // We are connected to the Internet for the first time so set NTP provider
                    ntp::setup();

                    ntpInitialized = true;

                    debugln("Connected to the Internet.");
                }

                connectionState = CONNECTION_STATE::STATE_INTERNET_CONNECTED;
            }
            else
            {
                connectionState = CONNECTION_STATE::STATE_CHECK_WIFI_CONNECTION;
            }
            break;

        case STATE_INTERNET_CONNECTED:

            ArduinoOTA.handle();

            if (!PSclient.connected())
            {
                PSclient.setServer(appConfig.mqttServer, appConfig.mqttPort);
                if (PSclient.connect(localHost, (MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/STATE").c_str(), 0, true, "offline"))
                {
                    PSclient.setBufferSize(10240);
                    PSclient.setCallback(mqtt_callback);

                    PSclient.subscribe((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/cmnd").c_str(), 0);

                    PSclient.publish((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/STATE").c_str(), "online", true);
                    LogEvent(EVENTCATEGORIES::Conn, 1, "Node online", WiFi.localIP().toString());
                }
            }

            if (PSclient.connected())
            {
                PSclient.loop();
            }

            if (needsHeartbeat)
            {
                SendHeartbeat();
                needsHeartbeat = false;
            }

            ntp::loop();

            time_t localTime = timechangerules::timezones[appConfig.timeZone]->toLocal(now(), &tcr);

            if (isGreetingsModeOn)
            {
                if (greetingsTime + GREETINGS_LENGTH < millis())
                {
                    isGreetingsModeOn = false;
                    oldMillisGreetings = millis();
                }
            }
            else
            {

                if (oldMillis + 1000 < millis())
                {
                    RefreshDisplay();
                    oldMillis = millis();
                }

                if ((((appConfig.birthDays[0].month + 1 == month(localTime)) && (appConfig.birthDays[0].day + 1 == day(localTime))) || (((appConfig.birthDays[1].month + 1 == month(localTime)) && (appConfig.birthDays[1].day + 1 == day(localTime))))) && (millis() - oldMillisGreetings > GREETINGS_INTERVAL))
                {
                    isGreetingsModeOn = true;
                    greetingsTime = millis();
                    if ((appConfig.birthDays[0].month + 1 == month(localTime)) && (appConfig.birthDays[0].day + 1 == day(localTime)))
                    {
                        DisplayGreetings(0);
                    }

                    if ((appConfig.birthDays[1].month + 1 == month(localTime)) && (appConfig.birthDays[1].day + 1 == day(localTime)))
                    {
                        DisplayGreetings(1);
                    }
                }
            }

            for (unsigned int i = 0; i < ANIMATION_CHANNEL::SIZE_OF_ENUM; i++)
            {
                if (!segments[i].isActive)
                {
                    animations.StopAnimation(i);
                }
                if (!animations.IsAnimationActive(i) && segments[i].isActive)
                {
                    GlowSegment(i, .5);
                }
            }

            animations.UpdateAnimations();
            strip.Show();

            // Set next connection state
            connectionState = CONNECTION_STATE::STATE_CHECK_WIFI_CONNECTION;
            break;
        }
    }
}
