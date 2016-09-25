/*
 * FILE:        DHT_2sensor.ino
 * VERSION:     0.3
 * PURPOSE:     Example that uses DHT library with two sensors
 * LICENSE:     GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *
 * Calls acquire on two sensors and monitors the results for long term
 * analysis.  It uses DHT.acquire and DHT.acquiring
 * Also keeps track of the time to complete the acquire and tracks errors
 *
 * Scott Piette (Piette Technologies) scott.piette@gmail.com
 *      January 2014        Original Spark Port
 *      October 2014        Added support for DHT21/22 sensors
 *                          Improved timing, moved FP math out of ISR
 */
// NOTE DHT_REPORT_TIMING requires DHT_DEBUG_TIMING in PietteTech_DHT.h for debugging edge->edge timings
//#define DHT_REPORT_TIMING

#include "PietteTech_DHT/PietteTech_DHT.h"

#define DHTTYPEA  DHT22       // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPINA   3           // Digital pin for comunications
#define DHTTYPEB  DHT11       // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPINB   2           // Digital pin for comunications

//declaration
void dht_wrapperA(); // must be declared before the lib initialization
void dht_wrapperB(); // must be declared before the lib initialization

// Lib instantiate
PietteTech_DHT DHTA(DHTPINA, DHTTYPEA, dht_wrapperA);
PietteTech_DHT DHTB(DHTPINB, DHTTYPEB, dht_wrapperB);
int n;      // counter

#define LOOP_DELAY 5000 // 5s intervals
int _sensorA_error_count;
int _sensorB_error_count;
int _spark_error_count;
unsigned long _lastTimeInLoop;
bool active_wrapper;    // NOTE: Do not remove this, it prevents compiler from optimizing

// This wrapper is in charge of calling
// must be defined like this for the lib work
void dht_wrapperA() {
    active_wrapper = true;      // NOTE: Do not remove this, it prevents compiler from optimizing
    DHTA.isrCallback();
}

// This wrapper is in charge of calling
// must be defined like this for the lib work
void dht_wrapperB() {
    active_wrapper = false;      // NOTE: Do not remove this, it prevents compiler from optimizing
    DHTB.isrCallback();
}

void setup()
{
    Serial.begin(9600);
    while (!Serial.available()) {
        Serial.println("Press any key to start.");
        delay (1000);
    }
    Serial.println("DHT Example program using 2 DHT sensors");
    Serial.println("---> DHT.acquire and DHT.aquiring");
    Serial.print("LIB version: ");
    Serial.println(DHTLIB_VERSION);
    Serial.println("---------------");

    delay(1000);        // Delay 1s to let the sensors settle
    _lastTimeInLoop = millis();
}

#if defined(DHT_REPORT_TIMING)
// This function will report the timings collected
void printEdgeTiming(class PietteTech_DHT *_d) {
    byte n;
    volatile uint8_t *_e = &_d->_edges[0];
    
    Serial.print("Edge timing = ");
    for (n = 0; n < 41; n++) {
        Serial.print(*_e++);
        if (n < 40)
            Serial.print(".");
    }
    Serial.print("\n\r");
}
#endif

void printSensorData(class PietteTech_DHT *_d) {
    int result = _d->getStatus();

    if (result != DHTLIB_OK)
        if (_d == &DHTA)
            _sensorA_error_count++;
        else
            _sensorB_error_count++;
        
    switch (result) {
        case DHTLIB_OK:
            Serial.println("OK");
            break;
        case DHTLIB_ERROR_CHECKSUM:
            Serial.println("Error\n\r\tChecksum error");
            break;
        case DHTLIB_ERROR_ISR_TIMEOUT:
            Serial.println("Error\n\r\tISR time out error");
            break;
        case DHTLIB_ERROR_RESPONSE_TIMEOUT:
            Serial.println("Error\n\r\tResponse time out error");
            break;
        case DHTLIB_ERROR_DATA_TIMEOUT:
            Serial.println("Error\n\r\tData time out error");
            break;
        case DHTLIB_ERROR_ACQUIRING:
            Serial.println("Error\n\r\tAcquiring");
            break;
        case DHTLIB_ERROR_DELTA:
            Serial.println("Error\n\r\tDelta time to small");
            break;
        case DHTLIB_ERROR_NOTSTARTED:
            Serial.println("Error\n\r\tNot started");
            break;
        default:
            Serial.println("Unknown error");
            break;
    }

#if defined(DHT_REPORT_TIMING)
    // print debug timing information
    printEdgeTiming(_d);
#endif

    Serial.print("Humidity (%): ");
    Serial.println(_d->getHumidity(), 2);

    Serial.print("Temperature (oC): ");
    Serial.println(_d->getCelsius(), 2);

    Serial.print("Temperature (oF): ");
    Serial.println(_d->getFahrenheit(), 2);

    Serial.print("Temperature (K): ");
    Serial.println(_d->getKelvin(), 2);

    Serial.print("Dew Point (oC): ");
    Serial.println(_d->getDewPoint());

    Serial.print("Dew Point Slow (oC): ");
    Serial.println(_d->getDewPointSlow());
}
    
void loop()
{
    unsigned long _us = millis();
    int _delta = (_us - _lastTimeInLoop);

    if (_delta > (1.05 * LOOP_DELAY))
        _spark_error_count++;

    // Launch the acquisition on the two sensors
    DHTA.acquire();
    DHTB.acquire();

    // Print information for Sensor A
    Serial.print("\n");
    Serial.print(n);
    Serial.print(" : ");
    Serial.print((float) (_delta / 1000.0));
    Serial.print("s");
    if (_sensorA_error_count > 0 || _spark_error_count > 0) {
        Serial.print(" : E=");
        Serial.print(_sensorA_error_count);
        Serial.print("/");
        Serial.print(_spark_error_count);
    }
    Serial.print(", Retrieving information from sensor: ");
    Serial.print("Read sensor A: ");

    while (DHTA.acquiring()) ;
    printSensorData(&DHTA);

    // Print information for Sensor B
    Serial.print("\n");
    Serial.print(n);
    Serial.print(" : ");
    Serial.print((float) (_delta / 1000.0));
    Serial.print("s");
    if (_sensorB_error_count > 0 || _spark_error_count > 0) {
        Serial.print(" : E=");
        Serial.print(_sensorB_error_count);
        Serial.print("/");
        Serial.print(_spark_error_count);
    }
    Serial.print(", Retrieving information from sensor: ");
    Serial.print("Read sensor B: ");

    while (DHTB.acquiring()) ;
    printSensorData(&DHTB);

    n++;
    _lastTimeInLoop = _us;
    
    delay(LOOP_DELAY);
}
