#include <stdio.h>;
#include "BusCommunication\include\Rs232Device.h";

int main() {

	// Init
	Rs232Device device;

	device.CreateHandle("COM1");
	device.InitHandle(6,8,device.EPARITY_NONE,device.STOPBITS_ONE,1,1);

	
	while(true) {

		// Hier kommt der ganze Konfigurationskram

		// Thread zum Aufmachen der Busverbindung
		
		
		// Thread zum Aufmachen einer Clientverbindung


		printf("Hallo");
	}
}