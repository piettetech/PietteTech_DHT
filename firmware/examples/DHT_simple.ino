/*
 * FILE:        DHT_simple.ino
 * VERSION:     0.3
 * PURPOSE:     Example that uses DHT library with two sensors
 * LICENSE:     GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *
 * Samples one sensor and monitors the results for long term
 * analysis.  It calls DHT.acquireAndWait
 *
 * Scott Piette (Piette Technologies) scott.piette@gmail.com
 *      January 2014        Original Spark Port
 *      October 2014        Added support for DHT21/22 sensors
 *                          Improved timing, moved FP math out of ISR
 */

#include "PietteTech_DHT/PietteTech_DHT.h"  // Uncomment if building in IDE
//#include "PietteTech_DHT.h"  // Uncommend if building using CLI

#define DHTTYPE  DHT22       // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN   2           // Digital pin for communications

//declaration
void dht_wrapper(); // must be declared before the lib initialization

// Lib instantiate
PietteTech_DHT DHT(DHTPIN, DHTTYPE, dht_wrapper);
int n;      // counter

void setup()
{
    Serial.begin(9600);
    while (!Serial.available()) {
        Serial.println("Press any key to start.");
        delay (1000);
    }
    Serial.println("DHT Example program using DHT.acquireAndWait");
    Serial.print("LIB version: ");
    Serial.println(DHTLIB_VERSION);
    Serial.println("---------------");
}

// This wrapper is in charge of calling
// must be defined like this for the lib work
void dht_wrapper() {
    DHT.isrCallback();
}

void loop()
{
    Serial.print("\n");
    Serial.print(n);
    Serial.print(": Retrieving information from sensor: ");
    Serial.print("Read sensor: ");
    //delay(100);
  
    int result = DHT.acquireAndWait();

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
    Serial.print("Humidity (%): ");
    Serial.println(DHT.getHumidity(), 2);

    Serial.print("Temperature (oC): ");
    Serial.println(DHT.getCelsius(), 2);

    Serial.print("Temperature (oF): ");
    Serial.println(DHT.getFahrenheit(), 2);

    Serial.print("Temperature (K): ");
    Serial.println(DHT.getKelvin(), 2);

    Serial.print("Dew Point (oC): ");
    Serial.println(DHT.getDewPoint());

    Serial.print("Dew Point Slow (oC): ");
    Serial.println(DHT.getDewPointSlow());

    n++;
    delay(2500);
}
