// 
// 
// 

#include "SendReciveRemoteTemp.h"
//#include "MickeDebug.h"



#define Pin13LED  13

//#define RequestTimeout 5000



SendReciveRemoteTemp::SendReciveRemoteTemp(RS485WithErrChk* RS,byte numOfRemoteSensors,long requestSendIntervall)
:_rs485(RS),_numOfSensors(numOfRemoteSensors),_requestSendIntervall(requestSendIntervall)
{
	pinMode(Pin13LED,OUTPUT);
	_requestSentTime = millis()+_requestSendIntervall;
}

//Kollar om det finns mottaget data i RS485 Bufferten och parsar detta
//l�ser ur mom det tex �r ett '@' f�r request el '$' f�r givar temperatur eller
// om det �r ett err som mottagits
String SendReciveRemoteTemp::checkForRemoteData()
{
	byte recvBuf[10];
	String returnVal;
	//H�mta data fr�n bufferten
	returnVal = reciveData(recvBuf);
	//Kolla om det inte har h�mtats n�got data eller om
	//n�got blivit fel och returnera detta.
	if (returnVal == "" || returnVal == "err")
	{
		return(returnVal);
	}
	
	//Kolla om det �r en request som mottagits
	if (char(recvBuf[0]) == '@')
	{
		//Skicka sensor data f�r alla sensorer en i taget.
		for (int i = 0;i < _numOfSensors; i++)
		{
			sendRemoteTemperature(i);
		}
		return(returnVal);
	}
	
	//Kolla om det �r temperatur data som mottagits
	if (char(recvBuf[0]) == '$')
	{
		//L�s av vilken sensor det g�ller
		byte recvSensIndex = char(recvBuf[1]);
		String sTemp;
		//L�s in den mottagna temperaturen
		for (int i = 2; i < 7; ++i)
		{
			sTemp += char(recvBuf[i]);
		}
		_remoteTemp[recvSensIndex] = sTemp.toFloat();
		return(sTemp);
	}
	
	return(returnVal);
	
}

//Returnerar true om requesten har skickats annars false
boolean SendReciveRemoteTemp::sendRemoteRequest(int sensorIndex)
{
		//Kolla om det redan finns en request p� g�ng
		if (millis() < _requestSentTime + _requestSendIntervall)
		{
			return(false);
		}
		
		digitalWrite(Pin13LED,HIGH);
		//s�tt ihop command str�ngen
		String cmdString = "@";
		cmdString += sensorIndex;
		//cmdString += "-432.1!";
		cmdString += "000000!";
		//skapa byte array och l�gg in str�ngen i den
		byte byteBuf[10];
		cmdString.getBytes(byteBuf,sizeof byteBuf);
		//Skicka commandot till RS485
		//sendMessage(byteBuf,sizeof byteBuf);
		_rs485->sendMessage(byteBuf,sizeof byteBuf);
		//s�tt request timern
		_requestSentTime = millis();
		digitalWrite(Pin13LED,LOW);
		return(true);
}

String SendReciveRemoteTemp::sendRemoteTemperature(byte sensorIndex)
{
			digitalWrite(Pin13LED,HIGH);
			String temp = String(_remoteTemp[sensorIndex]);
			
			//s�tt ihop command str�ngen
			String cmdString = "$";
			cmdString += sensorIndex;
			//L�gg in temperaturen
			cmdString += temp;
			//Fyll p� med 0:or
			for (int i = cmdString.length(); i < 8; ++i)
			{
				cmdString += "0";
			}
			cmdString += "!";
			
			//skapa byte array och l�gg in str�ngen i den
			byte byteBuf[10];
			cmdString.getBytes(byteBuf,sizeof byteBuf);
			//Skicka commandot till RS485
			//sendMessage(byteBuf,sizeof byteBuf);
			_rs485->sendMessage(byteBuf,sizeof byteBuf);
			//s�tt request timern
//			_requestSentTime = millis();
			digitalWrite(Pin13LED,LOW);
			return(cmdString);
}

//H�mtar data fr�n RS485 bufferten och returnerar en till hyfsad str�ng
//med dessa v�rden eller "error" om n�got g�tt fel.
String SendReciveRemoteTemp::reciveData(byte * recvBuf)
{
		byte recvTempBuf[10];
		byte recivedByteCount;
		String recvData;
		
		 //Kolla om det finns n�gon data i buffert annars hoppa ur
		 if (_rs485->available() == 0)
		 {
			 return("");
		 }

		//H�mta data fr�n bufferten
		recivedByteCount = _rs485->recvMessage(recvTempBuf,sizeof recvTempBuf);
		
		//Kolla att sista tecknet �r ett '!' om inte hoppa ur
		if (recvTempBuf[8] != char('!'))
		{
			return("err");
		}
		
		//Kolla att f�rsta tecknet �r ett '@' eller '$'
		if (recvTempBuf[0] != char('@') && recvTempBuf[0] != char('$'))
		{
			return("err");
		}
		
		//S�tt ihop en str�ng av de mottagna v�rderna
		for (int i = 0;i <= 8; i++) {
			recvBuf[i] = recvTempBuf[i];
			recvData += char(recvBuf[i]);
		}
		
		return(recvData);
		 
		 
	
}

void SendReciveRemoteTemp::setTemperatureToSend(byte sensorIndex,float temperature)
{
	_remoteTemp[sensorIndex] = temperature;
}

float SendReciveRemoteTemp::getRemoteTemperature(byte sensorIndex)
{
	return(_remoteTemp[sensorIndex]);
}




