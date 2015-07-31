// 
// 
// 
#include "OneWire.h"
#include "DallasTemperature.h"
#include "MickeDallasTemperature.h"

DeviceAddress sensor0 = {0x28, 0x91, 0x3E, 0xD0, 0x05, 0x00, 0x00, 0x8A };



MickeDallasTemperature::MickeDallasTemperature(OneWire* _onewire)
: DallasTemperature(_onewire)
{

}

// Initsierar och strtar upp sensorna och r�knar dem
// Loopar sedan igenom den och tar fram sensor adresserna
// och sparar ned dem
void MickeDallasTemperature::InitSensors()
{
	
	//starta data insamlingen fr�nsensorna
	begin();
	
	//st�ll in uppl�sningen. Rekomenderat v�rde �r 10.
	setResolution(12);
	
	//h�mta data f�r alla sensorer.
	//beh�vs f�r att bla. kunna r�kna dem
	requestTemperatures();
	
	//R�kna antalet sensorer
	_numberofsensors = getDeviceCount();
	
	//Loppa igen om alla inkopplade sensorer
	//och sparar undan sensoradresserna
	for (int i = 0;i < _numberofsensors;i++){
	  boolean b = getAddress(sensoraddress[i],i);
	}
	float f;
	f = getTempC(sensoraddress[0]);
	return;	
}

//Uppdaterar och h�mtar temperatur fr�n en givare med
//motsvarande nummer fr�n sensor indexet p� den
//inkopplade givarna
float MickeDallasTemperature::getSensorTempC(int sensorindex)
{
	//Uppdatera temperaturer fr�n givarna
	requestTemperatures();
	//h�mta temperatur fr�n en av givara
	float temperature = getTempC(sensoraddress[sensorindex]);
	return(temperature);			
}


