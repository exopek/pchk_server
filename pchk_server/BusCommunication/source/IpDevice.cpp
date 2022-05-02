/**
 * @file	src/common/ipimpl.cpp
 * IP Implementierung (alle Platformen).
 * @author	Lothar May
 * @since	2002-10-18
 * $Id: ipimpl.cpp 2385 2016-10-19 15:49:53Z TobiasJ $
 */
#define _WINSOCK_DEPRECATED_NO_WARNINGS // -> https://stackoverflow.com/a/34728676; https://stackoverflow.com/a/4726838

#include "../Common/include/Common.h"
#include "include/IpDevice.h"
#include "../Common/include/ErrorMessage.h"
//#include "../Common/include/assert.h"
#include <malloc.h>
#include <cstring>
#include <cstdio>
#include <cassert>

#ifdef _WIN32
//#include <Windows.h>
//#include <winsock2.h>
//#include <winerror.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#define SocketError WSAGetLastError()
#define SOCK_EWOULDBLOCK WSAEWOULDBLOCK
#define SOCK_EINPROGRESS WSAEWOULDBLOCK
#define MSG_NOSIGNAL 0
#define SHUT_WR SD_SEND
#define SHUT_RD SD_RECEIVE
#define IS_CONNECT_IN_PROGRESS(v) (((v) == SOCK_EINPROGRESS) || ((v) == WSAEALREADY))

#else
#error This source code is Win32 specific.
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netdb.h>

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#define closesocket close
#define ioctlsocket ioctl
#define IS_SOCKET_INVALID(s) ((s) < 0)
#define SocketError errno
#define SOCK_EWOULDBLOCK EWOULDBLOCK
#define SOCK_EINPROGRESS EINPROGRESS
#define SOCKET_ERROR -1
#define IS_CONNECT_IN_PROGRESS(v) (((v) == SOCK_EINPROGRESS) || ((v) == EALREADY))

#define inaddrr(x) (*(struct in_addr *) &ifr->x[sizeof(sockaddr_in.sin_port)])
#define IFRSIZE   ((int)(size * sizeof (struct ifreq)))
#endif


//#include <cassert>

//------ aus IpSocket.cpp importiert --------------------------------------------------------------
static std::string lastLocalMachineId;
static std::string lastLocalMachineIp;

std::string GetLocalMachineId()
{
	std::string ret = lastLocalMachineId;
	if (ret.empty())
	{
#ifdef _WIN32
		PIP_ADAPTER_INFO pAdapterInfo;

		ULONG requiredBufSize = 0;

		// Benötigte Puffergröße holen.
		if (GetAdaptersInfo(NULL, &requiredBufSize) == ERROR_BUFFER_OVERFLOW)
		{
			pAdapterInfo = (IP_ADAPTER_INFO *)malloc(requiredBufSize);
			// Informationen zu Netzwerk-Verbindungen abfragen.
			if ((GetAdaptersInfo(pAdapterInfo, &requiredBufSize)) == NO_ERROR)
			{
				PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
				while (pAdapter)
				{
					// Nur Ethernet-Adapter beruecksichtigen.
					if (pAdapter->Type == MIB_IF_TYPE_ETHERNET)
					{
						// Adresse sollte 6 Bytes lang sein - laengere sind auch OK, weitere Bytes werden ignoriert.
						if (pAdapter->AddressLength >= 6)
						{
							char macAddress[128];
							sprintf_s(macAddress, sizeof(macAddress), "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
								pAdapter->Address[0], pAdapter->Address[1], pAdapter->Address[2],
								pAdapter->Address[3], pAdapter->Address[4], pAdapter->Address[5]);

							// Use first one
							if (ret.empty())
								ret = macAddress;
						}
					}
					pAdapter = pAdapter->Next;
				}
			}
			free(pAdapterInfo);
		}
#else
		// Code kopiert und zurechtgeschnitten (Quelle: usenet)
		int                sockfd, size = 1;
		struct ifreq       *ifr;
		struct ifconf      ifc;

		if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) >= 0)  /* Error - socket creation failed. */
		{
			ifc.ifc_len = IFRSIZE;
			ifc.ifc_req = NULL;

			bool ok = true;
			do
			{
				++size;
				/* realloc buffer size until no overflow occurs  */
				if (NULL == (ifc.ifc_req = (ifreq *)realloc(ifc.ifc_req, IFRSIZE)))
				{
					ok = false; /* Error - out of memory. */
					break;
				}
				ifc.ifc_len = IFRSIZE;
				if (ioctl(sockfd, SIOCGIFCONF, &ifc))
				{
					ok = false; /* Error - could not read interface configuration. */
					break;
				}
			} while (IFRSIZE <= ifc.ifc_len);

			if (ok)
			{
				ifr = ifc.ifc_req;
				for (; (char *)ifr < (char *)ifc.ifc_req + ifc.ifc_len; ++ifr)
				{

					if (ifr->ifr_addr.sa_data == (ifr + 1)->ifr_addr.sa_data)
						continue;  /* duplicate, skip it */

					if (ioctl(sockfd, SIOCGIFFLAGS, ifr))
						continue;  /* failed to get flags, skip it */

					if (0 == ioctl(sockfd, SIOCGIFHWADDR, ifr))
					{

						/* Select which  hardware types to process.
						 *
						 *    See list in system include file included from
						 *    /usr/include/net/if_arp.h  (For example, on
						 *    Linux see file /usr/include/linux/if_arp.h to
						 *    get the list.)
						 */
						switch (ifr->ifr_hwaddr.sa_family)
						{
						case  ARPHRD_ETHER:
						case  ARPHRD_EETHER:
						case  ARPHRD_IEEE802:
							break;

						default:
							continue;
						}

						unsigned char      *u = (unsigned char *)&ifr->ifr_addr.sa_data;

						if (u && (u[0] + u[1] + u[2] + u[3] + u[4] + u[5])) {
							char macAddress[128];
							SPRINTF_S(macAddress, sizeof(macAddress),
								"%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
								u[0], u[1], u[2], u[3], u[4], u[5]);

							// Use first one
							if (ret.empty())
								ret = macAddress;
						}
					}
				}
			}

			free(ifc.ifc_req);
			close(sockfd);
		}
#endif
		lastLocalMachineId = ret;
	}
	return ret;
}

std::string GetLocalMachineIp()
{
	std::string ret = lastLocalMachineIp;
	if (ret.empty())
	{
		PIP_ADAPTER_INFO pAdapterInfo;
		ULONG requiredBufSize = 0;
		if (GetAdaptersInfo(NULL, &requiredBufSize) == ERROR_BUFFER_OVERFLOW)		// Benötigte Puffergröße holen.
		{
			pAdapterInfo = (IP_ADAPTER_INFO *)malloc(requiredBufSize);
			if ((GetAdaptersInfo(pAdapterInfo, &requiredBufSize)) == NO_ERROR)		// Informationen zu Netzwerk-Verbindungen abfragen.
			{
				PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
				while (pAdapter)
				{
					if (pAdapter->Type == MIB_IF_TYPE_ETHERNET)						// Nur Ethernet-Adapter beruecksichtigen.
					{
						// Adresse sollte 6 Bytes lang sein - laengere sind auch OK, weitere Bytes werden ignoriert.
						if (pAdapter->AddressLength >= 6)
						{
							char ipAddress[20];
							memcpy(ipAddress, (void *)(&pAdapter->IpAddressList.IpAddress), sizeof(pAdapter->IpAddressList.IpAddress));

							// Use first one
							if (ret.empty())
								ret = ipAddress;
						}
					}
					pAdapter = pAdapter->Next;
				}
			}
			free(pAdapterInfo);
		}
		lastLocalMachineIp = ret;
	}
	return ret;
}

std::string GetLocalHostName()
{
	std::string ret;
#ifdef _WIN32
	char hostname[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD size = sizeof(hostname);
	if (GetComputerNameA(hostname, &size))
		ret = hostname;
#else
	char hostname[MAXHOSTNAMELEN + 1];
	if (gethostname(hostname, MAXHOSTNAMELEN) >= 0)
	{
		hostname[MAXHOSTNAMELEN] = '\0';
		ret = hostname;
	}
#endif
	return ret;
}
//-------------------------------------------------------------------------------------------------

IpDevice::IpDevice()
: m_status(CLOSED)
{
	m_socket = INVALID_SOCKET;
	m_readTimeout = 0;
	m_writeTimeout = 0;
	m_connectTimeout = 0;

	m_serverPort = IP_SERVER_PORT_DEFAULT;
	m_readTimeout = IP_READ_TIMEOUT_DEFAULT;
	m_writeTimeout = IP_WRITE_TIMEOUT_DEFAULT;
	m_connectTimeout = IP_CONNECT_TIMEOUT_DEFAULT;
	SetRemoteAddress(IP_REMOTE_ADDRESS_DEFAULT);
}

/*IpDevice::IpDevice(const IPSocket &socket, const IPProperties &properties)
: m_status(CONNECTED)
{
	m_socket = _new IPSocket(socket);
	assert(m_socket);
	m_properties = _new IPProperties(properties);
	assert(m_properties);
}											??? kann nie funktioniert haben ?? */

IpDevice::~IpDevice()
{
	DestroySocket();
}

void IpDevice::Open()
{
	if (GetStatus() != IpDevice::CONNECTED)
	{
		if (m_socket == INVALID_SOCKET)
		{
			if (CreateSocket())
			{
				if (ConnectSocket(m_remoteAddress, m_serverPort))
				{
					SetStatus(IpDevice::CONNECTED);			// Jo, hat gefunzt
					return;
				}
			}
		}

		DestroySocket();									// irgendein Fehler aufgetreten
		SetStatus(IpDevice::CLOSED);
	}
}

int IpDevice::Read(char* buf, unsigned size) const
{
	if (GetStatus() != IpDevice::CONNECTED)
	{
		ToErrorMailbox(ErrMsgError, ERR_IO_SOCKET, ERR_IO_RecvNotConnected);
		return -1;
	}

	int haveRead = 0;
	// set erzeugen zum Warten auf eingehende Daten
	fd_set rdset;
	FD_ZERO(&rdset);
	SocketValue fdmax = 0;
#ifndef _WIN32
	FD_SET(m_destroyEv.m_syncObjectData.pipefd[0], &rdset);
	if (m_destroyEv.m_syncObjectData.pipefd[0] > fdmax)
		fdmax = m_destroyEv.m_syncObjectData.pipefd[0];
#endif
	FD_SET(m_socket, &rdset);
	if (m_socket > fdmax)
		fdmax = m_socket;

	// Timeout setzen
	struct timeval timeout;
	struct timeval* ptimeout = NULL;
	if (m_readTimeout != INFINITE)
	{
		timeout.tv_sec = m_readTimeout / 1000;
		timeout.tv_usec = (m_readTimeout % 1000) * 1000 + 500;
		ptimeout = &timeout;
	}

	// select() ausführen
	int result = select(fdmax + 1, &rdset, NULL, NULL, ptimeout);

	if (result == SOCKET_ERROR)// !IS_VALID_SELECT(result))
	{
		ToErrorMailbox(ErrMsgAbort, ERR_IO_SOCKET, ERR_IO_RecvSelect, SocketError);
		return -1;
	}
	else if (result > 0) // 0 ist Timeout
	{
#ifndef _WIN32
		if (FD_ISSET(m_destroyEv.m_syncObjectData.pipefd[0], &rdset) && m_destroyEv.TryAcquire())
			throw LcnException(ERR_IO_SOCKET, ERR_IO_RecvConnectionClosed, ERR_defaultErrorCode, true);
#endif
		if (FD_ISSET(m_socket, &rdset))
		{
			struct sockaddr_in lastFrom;
			socklen_t lastFromLen = sizeof(lastFrom);
			haveRead = recvfrom(m_socket, buf, size, MSG_NOSIGNAL, (struct sockaddr*)&lastFrom, &lastFromLen);
			assert(lastFromLen == sizeof(lastFrom));

			if (haveRead == SOCKET_ERROR)// !IS_VALID_RECV(haveRead))
			{
				ToErrorMailbox(ErrMsgError, ERR_IO_SOCKET, ERR_IO_Recv, SocketError);
				return -1;
			}
			// Sobald recv 0 liefert, wurde die Verbindung beendet.
			// Wenn die Verbindung nicht neu aufgebaut werden kann (z.B. bei einem Server-Socket)
			// ist dies ein fataler Fehler.
			if (haveRead == 0)
			{
				ToErrorMailbox(ErrMsgError, ERR_IO_SOCKET, ERR_IO_RecvConnectionClosed, ERR_defaultErrorCode);
				return -1;
			}
	/*		// Rueckgabewerte
			if (inAddr)
				*inAddr = inet_ntoa(lastFrom.sin_addr);
			if (inPort)
				*inPort = ntohs(lastFrom.sin_port);             erstmal weggelassen SF 28.05.21 */
		}
	}

	return haveRead;
}

int IpDevice::Write(const char* buf, unsigned size) const
{
	if (GetStatus() != IpDevice::CONNECTED)
	{
		ToErrorMailbox(ErrMsgError, ERR_IO_SOCKET, ERR_IO_SendNotConnected);
		return -1;
	}

	int haveSent = 0;
	fd_set wrset;

	FD_ZERO(&wrset);
	FD_SET(m_socket, &wrset);

	// Timeout setzen
	struct timeval timeout;
	struct timeval* ptimeout = NULL;
	if (m_writeTimeout != INFINITE)
	{
		timeout.tv_sec = m_writeTimeout / 1000;
		timeout.tv_usec = (m_writeTimeout % 1000) * 1000;
		ptimeout = &timeout;
	}

	int result = select((int)m_socket + 1, NULL, &wrset, NULL, ptimeout);
	if (result == SOCKET_ERROR)
	{
		ToErrorMailbox(ErrMsgError, ERR_IO_SOCKET, ERR_IO_SendSelect, SocketError);
		return -1;
	}
	else if (result > 0)
	{
		haveSent = send(m_socket, buf, size, MSG_NOSIGNAL);
		if (haveSent == SOCKET_ERROR)
		{
			ToErrorMailbox(ErrMsgError, ERR_IO_SOCKET, ERR_IO_Send, SocketError);
			return -1;
		}
	}

	return haveSent;
}

bool IpDevice::Close()
{
	bool retVal = DestroySocket();
	SetStatus(IpDevice::CLOSED);
	return retVal;
}

bool IpDevice::IsReady() const
{
	return (GetStatus() == IpDevice::CONNECTED);
}

unsigned IpDevice::GetTypeID() const
{
	return IODEVICE_TYPEID_IP;		//IO_TYPEID_IMPL_IP;
}

IoDevice *IpDevice::Clone() const
{
	IpDevice *newImpl = _new IpDevice;
	newImpl->m_socket = m_socket;
	memcpy((void*)(&newImpl->m_connectData), (void*)&m_connectData, sizeof(m_connectData));
	newImpl->m_connectTimer = m_connectTimer;

	newImpl->m_serverPort = m_serverPort;
	newImpl->m_readTimeout = m_readTimeout;
	newImpl->m_writeTimeout = m_writeTimeout;
	newImpl->m_connectTimeout = m_connectTimeout;
	newImpl->SetRemoteAddress(m_remoteAddress);

	newImpl->SetStatus(GetStatus());
	return newImpl;
}

//----- aus IpProperties.cpp ----------------------------------------------------------------------------------------------
void IpDevice::SetRemoteAddress(const char* remoteAddress)
{
	strncpy_s(m_remoteAddress, sizeof(m_remoteAddress), remoteAddress, IP_MAX_ADDRESS_LEN);
	m_remoteAddress[IP_MAX_ADDRESS_LEN] = 0;
}

bool IpDevice::SetServerPort(unsigned short serverPort)
{
	if (serverPort > IP_PORT_MAX)
	{
		ToErrorMailbox(ErrMsgAbort, ERR_IO_SOCKET, ERR_IO_InvalidServerPort, serverPort);
		return false;
	}

	m_serverPort = serverPort;
	return true;
}

//----- aus IpSocket.cpp -----------------------------------------------------------------------------------------------
void IpDevice::SetSocketValue(SocketValue socket)
{
	if (m_socket != socket)
	{
		DestroySocket();
#ifndef _WIN32
		m_destroyEv.ResetEvent();
#endif
		m_socket = socket;
	}
}

bool IpDevice::CreateSocket()
{
	struct WSAData wsaData;
	int wserr = WSAStartup(WINSOCK_VERSION, &wsaData);										// Winsock Initialisierung
	if (wserr != 0)
	{
		ToErrorMailbox(ErrMsgAbort, ERR_IO_SOCKET, ERR_IO_InitWSAStartup, wserr);
		return false;
	}

	if (LOBYTE(wsaData.wVersion) != LOBYTE(WINSOCK_VERSION))								// Hauptversion muss übereinstimmen
	{
		ToErrorMailbox(ErrMsgAbort, ERR_IO_SOCKET, ERR_IO_InitWSAVersion, wsaData.wVersion);
		return false;
	}

	SocketValue s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);								// wir nehmen nur TCP/IP
	if (s == NULL)
	{
		ToErrorMailbox(ErrMsgAbort, ERR_IO_SOCKET, ERR_IO_Create, SocketError);
		return false;
	}

	// Nach closesocket soll gleich wieder ein neues bind/listen/accept am gleichen Port möglich sein.
	// Also die Adresse wiederverwenden, damit dies nicht fehlschlägt.
	unsigned long reuseAddrFlag = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseAddrFlag, sizeof(reuseAddrFlag)) == SOCKET_ERROR)
	{
		ToErrorMailbox(ErrMsgAbort, ERR_IO_SOCKET, ERR_IO_Create_SO_REUSEADDR, SocketError);
		return false;
	}

	// Es soll nichts blockieren, da sonst der Thread hängt.
	unsigned long nonBlockingFlag = 1;
	if (ioctlsocket(s, FIONBIO, &nonBlockingFlag) == SOCKET_ERROR)
	{
		ToErrorMailbox(ErrMsgAbort, ERR_IO_SOCKET, ERR_IO_Create_FIONBIO, SocketError);
		return false;
	}

	SetSocketValue(s);
	return true;
}

bool IpDevice::ConnectSocket(const char* targetAddr, unsigned short port)
{
	bool isConnected = false;

	if (!targetAddr || (targetAddr[0] == 0))
	{
		ToErrorMailbox(ErrMsgAbort, ERR_IO_SOCKET, ERR_IO_ConnectInvalidAddress, ERR_defaultErrorCode);
		return false;
	}

	bool success = false;

	addrinfo aiHints;
	addrinfo *aiList = NULL;

	memset(&aiHints, 0, sizeof(aiHints));
	aiHints.ai_family = PF_INET;

	char thePort[40];
	sprintf_s(thePort, sizeof(thePort), "%d", port);
	if ((getaddrinfo(targetAddr, thePort, &aiHints, &aiList) == 0) && aiList)
	{
		if (aiList->ai_addrlen == sizeof(sockaddr_in))
		{
			memcpy(&m_connectData, aiList->ai_addr, sizeof(m_connectData));
			success = true;									// muss mit diesem Flag gemacht werden, damit die Liste frei gegeben werden kann!
		}
	}

	if (aiList)
		freeaddrinfo(aiList);

	if (!success)
	{
		ToErrorMailbox(ErrMsgError, ERR_IO_SOCKET, ERR_IO_ConnectCannotResolve, targetAddr);
		return false;
	}

	// Hier wird ein Attribut verwendet und keine lokale Variable,
	// da das connect im Hintergrund weiterlaeuft.
	int result = ::connect(m_socket, (sockaddr*)&m_connectData, sizeof(sockaddr_in));
	if (result != SOCKET_ERROR)								// connect liefert 0 wenn es sofort erfolgreich war.
	{
		isConnected = true;
	}
	else
	{
		int errorCode = SocketError;						// Fehlercode prüfen
		if (!IS_CONNECT_IN_PROGRESS(errorCode))
		{
			ToErrorMailbox(ErrMsgAbort, ERR_IO_SOCKET, ERR_IO_ConnectCall, targetAddr, errorCode);
			return false;
		}
		m_connectTimer.Start(m_connectTimeout);

		fd_set wrset;										// OK - "in progress", warten bis die Verbindung aufgebaut wurde.
		FD_ZERO(&wrset);									// Verbindungsaufbau ist vollendet, sobald Schreibzugriffe möglich sind.
		FD_SET(m_socket, &wrset);

		struct timeval timeout;								// Timeout setzen
		timeout.tv_sec = m_connectTimeout / 1000;
		timeout.tv_usec = (m_connectTimeout % 1000) * 1000;

		int result = ::select((int)m_socket + 1, NULL, &wrset, NULL, &timeout);
		if (result == SOCKET_ERROR)
		{
			m_connectTimer.Stop();
			ToErrorMailbox(ErrMsgAbort, ERR_IO_SOCKET, ERR_IO_ConnectSelect, targetAddr, SocketError);
			return false;
		}

		if (result == 0)									// Timeout
		{
			if (m_connectTimer.HasTimeoutElapsed())
			{
				m_connectTimer.Stop();
				ToErrorMailbox(ErrMsgError, ERR_IO_SOCKET, ERR_IO_ConnectTimeout);
				return false;
			}
		}
		else
		{
			int flag = 1;									// Rückgabewert prüfen.
			socklen_t flagLength = sizeof(flag);
			getsockopt(m_socket, SOL_SOCKET, SO_ERROR, (char*)&flag, &flagLength);
			if (flag != 0)
			{
				m_connectTimer.Stop();						// connect-Aufruf ist fehlgeschlagen.
				ToErrorMailbox(ErrMsgError, ERR_IO_SOCKET, ERR_IO_Connect, targetAddr, flag);
				return false;
			}
			else
			{
				m_connectTimer.Stop();
				isConnected = true;							// Verbindungsaufbau erfolgreich.
			}
		}
	}

	return isConnected;
}

bool IpDevice::DestroySocket()
{
	bool retval = false;

	if (m_socket == INVALID_SOCKET)
		retval = true;
	else
	{												// Server informieren, dass die Verbindung beendet wird.
		int result = ::shutdown(m_socket, SHUT_WR);	// SYSCALL(result, shutdown(GetSocketValue(), SHUT_WR), IS_VALID_SHUTDOWN);
		result = ::closesocket(m_socket);			// SYSCALL(result, closesocket(GetSocketValue()), IS_VALID_CLOSESOCKET);
		if (result != SOCKET_ERROR)					// IS_VALID_CLOSESOCKET(result))
		{
#ifndef _WIN32
			m_destroyEv.SetEvent();
#endif
			m_socket = INVALID_SOCKET;
			retval = true;
		}
	}
	return retval;
}


