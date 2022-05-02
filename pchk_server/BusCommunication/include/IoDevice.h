/**
 * @file	include/slimio/internal/ioimpl.h
 * Abstrakte Basisklasse für I/O-Implementierungen (RS232, Netzwerk, ggf. USB usw.)
 * @author	Lothar May
 * @since	2002-10-30
 * $Id: ioimpl.h 1 2013-03-21 08:03:52Z TobiasJ $
 */

#pragma once

// Definess für Eigenschaften von I/O - Verbindungen.
//----------------------------------------------------------------------------------------
#define IODEVICE_TYPEID_RS232				10				// IO_TYPEID_IMPL_RS232
#define IODEVICE_TYPEID_IP					11				// IO_TYPEID_IMPL_IP
//#define IODEVICE_TYPEID_SERVER_IP			21				// IO_TYPEID_IMPL_SERVER_IP

#define IP_REMOTE_ADDRESS_DEFAULT			"0.0.0.0"	// Adresse des entfernten Devices, das für I/O verwendet wird.
#define IP_MAX_ADDRESS_LEN					512

#define IP_LOCAL_ADDRESS_DEFAULT			"0.0.0.0"	// Adresse des lokalen Devices, das für I/O verwendet wird.

#define RS232_PORT_0						0			// Port des lokalen Devices (Portnummer für seriellen Port).
#define RS232_PORT_1						1			// Die Werte werden direkt für Open verwendet, also Reihenfolge und IDs nicht ändern!
#define RS232_PORT_2						2			// ACHTUNG: Windows COM1 entspricht Port 0!
#define RS232_PORT_3						3
#define RS232_PORT_4						4
#define RS232_PORT_5						5
#define RS232_PORT_6						6
#define RS232_PORT_7						7
#define RS232_PORT_8						8
#define RS232_PORT_9						9
#define RS232_PORT_10						10
#define RS232_PORT_11						11
#define RS232_PORT_12						12
#define RS232_PORT_13						13
#define RS232_PORT_14						14
#define RS232_PORT_15						15
#define RS232_PORT_16						16
#define RS232_PORT_17						17
#define RS232_PORT_18						18
#define RS232_PORT_19						19
#ifdef _WIN32
#define RS232_PORT_MAX						255
#define RS232_PORT_DEFAULT					"COM1"
#else
#define RS232_PORT_MAX						RS232_PORT_19
#define RS232_PORT_DEFAULT					"ttyS0"
#endif

#define MAX_RS232_COM_STRING_LEN			40			// Maximale Länge des COM-Strings (zum Zugriff auf einen speziellen COM-Port).

#define IP_PORT_MAX							0xFFFF-1
//#define IP_CLIENT_PORT_DEFAULT				0			// Port des Clients (Standard: 0, Betriebssystem wählt den Port).
#define IP_SERVER_PORT_DEFAULT				7126		// Port des Servers (freier Port ab 1024).

#define RS232_MODE_STD						0			// Modus der RS232-Verbindung (spezielle Modi sind nötig für die Kommunikation mit dem LCN-PC).
#define RS232_MODE_BIT_PCK_CD_CTS			1
#define RS232_MODE_BIT_PCK_CHECK_DSR 		2
#define RS232_MODE_BIT_MODEM_CHECK_DSR		4
#define RS232_MODE_MAX						(RS232_MODE_BIT_PCK_CD_CTS | RS232_MODE_BIT_PCK_CHECK_DSR | RS232_MODE_BIT_MODEM_CHECK_DSR)
#define RS232_MODE_DEFAULT					RS232_MODE_STD

														// Verhalten vom RTS-Pin.
#define RS232_RTSMODE_ALWAYSON				0			// LCN-PK/LCN-PKU
#define RS232_RTSMODE_TOGGLE				1			// LCN-PC (old)
#define RS232_RTSMODE_ALWAYSOFF				2			// Only LCN-PK/LCN-PKU will be able to send data
#define RS232_RTSMODE_MAX					2

#define RS232_BAUDRATE_110					0			// Baudrate für RS232-Verbindung.
#define RS232_BAUDRATE_300					1			// Die Werte werden in einem Array abgelegt, also Reihenfolge und IDs nicht ändern!
#define RS232_BAUDRATE_600					2
#define RS232_BAUDRATE_1200					3
#define RS232_BAUDRATE_2400					4
#define RS232_BAUDRATE_4800					5
#define RS232_BAUDRATE_9600					6
#define RS232_BAUDRATE_14400				7
#define RS232_BAUDRATE_19200				8
#define RS232_BAUDRATE_38400				9
#define RS232_BAUDRATE_56000				10
#define RS232_BAUDRATE_57600				11
#define RS232_BAUDRATE_115200				12
#define RS232_BAUDRATE_128000				13
#define RS232_BAUDRATE_256000				14
#define RS232_BAUDRATE_MAX					RS232_BAUDRATE_256000
#define RS232_BAUDRATE_DEFAULT				RS232_BAUDRATE_19200

//#define PROP_IO_INT_PROTOCOL  				31u			// Protokoll für die Kommunikation (z.B. TCP, UDP).
//#define IP_PROTOCOL_TCP						0
//#define IP_PROTOCOL_UDP						1
//#define IP_PROTOCOL_MIN						IP_PROTOCOL_TCP
//#define IP_PROTOCOL_MAX						IP_PROTOCOL_UDP
//#define IP_PROTOCOL_DEFAULT					IP_PROTOCOL_TCP

#define RS232_RESET_RTS_DELAY_MAX			100
#define RS232_RESET_RTS_DELAY_DEFAULT		1				// Verzögerung beim Zurücksetzen von RTS (in Millisekunden).

#define RS232_CHECK_CD_DELAY_MAX			1000
#define RS232_CHECK_CD_DELAY_DEFAULT		3				// Verzögerung beim Prüfen von CTS (in Millisekunden).Ehemals 8

#define IO_SEND_DELAY_MAX					1000
#define IO_LCN_SEND_DELAY_DEFAULT			0				// Verzögerung nach dem Senden (in Millisekunden).
#define IO_MODEM_SEND_DELAY_DEFAULT			0

#define IO_SENDFAILURE_DELAY_MAX	  		1000
#define IO_LCN_SENDFAILURE_DELAY_DEFAULT	0				// Verzögerung nach dem fehlgeschlagenem Sendeversuch (in Millisekunden).

#define IO_SEND_EXT_SEG_DELAY_MAX			1000
#define IO_LCN_SEND_EXT_SEG_DELAY_DEFAULT	0				// Verzögerung nach dem Senden an ein externes Segment (Decorator, zusätzlich zu SendDelay) (in Millisekunden)

#define IO_SEND_SAME_MOD_DELAY_MAX			1000
#define IO_LCN_SEND_SAME_MOD_DELAY_DEFAULT	0				// Verzögerung beim Senden von Datentelegrammen an dasselbe Modul (in Millisekunden)

#define RS232_WRITE_TIMEOUT_DEFAULT			100				// Timeout beim Senden von Daten (in Millisekunden).
#define IP_WRITE_TIMEOUT_DEFAULT			150

#define RS232_READ_TIMEOUT_DEFAULT			10				// Timeout beim Empfangen von Daten (in Millisekunden).
#define IP_READ_TIMEOUT_DEFAULT				10

#define IP_CONNECT_TIMEOUT_DEFAULT			4000			// Timeout beim Öffnen einer Verbindung (in Millisekunden).

#define IP_ACCEPT_TIMEOUT_DEFAULT			10				// (Periodisches) Timeout beim Warten auf eine Verbindung.

#define IP_BACKLOG_DEFAULT					5				// Maximale Anzahl an wartenden Verbindungen (für den Server-Modus).
//----------------------------------------------------------------------------------------

/**
 * Basisklasse für alle I/O-Implementierungen.
 * Diese Klasse abstrahiert von der hardware- und betriebssystemspezifischen
 * Implementierung der I/O-Zugriffe und von dem eigentlichen I/O-Gerät.
 */

class IoDevice// : public PropertyIface
{
public:

	IoDevice();
	virtual ~IoDevice();

	virtual void Open() = 0;
	virtual bool Close() = 0;
	virtual int Read(char* buf, unsigned size) const = 0;
	virtual int Write(const char* buf, unsigned size) const = 0;

	virtual bool IsReady() const = 0;

	virtual unsigned GetTypeID() const = 0;						// Zugriff auf die Typ-ID eines konkreten Objektes.

	virtual IoDevice *Clone() const = 0;						// Erzeugt ein Klon des Objektes (mit new). Der Klon erhält die Verantwortung über die internen Handles.

protected:
};
