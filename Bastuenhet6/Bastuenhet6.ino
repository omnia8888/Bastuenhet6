/*
 Name:		Bastuenhet6.ino
 Created:	8/1/2015 1:05:01 AM
 Author:	Micke
*/

/* -Change Log
6.0 Implemented readings for more Dallas sensors and added LED Display support for use with 
    Local Water heater Temperature readings.
*/

#include <LedControl.h>
#include <SoftwareSerial.h>
#include <DallasTemperature.h>
#include <OneWire.h>

//Micke egna bibliotek
#include "MickeDallasTemperature.h"
#include "MickesLedControl.h"
#include "RS485WithErrChk.h"
#include "SendReciveRemoteTemp.h"
#include "TemperatureDisplay.h"


//Pin nummer konfigurationen som enheterna är
//inkopplade på
#define LEDCONTROL_DATA_PIN 9
#define LEDCONTROL_CLOCK_PIN 8
#define LEDCONTROL_CS_PIN 7

//Antal inkopplade MAX 7219 ledcontrol enheter
#define NUM_OF_LEDCONTROLS 2

//Styr Pinnen för Dallas sensorerna
#define ONE_WIRE_CONTROL_PIN 4

#define SSerialRX        10  //Serial Receive pin
#define SSerialTX        11  //Serial Transmit pin
#define SSerialTxControl 3   //RS485 Direction control

#define RS485_SERIAL_COM_SPEED 4800
#define NUM_OF_REMOTE_SENSORS 3

//Definera samplings hastighet som data från
//frekvens ska hämtas
#define  READ_FREQVENCY 1000
//Definerar hur ofta det lägsta värdet från sjö temperaturen ska läsas av.
#define FREQTEMP_LOW_VALUE_UPDATE_INTERVALL 15000

//Skapa de globala objekten

OneWire oWire(ONE_WIRE_CONTROL_PIN);
MickeDallasTemperature mDallasTemp(&oWire);

RS485WithErrChk rs485 = RS485WithErrChk(SSerialRX, SSerialTX, SSerialTxControl);
SendReciveRemoteTemp sendRecvRTemp(&rs485, NUM_OF_REMOTE_SENSORS, 5000);

//Skapa det globala Micke Led control objektet
MickesLedControl MickeLC(LEDCONTROL_DATA_PIN, LEDCONTROL_CLOCK_PIN,
	LEDCONTROL_CS_PIN, NUM_OF_LEDCONTROLS);

//Skapa led display objekt.
TemperatureDisplay topDisplay = TemperatureDisplay(0, 0, 4, MickeLC);
TemperatureDisplay midDisplay = TemperatureDisplay(1, 0, 0, MickeLC);
TemperatureDisplay lowDisplay = TemperatureDisplay(2, 1, 0, MickeLC);

float lowestfreqTemp = 0;
float currentFreqTemp = 0;
long lastFreqTempUpdate = 0;

//Behövs för att köra Freqcounter
//interupten nedan
volatile unsigned long firstPulseTime;
volatile unsigned long lastPulseTime;
volatile unsigned long numPulses;

void isr()
{
	unsigned long now = micros();
	if (numPulses == 0) {
		firstPulseTime = now;
	}
	else {
		lastPulseTime = now;
	}
	++numPulses;
}

void setup()
{
	Serial.begin(9600);
	//Starta upp Dallas sensorerna
	Serial.println("Init Dallas Sensors..");
	mDallasTemp.InitSensors();

	rs485.InitRs485ComPort(RS485_SERIAL_COM_SPEED);

	//Starta upp led displayerna;
	Serial.println("Init LED Displays..");
	MickeLC.initLedDisplay(0, 5);
	MickeLC.initLedDisplay(1, 5);
}

void loop()
{
	float sjoTemp;
	float bastuTemp;
	float uteTemp;

	float vvTempTop;
	float vvTempMid;
	float vvTempLow;

	//Hämta temperatur från FreqCountern
	sjoTemp = GetLowestTempFromFreqCounter();

	//Hämta  temperatur från Dallas sensorn  
	//Läs av bastu och ute temperatur
	bastuTemp = mDallasTemp.getSensorTempC(0);
	uteTemp = mDallasTemp.getSensorTempC(1);

	//Läs av givarna i Varmatten beredaren.
	vvTempTop = mDallasTemp.getSensorTempC(2);
	vvTempMid = mDallasTemp.getSensorTempC(3);
	vvTempLow = mDallasTemp.getSensorTempC(4);

	//Skriv ut temperatur värdena i com fönstret
	Serial.print("Sjo Temperatur = ");
	Serial.println(sjoTemp);
	Serial.print("Bastu Temperatur = ");
	Serial.println(bastuTemp);
	Serial.print("Ute Temperatur = ");
	Serial.println(uteTemp);
	Serial.println();

	Serial.print("VV Temperatur i Toppen = ");
	Serial.println(vvTempTop);
	Serial.print("VV Temperatur i Mitten = ");
	Serial.println(vvTempMid);
	Serial.print("VV Temperatur i Botten = ");
	Serial.println(vvTempLow);
	Serial.println();

	//Sätt de temperaturer som skall sändas iväg
	sendRecvRTemp.setTemperatureToSend(0, sjoTemp);
	sendRecvRTemp.setTemperatureToSend(2, bastuTemp);
	sendRecvRTemp.setTemperatureToSend(1, uteTemp);

	//Kolla om det kommit in en request.
	String sTemp;
	sTemp = sendRecvRTemp.checkForRemoteData();
	Serial.print("Request data recived = ");
	Serial.println(sTemp);

	//Skriv ut temperatur värdena från VVBeredaren till Displayerna.
	topDisplay.showTemperature(vvTempTop);
	midDisplay.showTemperature(vvTempMid);
	lowDisplay.showTemperature(vvTempLow);
}

float GetLowestTempFromFreqCounter()
{
	float tempFromFreqCount;
	//Kolla om den körs för första gången i sådant fall hämta och returnera
	//en temperatur på direktent
	if (lastFreqTempUpdate == 0) {
		//Updatera timern
		Serial.println("Setting freqTempStartValue");
		lastFreqTempUpdate = millis();
		tempFromFreqCount = GetTempFromFreqCounter(READ_FREQVENCY);
		lowestfreqTemp = tempFromFreqCount;
		return lowestfreqTemp;
	}

	if (lastFreqTempUpdate + FREQTEMP_LOW_VALUE_UPDATE_INTERVALL > millis()) {
		Serial.print("Tar det sist updaterade lagsta FreqTemp vardet = ");
		Serial.println(lowestfreqTemp);
		//Tröskel värdet för updaterings intervallet har INTE uppnåtts,
		//så lägsta freqCount temperaturen ska bara läsas av
		tempFromFreqCount = GetTempFromFreqCounter(READ_FREQVENCY);

		Serial.print("Senaste hamtade Sjotemp = ");
		Serial.println(tempFromFreqCount);
		//Kolla om den senast hämtade temperaturen från FreqCounter är
		//Lägre än den hittils lägsta temperaturen.
		if (tempFromFreqCount < lowestfreqTemp) {
			//om det är lägst sparas undan.
			lowestfreqTemp = tempFromFreqCount;
		}
	}
	else {
		//Tiden har gått över och nytt lägsta värde skall sättas
		Serial.print("Updaterar sjoTempVardet med det senaste lagsta = ");
		Serial.println(lowestfreqTemp);
		currentFreqTemp = lowestfreqTemp;
		lastFreqTempUpdate = millis();
	}
	Serial.println();
	return currentFreqTemp;
}

//Hämta temperatur från freqCouner
float GetTempFromFreqCounter(int readfreq)
{
	//Hämta temperatur från puls räknare
	int freq;
	// Measure the frequency over the specified sample time in milliseconds, returning the frequency in Hz
	numPulses = 0;                      // prime the system to start a new reading
	attachInterrupt(0, isr, RISING);    // enable the interrupt
	delay(readfreq);
	detachInterrupt(0);
	freq = (numPulses < 2) ? 0 : (1000000UL * (numPulses - 1)) / (lastPulseTime - firstPulseTime);
	float temperatur = freq;
	return(temperatur / 10);
}

