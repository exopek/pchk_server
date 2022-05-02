/**
 * @file	include/slimio/internal/rs232impl.h
 * RS232-Implementierung (alle Platformen).
 * @author	Lothar May
 * @since	2002-10-22
 * $Id: rs232impl.h 1 2013-03-21 08:03:52Z TobiasJ $
 */

#pragma once

#include "../include/IoDevice.h"

//--- aus Rs232Handle.h --------------------------------------
#ifdef _WIN32
#endif

#ifdef _WIN32
#include <Windows.h>
typedef HANDLE HandleValue;			// HandleValue entspricht dem Windows-Datentyp HANDLE.
#else
typedef int HandleValue;			// open(...) gibt einen int zurück (Unix/Linux), CreateFile(...) dagegen ein HANDLE (Windows).
#define INVALID_HANDLE_VALUE (-1)		// Unter Unix/Linux sind alle negativen File-Handles ungültig. Zur Repräsentation eines einzelnen ungültigen Handles wird -1 gewählt.
class HandleData								// Unter Linux werden keine Handle-Daten benötigt.
{
};
#endif

#define RS232_RESET_RTS_DELAY_USEC_PER_BYTE	1040	// Zeitverzögerung beim Zurücksetzen von RTS pro Byte (Microsekunden).

/**
 * Dies ist die Klasse für die platformunabhängige Implementierung der RS232 Zugriffe.
 */

class Rs232Device : public IoDevice
{
public:
	Rs232Device();												// Standardkonstruktor. Setzt den Status der Verbindung auf geschlossen.
	virtual ~Rs232Device();										// Destruktor. Schließt automatisch die Verbindung.

	virtual void Open();
	virtual bool Close();
	virtual int Read(char* buf, unsigned size) const;
	virtual int Write(const char* buf, unsigned size) const;
	virtual bool IsReady() const;
	virtual unsigned GetTypeID() const;

	virtual IoDevice *Clone() const;

	bool SetComPort(const char* comPort);
	bool SetBaudrate(unsigned baudrate);
	bool SetComMode(int comMode);
	bool SetComRTSMode(int comRTSMode);
	bool SetResetRTSDelay(unsigned delay);
	bool SetCheckCDDelay(unsigned delay);
	void SetReadTimeout(unsigned timeout);
	void SetWriteTimeout(unsigned timeout);

	const char* GetComPort()			{ return m_comPort; }
	unsigned GetBaudrate() const		{ return m_baudrate; }
	int GetComMode() const				{ return m_comMode; }
	int GetComRTSMode() const			{ return m_comRTSMode; }
	unsigned GetResetRTSDelay() const	{ return m_resetRTSDelay; }
	unsigned GetCheckCDDelay() const	{ return m_checkCDDelay; }
	unsigned GetReadTimeout() const		{ return m_readTimeout; }
	unsigned GetWriteTimeout() const	{ return m_writeTimeout; }

	//--- aus Rs232Handle.h --------------------------------------
	enum Parity { EPARITY_NONE, EPARITY_ODD, EPARITY_EVEN, EPARITY_MARK, EPARITY_SPACE };
	enum StopBits { STOPBITS_ONE, STOPBITS_ONE5, STOPBITS_TWO };

	enum ComMode {									// Datentyp für den LCN-Kommunikationsmodus.
		MODE_STD = 0,
		MODE_PCK_CD_CTS = 1,						// Collision detection via CTS is available
		MODE_PCK_CHECK_DSR = 2,						// Only used for RTSMODE_TOGGLE
		MODE_MODEM_CHECK_DSR = 4					// DSR is required to check connection state (not used for LCN)
	};

	enum ComRTSMode {
		RTSMODE_ALWAYSON = 0,						// Default for LCN-PK/LCN-PKU
		RTSMODE_TOGGLE = 1,							// Required for (old) LCN-PC interface
		RTSMODE_ALWAYSOFF = 2						// Cannot write if old LCN-PC interface is connected
	};

	bool CreateHandle(const char* device);			// Erzeugt ein neues Handle entsprechend des Device-Namens

	// Bereitet ein geöffnetes Handle auf die Kommunikation vor, indem Baudrate und Modus gesetzt werden.
	bool InitHandle(unsigned baudrate, unsigned char byteSize, Parity parity, StopBits stopBits, int comMode, int comRTSMode);

	// Setzt die Timeouts (Millisekunden) für die Kommunikation bei einem geöffneten Handle.
	bool SetIOTimeoutsHandle(/*unsigned readTimeout, unsigned writeTimeout*/);

	bool DestroyHandle();							// Schließt das Handle, wenn es gültig ist. true: Erfolgreich.  false: Nicht erfolgreich.
													// Diese Methode wirft im Falle eines Fehlers keine Exception, damit sie problemlos im Destruktor aufgerufen werden kann.

	bool IsDeviceConnected() const;					// Versucht zu überprüfen, ob das Gerät noch verbunden ist, OHNE DATEN ZU VERSENDEN! Es werden RS232-Pins geprüft, je nach Konfiguration.

	bool Test(const char *comPort);					// Testet, ob hinter der Schnittstelle eine PKU lebt
	enum RS232Status { CLOSED, OPENED };						// Status der RS232-Verbindung.
	RS232Status GetStatus() const { return m_status; }

protected:
	void SetStatus(RS232Status status) {m_status = status;}

	//--- aus Rs232Handle.h --------------------------------------
	bool InternalCloseHandle();						// Schließt das Handle (ohne Gültigkeitsprüfung). true: Schließen erfolgreich.   false: Schließen ist fehlgeschlagen.
	void SetHandleValue(HandleValue handle);		// Setzt den Wert des Handles. Hierbei wird, wenn nötig, das aktuelle Handle geschlossen.

	//--- aus Rs232Properties.h --------------------------------------
	char m_comPort[MAX_RS232_COM_STRING_LEN];		// COM-Schnittstelle (COM1, COM2 etc.).
	unsigned m_baudrate;							// Geschwindigkeit der COM-Verbindung.
	int m_comMode;									// Modus der COM-Verbindung (i.e. ist es eine "normale" RS232-Verbindung oder eine mit RTS.
	int m_comRTSMode;								// Modus für RTS-Pin.
	unsigned m_resetRTSDelay;						// Verzögerung vor dem Zurücksetzen von RTS nach dem Senden. (LCN-PC) Einheit: Millisekunden.
	unsigned m_checkCDDelay;						// Wartezeit vor dem Senden, in der geprüft wird ob der Bus frei ist. (LCN-PC) Einheit: Millisekunden.
	unsigned m_readTimeout;							// Timeout (msec) beim Empfangen von Daten von der seriellen Schnittstelle.
	unsigned m_writeTimeout;						// Timeout (msec) beim Senden von Daten an die serielle Schnittstelle.

	//--- aus Rs232Handle.h --------------------------------------
	HandleValue m_handle;							// Das zugrunde liegende Handle.
	mutable OVERLAPPED m_handleInputOverlapped;		// Struktur fuer den asynchronen Empfang von Daten.
	mutable OVERLAPPED m_handleOutputOverlapped;	// Struktur fuer das asynchrone Versenden von Daten.
	mutable OVERLAPPED m_handleComEventOverlapped;	// Struktur fuer das asynchrone Warten auf Com-Events.
	mutable DWORD m_handleComEventMask;				// Event-Maske beim Senden/Empfangen von Daten.
	void InitHandleData();							// Erzeugt Events.
	void CleanupHandleData();						// Gibt Events frei.

private:
	RS232Status m_status;							// Status der RS232-Verbindung.
};
