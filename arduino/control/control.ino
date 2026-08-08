#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);


const int LED_PIN = 2;

const float BRAKE_THRESHOLD = -2.0;

const float FILTER = 0.85;

const int BRAKE_DELAY = 50;

const float FALL_ANGLE = 60.0;

const float IMPACT_THRESHOLD = 18.0;

const int FALL_DELAY = 120;

const int LED_TIME = 3000;



float xFilter = 0;
float yFilter = 0;
float zFilter = 0;

unsigned long brakeStart = 0;
unsigned long fallStart = 0;
unsigned long ledStart = 0;



void setup()
{
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);

    if(!accel.begin())
    {
        Serial.println("ADXL345 ERROR");
        while(1);
    }

    accel.setRange(ADXL345_RANGE_16_G);

    Serial.println("READY");
}

//========================

void loop()
{
    sensors_event_t event;
    accel.getEvent(&event);



    xFilter = FILTER * xFilter + (1.0 - FILTER) * event.acceleration.x;
    yFilter = FILTER * yFilter + (1.0 - FILTER) * event.acceleration.y;
    zFilter = FILTER * zFilter + (1.0 - FILTER) * event.acceleration.z;



    bool brake = false;

    if(zFilter < BRAKE_THRESHOLD)
    {
        if(brakeStart == 0)
            brakeStart = millis();

        if(millis() - brakeStart > BRAKE_DELAY)
            brake = true;
    }
    else
    {
        brakeStart = 0;
    }


    float angle =
    atan2(
        sqrt(xFilter*xFilter + yFilter*yFilter),
        abs(zFilter)
    ) * 180.0 / PI;

    float impact =
    sqrt(
        xFilter*xFilter +
        yFilter*yFilter +
        zFilter*zFilter
    );


    bool fall = false;

    if(angle > FALL_ANGLE &&
       impact > IMPACT_THRESHOLD)
    {
        if(fallStart == 0)
            fallStart = millis();

        if(millis() - fallStart > FALL_DELAY)
            fall = true;
    }
    else
    {
        fallStart = 0;
    }

    if(brake || fall)
    {
        ledStart = millis();
    }

    if(millis() - ledStart < LED_TIME)
        digitalWrite(LED_PIN, HIGH);
    else
        digitalWrite(LED_PIN, LOW);


    Serial.print("X:");
    Serial.print(xFilter,2);

    Serial.print("  Y:");
    Serial.print(yFilter,2);

    Serial.print("  Z:");
    Serial.print(zFilter,2);

    Serial.print("  Angle:");
    Serial.print(angle,1);

    Serial.print("  Impact:");
    Serial.print(impact,1);

    Serial.print("  Brake:");
    Serial.print(brake);

    Serial.print("  Fall:");
    Serial.println(fall);

    delay(20);
}