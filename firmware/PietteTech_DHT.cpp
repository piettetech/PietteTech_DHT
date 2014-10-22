/*
 * FILE:        PietteTech_DHT.cpp
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
 * This library supports the DHT sensor on the following pins
 * D0, D1, D2, D3, D4, A0, A1, A3, A5, A6, A7
 * http://docs.spark.io/firmware/#interrupts-attachinterrupt
 *
 */

/*
    Timing of DHT22 SDA signal line after MCU pulls low for 1ms
    https://github.com/mtnscott/Spark_DHT/AM2302.pdf
 
  - - - -            -----           -- - - --            ------- - -
         \          /     \         /  \      \          /
          +        /       +       /    +      +        /
           \      /         \     /      \      \      /
            ------           -----        -- - --------
 ^        ^                ^                   ^          ^
 |   Ts   |        Tr      |        Td         |    Te    |
 
    Ts : Start time from MCU changing SDA from Output High to Tri-State (Hi-Z)
         Spec: 20-200us             Tested: < 65us
    Tr : DHT response to MCU controlling SDA and pulling Low and High to
         start of first data bit
         Spec: 150-170us            Tested: 125 - 200us
    Td : DHT data bit, falling edge to falling edge
         Spec: '0' 70us - 85us      Tested: 60 - 110us
         Spec: '1' 116us - 130us    Tested: 111 - 155us
    Te : DHT releases SDA to Tri-State (Hi-Z)
         Spec: 45-55us              Not Tested
 */

#include "application.h"
#include "math.h"
#include "PietteTech_DHT.h"

// Thanks to Paul Kourany for this word type conversion function
uint16_t word(uint8_t high, uint8_t low) {
    uint16_t ret_val = low;
    ret_val += (high << 8);
    return ret_val;
}

PietteTech_DHT::PietteTech_DHT(uint8_t sigPin, uint8_t dht_type, void (*callback_wrapper)()) {
    begin(sigPin, dht_type, callback_wrapper);
    _firstreading = true;
}

void PietteTech_DHT::begin(uint8_t sigPin, uint8_t dht_type, void (*callback_wrapper) ()) {
    _sigPin = sigPin;
    _type = dht_type;
    isrCallback_wrapper = callback_wrapper;

    pinMode(sigPin, OUTPUT);
    digitalWrite(sigPin, HIGH);
    _lastreadtime = 0;
    _state = STOPPED;
    _status = DHTLIB_ERROR_NOTSTARTED;
}

int PietteTech_DHT::acquire() {
    // Check if sensor was read less than two seconds ago and return early
    // to use last reading
    unsigned long currenttime = millis();
    if (currenttime < _lastreadtime) {
        // there was a rollover
        _lastreadtime = 0;
    }
    if (!_firstreading && ((currenttime - _lastreadtime) < 2000 )) {
        // return last correct measurement, (this read time - last read time) < device limit
        return DHTLIB_ACQUIRED;
    }
    
    if (_state == STOPPED || _state == ACQUIRED) {
        /*
         * Setup the initial state machine
         */
        _firstreading = false;
        _lastreadtime = currenttime;
        _state = RESPONSE;

#if defined(DHT_DEBUG_TIMING)
        /*
         * Clear the debug timings array
         */
        for (int i = 0; i < 41; i++) _edges[i] = 0;
        _e = &_edges[0];
#endif

        /*
         * Set the initial values in the buffer and variables
         */
        for (int i = 0; i < 5; i++) _bits[i] = 0;
        _cnt = 7;
        _idx = 0;
        _hum = 0;
        _temp = 0;

        /*
         * Toggle the digital output to trigger the DHT device
         * to send us temperature and humidity data
         */
        pinMode(_sigPin, OUTPUT);
        digitalWrite(_sigPin, LOW);
        if (_type == DHT11)
            delay(18);                  // DHT11 Spec: 18ms min
        else
            delayMicroseconds(1500);    // DHT22 Spec: 0.8-20ms, 1ms typ
        pinMode(_sigPin, INPUT);        // Note Hi-Z mode with pullup resistor
                                        // will keep this high until the DHT responds.
        /*
         * Attach the interrupt handler to receive the data once the DHT
         * starts to send us data
         */
        _us = micros();
        attachInterrupt(_sigPin, isrCallback_wrapper, FALLING);

        return DHTLIB_ACQUIRING;
    } else
        return DHTLIB_ERROR_ACQUIRING;
}

int PietteTech_DHT::acquireAndWait() {
    acquire();
    while(acquiring()) ;
    return getStatus();
}

void PietteTech_DHT::isrCallback() {
    unsigned long newUs = micros();
    unsigned long delta = (newUs - _us);
    _us = newUs;

    if (delta > 6000) {
        _status = DHTLIB_ERROR_ISR_TIMEOUT;
        _state = STOPPED;
        detachInterrupt(_sigPin);
        return;
    }
    switch(_state) {
        case RESPONSE:            // Spec: 80us LOW followed by 80us HIGH
            if(delta < 65) {      // Spec: 20-200us to first falling edge of response
                _us -= delta;
                break; //do nothing, it started the response signal
            } if(125 < delta && delta < 200) {
#if defined(DHT_DEBUG_TIMING)
                *_e++ = delta;  // record the edge -> edge time
#endif
                _state = DATA;
            } else {
                detachInterrupt(_sigPin);
                _status = DHTLIB_ERROR_RESPONSE_TIMEOUT;
                _state = STOPPED;
#if defined(DHT_DEBUG_TIMING)
                *_e++ = delta;  // record the edge -> edge time
#endif
            }
            break;
        case DATA:          // Spec: 50us low followed by high of 26-28us = 0, 70us = 1
            if(60 < delta && delta < 155) { //valid in timing
        	_bits[_idx] <<= 1; // shift the data
        	if(delta > 110) //is a one
                    _bits[_idx] |= 1;
#if defined(DHT_DEBUG_TIMING)
                *_e++ = delta;  // record the edge -> edge time
#endif
                if (_cnt == 0) { // we have completed the byte, go to next
                    _cnt = 7; // restart at MSB
                    if(++_idx == 5) { // go to next byte, if we have got 5 bytes stop.
                        detachInterrupt(_sigPin);
                        // Verify checksum
                        uint8_t sum = _bits[0] + _bits[1] + _bits[2] + _bits[3];
                        if (_bits[4] != sum) {
                            _status = DHTLIB_ERROR_CHECKSUM;
                            _state = STOPPED;
                        } else {
                            _status = DHTLIB_OK;
                            _state = ACQUIRED;
                            _convert = true;
                        }
                        break;
                    }
                } else _cnt--;
            } else if(delta < 10) {
                detachInterrupt(_sigPin);
                _status = DHTLIB_ERROR_DELTA;
                _state = STOPPED;
            } else {
                detachInterrupt(_sigPin);
                _status = DHTLIB_ERROR_DATA_TIMEOUT;
                _state = STOPPED;
            }
            break;
        default:
            break;
    }
}

void PietteTech_DHT::convert() {
    // Calculate the temperature and humidity based on the sensor type
    switch (_type) {
        case DHT11:
            _hum = _bits[0];
            _temp = _bits[2];
            break;
        case DHT22:
        case DHT21:
            _hum = word(_bits[0], _bits[1]) * 0.1;
            _temp = (_bits[2] & 0x80 ?
                     -word(_bits[2] & 0x7F, _bits[3]) :
                     word(_bits[2], _bits[3])) * 0.1;
            break;
    }
    _convert = false;
}

bool PietteTech_DHT::acquiring() {
    if (_state != ACQUIRED && _state != STOPPED)
        return true;
    return false;
}

int PietteTech_DHT::getStatus() {
    return _status;
}

float PietteTech_DHT::getCelsius() {
    DHT_CHECK_STATE;
    return _temp;
}

float PietteTech_DHT::getHumidity() {
    DHT_CHECK_STATE;
    return _hum;
}

float PietteTech_DHT::getFahrenheit() {
    DHT_CHECK_STATE;
    return _temp * 9 / 5 + 32;
}

float PietteTech_DHT::getKelvin() {
    DHT_CHECK_STATE;
    return _temp + 273.15;
}

/*
 * Added methods for supporting Adafruit Unified Sensor framework
 */
float PietteTech_DHT::readTemperature() {
    acquireAndWait();
    return getCelsius();
}

float PietteTech_DHT::readHumidity() {
    acquireAndWait();
    return getHumidity();
}

// delta max = 0.6544 wrt dewPoint()
// 5x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
double PietteTech_DHT::getDewPoint() {
    DHT_CHECK_STATE;
    double a = 17.271;
    double b = 237.7;
    double temp_ = (a * (double) _temp) / (b + (double) _temp) + log( (double) _hum/100);
    double Td = (b * temp_) / (a - temp_);
    return Td;
}

// dewPoint function NOAA
// reference: http://wahiduddin.net/calc/density_algorithms.htm
double PietteTech_DHT::getDewPointSlow() {
    DHT_CHECK_STATE;
    double a0 = (double) 373.15 / (273.15 + (double) _temp);
    double SUM = (double) -7.90298 * (a0-1.0);
    SUM += 5.02808 * log10(a0);
    SUM += -1.3816e-7 * (pow(10, (11.344*(1-1/a0)))-1) ;
    SUM += 8.1328e-3 * (pow(10,(-3.49149*(a0-1)))-1) ;
    SUM += log10(1013.246);
    double VP = pow(10, SUM-3) * (double) _hum;
    double T = log(VP/0.61078); // temp var
    return (241.88 * T) / (17.558-T);
}
