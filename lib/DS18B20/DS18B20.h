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

#ifndef DS18B20_h
#define DS18B20_h

#include "Arduino.h"
#include <OneWire.h>
#include "string.h"

class DS18B20
{
	public:
		DS18B20(int pin);
		void init(int numSensors);
		void getAddresses(byte arrayIn[][8]);
		String getError();
		bool startConversion(byte address[8], bool parasitePower);
		float getTemp(byte address[8]);

		
		//int getID(byte[8] address);
		//int setID(byte[8] address);
		//boolean clearID(byte[8] address);
		//boolean clearIDbyID(int id);
		//float getTempByID(int id);
	private:
		int _pin;
		OneWire _ow;
		byte _address[8];
		byte _tmpAddr[8];
		int _ids[10];
		int _x;
		int _i;
		String _lastError;
		String _tmpString;
		String addressToString(byte *arrayPointer, int arraySize);
		byte _present;
		byte _ms;
		byte _ls;
		byte _data[9];
		float _temperature;
		int _positive;
		int _numSensors;
		
};  

#endif