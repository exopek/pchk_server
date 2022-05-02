/**
 * @file	include/slimio/internal/ipimpl.h
 * IP Implementierung (Schnittstelle f�r Linux- und Windows Implementierung).
 * @author	Lothar May
 * @since	2002-10-15
 * $Id: ipimpl.h 1 2013-03-21 08:03:52Z TobiasJ $
 */

#pragma once

//--- aus IpSocket.h -------------------------------------
#ifdef _WIN32
#include <Windows.h>
#include <winsock2.h>
#include <cstddef>
using std::size_t;
#else
#include <netinet/in.h>
#endif

#ifdef _WIN32
typedef SOCKET SocketValue;	// SocketValue entspricht dem Windows-Datentyp SOCKET.
#else
typedef int SocketValue;	// socket(...) gibt unter Unix/Linux einen int zur�ck, unter Windows dagegen ein SOCKET.

#define INVALID_SOCKET (-1)		// Unter Unix/Linux sind alle negativen Sockets ung�ltig. Zur Repr�sentation
								// eines einzelnen ung�ltigen Sockets wird -1 gew�hlt.
#endif

#include "../../Common/include/Timer.h"

#include <string>

#ifdef _WIN32
typedef int socklen_t;
#endif

typedef enum { TCP = 0, UDP } Protocol;


extern std::string GetLocalMachineId();
extern std::string GetLocalMachineIp();
extern std::string GetLocalHostName();
//--------------------------------------------------------
#include "IoDevice.h"

/**
 * Dies ist die Klasse f�r platformabh�ngige Implementierungen der IP Zugriffe.
 */

class IpDevice : public IoDevice
{
public:

	// Standardkonstruktor. Setzt den Status der Verbindung auf geschlossen.
	IpDevice();

	// Erzeugt eine implementierung anhand eines bereits verbundenen Sockets.
	// @param socket Der Verbindungsendpunkt - es muss bereits eine Verbindung aufgebaut sein.
	// @param properties Die Einstellungen, die f�r die Verbindung "geerbt" werden.
//	IpDevice(const IPSocket &socket, const IPProperties &properties);						???? Kann nie funktioniert haben??

	virtual ~IpDevice();										// Destruktor. Tut nichts.
	virtual void Open();										// Baut eine Verbindung �ber IP auf (als Client).
	virtual bool Close();										// Verbindung (wenn vorhanden) schlie�en.
																// Dieser Aufruf erzeugt keine IPException, selbst wenn kein Socket zum Schlie�en da ist.
	virtual int Read(char* buf, unsigned size) const;
	virtual int Write(const char* buf, unsigned size) const;
	virtual bool IsReady() const;
	virtual unsigned GetTypeID() const;

	virtual IoDevice *Clone() const;

	const char* GetRemoteAddress() { return m_remoteAddress; }
	int GetServerPort() { return m_serverPort; }
	int GetWriteTimeout() { return (int)m_writeTimeout; }
	int GetReadTimeout() { return (int)m_readTimeout; }
	int GetConnectTimeout() { return (int)m_connectTimeout; }

	void SetRemoteAddress(const char* remoteAddress);
	bool SetServerPort(unsigned short serverPort);
	void SetWriteTimeout(unsigned value) { m_writeTimeout = value; }
	void SetReadTimeout(unsigned value) { m_readTimeout = value; }
	void SetConnectTimeout(unsigned value) { m_connectTimeout = value; }

//----- aus IpSocket.cpp -----------------------------------------------------------------------------------------------
	void SetSocketValue(SocketValue socket);							// Setzt den Wert des Sockets. Hierbei wird, wenn n�tig, der aktuelle Socket geschlossen.
	bool CreateSocket(/*Protocol protocol = TCP*/);						// Erzeugt einen neuen Socket. Protokoll f�r die Verbindung (TCP, UDP)
	bool DestroySocket();												// Schlie�t/zerst�rt den Socket. true: Erfolgreich.
																		// false: Nicht geschlossen (entweder nicht erfolgreich oder autoclose ist false).
																		// Diese Methode wirft im Falle eines Fehlers keine Exception, damit sie problemlos im Destruktor aufgerufen werden kann.

	bool ConnectSocket(const char* targetAddr, unsigned short port);	// Baut eine Verbindung �ber IP auf (als Client).
																		// @param targetAddr Zieladresse (remote host).
																		// @param port Zielport (remote host).
																		// @retval true: Verbindungsaufbau erfolgreich. false: Pending.

	enum IPStatus { CLOSED, PENDING, CONNECTED };						// Status der IP-Verbindung.
	IPStatus GetStatus() const {return m_status;}						// Lesezugriff auf den Status. @return m_status.
	void SetStatus(IPStatus status) {m_status = status;}				// Status setzen.

protected:
	//----- aus IpProperties.h ----------------------------------------------------------------------------------------------
	char m_remoteAddress[IP_MAX_ADDRESS_LEN + 1];						// Ziel IP-Adresse. Dies ist die entfernte Adresse, die f�r connect verwendet wird.
	unsigned short m_serverPort;										// Portnummer des Servers. Im Server-Modus ist dies der lokale Port, im Client-Modus der entfernte Port.
	unsigned m_readTimeout;												// Timeout beim Empfangen von Daten �ber Netzwerk.
	unsigned m_writeTimeout;											// Timeout beim Senden von Daten �ber Netzwerk.
	unsigned m_connectTimeout;											// Timeout beim �ffnen einer Verbindung.

//----- aus IpSocket.h -------------------------------------------------------------------------------------------------
	SocketValue m_socket;												// Der zugrunde liegende Socket.
	struct sockaddr_in m_connectData;									// Interne Verbindungs-Daten, auf die von einem asynchronen connect zugegriffen wird.
	Timer m_connectTimer;												// Software-Timer f�r das Timeout bei dem Verbindungsaufbau.

private:
	IPStatus m_status;													// Status der IP-Verbindung.
};
