#ifndef ENUMS_H
#define ENUMS_H

enum BUTTON_STATES {
  BUTTON_NOT_PRESSED,    //  button not pressed
  BUTTON_BOUNCING,       //  button in not stable state
  BUTTON_PRESSED,        //  button pressed
  BUTTON_LONG_PRESSED    //  button pressed for at least BUTTON_LONG_PRESS_TRESHOLD milliseconds
};

// Connection FSM operational states
enum CONNECTION_STATE {
  STATE_CHECK_WIFI_CONNECTION,
  STATE_WIFI_CONNECT,
  STATE_CHECK_INTERNET_CONNECTION,
  STATE_INTERNET_CONNECTED
};

//  Event categories for logs
enum EVENTCATEGORIES
{
    System,
    Conn,
    MqttMsg,
    NtpTime,
    Reboot,
    Authentication,
    Login,
    PageHandler,
    RefreshSunsetSunrise,
    ReadTemp,
    MqttParamChange,
    TemperatureInterval,
    Hall,
    DSTChange,
    TimeZoneChange,
    IRreceived,
    Lights,
    EntranceLight,
    PwmAutoChange,
    StaircaselightDelay,
    StaircaseLight,
    I2CButtonPressed,
    FriendlyNameChange,
    HeartbeatIntervalChange,
    Relay,
    Clock,
    ArchlightRotaryEncoderDirection,
    BoilerDelay,
    Boiler
};


//  Modes / operational states
enum OPERATION_MODE{
    STATIC_COLOR,
    STATIC_MONO,
    GLOWING_COLOR,
    GLOWING_MONO
};

//  Semaphores
enum SEMAPHOR{
    sem0,
    sem1,
    sem2,
    sem3,
    sem4,
    sem5
};

//  Animation channels
enum ANIMATION_CHANNEL{
    SENTENCE,
    TEN_MINUTES,
    HALF,
    QUARTER,
    TWENTY,
    FIVE_MINUTES,
    MINUTES,
    HAPPY,
    PAST,
    TO,
    BIRTH,
    DAY,
    OCLOCK,
    ONE,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    TEN,
    ELEVEN,
    TWELVE,
    NAME0,
    NAME1,
    LOGO,
    SUNDAY,
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
    SEMAPHORE0,
    SEMAPHORE1,
    SEMAPHORE2,
    SEMAPHORE3,
    SEMAPHORE4,
    SEMAPHORE5,
    SIZE_OF_ENUM    //  This must be the last item!!!
};

#endif