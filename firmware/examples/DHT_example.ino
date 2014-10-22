/*
 * FILE:        DHT_example.ino
 * VERSION:     0.3
 * PURPOSE:     Example that uses DHT library with two sensors
 * LICENSE:     GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *
 * Example that start acquisition of DHT sensor and allows the
 * loop to continue until the acquisition has completed
 * It uses DHT.acquire and DHT.acquiring
 *
 * Change DHT_SAMPLE_TIME to vary the frequency of samples
 *
 * Scott Piette (Piette Technologies) scott.piette@gmail.com
 *      January 2014        Original Spark Port
 *      October 2014        Added support for DHT21/22 sensors
 *                          Improved timing, moved FP math out of ISR
 */

#include "PietteTech_DHT/PietteTech_DHT.h"

// system defines
#define DHTTYPE  DHT22              // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN   3         	    // Digital pin for communications
#define DHT_SAMPLE_INTERVAL   2000  // Sample every two seconds

//declaration
void dht_wrapper(); // must be declared before the lib initialization

// Lib instantiate
PietteTech_DHT DHT(DHTPIN, DHTTYPE, dht_wrapper);

// globals
unsigned int DHTnextSampleTime;	    // Next time we want to start sample
bool bDHTstarted;		    // flag to indicate we started acquisition
int n;                              // counter

void setup()
{
    Serial.begin(9600);
    while (!Serial.available()) {
        Serial.println("Press any key to start.");
        delay (1000);
    }
    Serial.println("DHT Example program using DHT.acquire and DHT.aquiring");
    Serial.print("LIB version: ");
    Serial.println(DHTLIB_VERSION);
    Serial.println("---------------");

    DHTnextSampleTime = 0;  // Start the first sample immediately
}


// This wrapper is in charge of calling
// mus be defined like this for the lib work
void dht_wrapper() {
    DHT.isrCallback();
}

void loop()
{
  // Check if we need to start the next sample
  if (millis() > DHTnextSampleTime) {
	if (!bDHTstarted) {		// start the sample
	    Serial.print("\n");
	    Serial.print(n);
	    Serial.print(": Retrieving information from sensor: ");
	    DHT.acquire();
	    bDHTstarted = true;
	}

	if (!DHT.acquiring()) {		// has sample completed?

	    // get DHT status
	    int result = DHT.getStatus();

	    Serial.print("Read sensor: ");
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

	    n++;  // increment counter
	    bDHTstarted = false;  // reset the sample flag so we can take another
	    DHTnextSampleTime = millis() + DHT_SAMPLE_INTERVAL;  // set the time for next sample
	}
    }
}
