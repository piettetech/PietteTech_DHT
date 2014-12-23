/*
 * FILE:        PietteTech_DHT.h
 * VERSION:     0.3
 * PURPOSE:     Spark Interrupt driven lib for DHT sensors
 * LICENSE:     GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *
 * S Piette (Piette Technologies) scott.piette@gmail.com
 *      January 2014        Original Spark Port
 *      October 2014        Added support for DHT21/22 sensors
 *                          Improved timing, moved FP math out of ISR
 *
 * Based on adaptation by niesteszeck (github/niesteszeck)
 * Based on original DHT11 library (http://playgroudn.adruino.cc/Main/DHT11Lib)
 *
 *
 * With this library connect the DHT sensor to the following pins
 * D0, D1, D2, D3, D4, A0, A1, A3, A5, A6, A7
 * http://docs.spark.io/firmware/#interrupts-attachinterrupt
 *
 */

#ifndef __PIETTETECH_DHT_H__
#define __PIETTETECH_DHT_H__

// There appears to be a overrun in memory on this class.  For now please leave DHT_DEBUG_TIMING enabled
#define DHT_DEBUG_TIMING        // Enable this for edge->edge timing collection

#include "application.h"

#define DHTLIB_VERSION "0.3"

// device types
#define DHT11                               11
#define DHT21                               21
#define AM2301                              21
#define DHT22                               22
#define AM2302                              22

// state codes
#define DHTLIB_OK                          0
#define DHTLIB_ACQUIRING                   1
#define DHTLIB_ACQUIRED                    2
#define DHTLIB_RESPONSE_OK                 3

// error codes
#define DHTLIB_ERROR_CHECKSUM              -1
#define DHTLIB_ERROR_ISR_TIMEOUT           -2
#define DHTLIB_ERROR_RESPONSE_TIMEOUT      -3
#define DHTLIB_ERROR_DATA_TIMEOUT          -4
#define DHTLIB_ERROR_ACQUIRING             -5
#define DHTLIB_ERROR_DELTA                 -6
#define DHTLIB_ERROR_NOTSTARTED            -7

#define DHT_CHECK_STATE                         \
        if(_state == STOPPED)                   \
            return _status;			\
        else if(_state != ACQUIRED)		\
            return DHTLIB_ERROR_ACQUIRING;      \
        if(_convert) convert();

class PietteTech_DHT
{
public:
    PietteTech_DHT(uint8_t sigPin, uint8_t dht_type, void (*isrCallback_wrapper)());
    void begin(uint8_t sigPin, uint8_t dht_type, void (*isrCallback_wrapper)());
    void isrCallback();
    int acquire();
    int acquireAndWait();
    float getCelsius();
    float getFahrenheit();
    float getKelvin();
    double getDewPoint();
    double getDewPointSlow();
    float getHumidity();
    bool acquiring();
    int getStatus();
    float readTemperature();
    float readHumidity();
#if defined(DHT_DEBUG_TIMING)
    volatile uint8_t _edges[41];
#endif
    
private:
    void (*isrCallback_wrapper)(void);
    void convert();
    
    enum states{RESPONSE=0,DATA=1,ACQUIRED=2,STOPPED=3,ACQUIRING=4};
    volatile states _state;
    volatile int _status;
    volatile uint8_t _bits[5];
    volatile uint8_t _cnt;
    volatile uint8_t _idx;
    volatile unsigned long _us;
    volatile bool _convert;
#if defined(DHT_DEBUG_TIMING)
    volatile uint8_t *_e;
#endif
    int _sigPin;
    int _type;
    unsigned long _lastreadtime;
    bool _firstreading;
    float _hum;
    float _temp;
};
#endif
