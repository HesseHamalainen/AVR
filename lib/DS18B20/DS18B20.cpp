/*
 * Library DS18B20 makes use of the OneWire Library to
 * easily retrieve temperatures from a defined number of sensors
 *
 * v0.2 - 2011-05-11
 *
 * Distributed under Modified BSD License
 *
 * Copyright (c) 2011, Oliver Butterfield
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Oliver Butterfield nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "Arduino.h"
#include "DS18B20.h"

DS18B20::DS18B20(int pin) : _ow(pin)
{

}

void DS18B20::init(int numSensors)
{
	_lastError = "";
	_numSensors = numSensors;
}

String DS18B20::getError()
{
	return _lastError;
}

String DS18B20::addressToString(byte *arrayPointer, int arraySize)
{
	String _tmpString = "";
	for(_i = 0; _i < arraySize; _i++)
	{
		_tmpString = _tmpString + ":" + *(arrayPointer+_i);
	}
	return _tmpString.substring(1);
}

void DS18B20::getAddresses(byte (*arrayIn)[8])
{
  _x = 0;
  byte * p;
  p = arrayIn[0];
  while ( _x < _numSensors )
  {
    if ( _ow.search(_tmpAddr) != 1)
    {
      _ow.reset_search();
      break;
    }
    if ( OneWire::crc8( _tmpAddr, 7) != _tmpAddr[7]) {
	  _lastError = "CRC fail for " + addressToString(_tmpAddr, 8);
    }
	else if ( _tmpAddr[0] != 0x28)
	{
	  _lastError = addressToString(_tmpAddr, 8) + " is incorrect sensor type";
    }
	else
	{
	    for(_i = 0; _i < 8; _i++)
		{
		*p = _tmpAddr[_i];
		p++;
		}
	}
    _x++;
  }
}

bool DS18B20::startConversion(byte address[8], bool parasitePower)
{
	_present = 0;
	_positive = 1;
	_temperature = 0.0;

	_ow.reset();
    _ow.select(address);
    _ow.write(0x44, parasitePower);
}

float DS18B20::getTemp(byte address[8])
{
    _present = _ow.reset();
    _ow.select(address);    
    _ow.write(0xBE);         // Read Scratchpad
    for ( _i = 0; _i < 9; _i++) {           // we need to read 9 bytes
      _data[_i] = _ow.read();
      if(_i == 0) _ls = _data[_i];
      if(_i == 1) _ms = _data[_i];
    }
	if((_ms >> 7 ) & 1 == 1)
	{
		//negative number
		_temperature = 0.0625;
		_positive = 0;
	}
	for(_i = 0; _i < 8; _i++)
	{
		if((_ls >> _i) & 1 == _positive) _temperature = _temperature + pow(2, (_i - 4));
		if((_ms >> _i) & 1 == _positive) _temperature = _temperature + pow(2, (_i + 4));
	}
	return _temperature;
}
