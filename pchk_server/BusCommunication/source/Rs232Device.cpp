/**
 * @file	src/common/rs232impl.cpp
 * RS232-Implementierung (alle Platformen).
 * @author	Lothar May
 * @since	2002-10-23
 * $Id: rs232impl.cpp 2385 2016-10-19 15:49:53Z TobiasJ $
 */

//#include "../Common/include/Common.h"
#include "../include/Rs232Device.h"
//#include "../Common/include/ErrorMessage.h"
#include <cstdio>
#include <cassert>
//#include "../Common/include/assert.h"
#include "../helper/include/timer.h"

#ifndef _WIN32
#error This source code is Win32 specific.
#endif

//=========== Aus handledata.cpp importiert ================================================
void Rs232Device::InitHandleData()
{
	// Events erzeugen mit folgenden Eigenschaften:
	// - keine Sicherheits-Attribute
	// - automatisches Zuruecksetzen
	// - Anfangs nicht gesetzt
	// - namenlos
	memset(&m_handleInputOverlapped, 0, sizeof(OVERLAPPED));
	memset(&m_handleOutputOverlapped, 0, sizeof(OVERLAPPED));
	memset(&m_handleComEventOverlapped, 0, sizeof(OVERLAPPED));
	m_handleInputOverlapped.hEvent = ::CreateEventA(NULL, true, false, NULL);
	m_handleOutputOverlapped.hEvent = ::CreateEventA(NULL, true, false, NULL);
	m_handleComEventOverlapped.hEvent = ::CreateEventA(NULL, true, false, NULL);
}

void Rs232Device::CleanupHandleData()
{
	if (m_handleInputOverlapped.hEvent)
	{
		::CloseHandle(m_handleInputOverlapped.hEvent);
		m_handleInputOverlapped.hEvent = NULL;
	}
	if (m_handleOutputOverlapped.hEvent)
	{
		::CloseHandle(m_handleOutputOverlapped.hEvent);
		m_handleOutputOverlapped.hEvent = NULL;
	}
	if (m_handleComEventOverlapped.hEvent)
	{
		::CloseHandle(m_handleComEventOverlapped.hEvent);
		m_handleComEventOverlapped.hEvent = NULL;
	}
}
//=========== Ende Handledata.cpp ==========================================================

Rs232Device::Rs232Device()
: m_status(CLOSED)
{
	m_handle = INVALID_HANDLE_VALUE;
	m_baudrate = RS232_BAUDRATE_9600;							// setzt genau die Parameter wie für PKU benötigt
	m_comMode = RS232_MODE_STD | RS232_MODE_BIT_PCK_CD_CTS;
	m_comRTSMode = RS232_RTSMODE_ALWAYSON;
	m_resetRTSDelay = RS232_RESET_RTS_DELAY_DEFAULT;
	m_checkCDDelay = RS232_CHECK_CD_DELAY_DEFAULT;
	m_readTimeout = RS232_READ_TIMEOUT_DEFAULT;
	m_writeTimeout = RS232_WRITE_TIMEOUT_DEFAULT;

	SetComPort(RS232_PORT_DEFAULT);								// der müsste dann nochmal angepasst werden
	InitHandleData();
}

Rs232Device::~Rs232Device()
{
	DestroyHandle();
}

void Rs232Device::Open()
{
	if (GetStatus() != Rs232Device::OPENED)
	{
		const char* port = m_comPort;
		unsigned baudrate = m_baudrate;

		int internalComMode = Rs232Device::MODE_STD;
		if (m_comMode & RS232_MODE_BIT_PCK_CD_CTS)
			internalComMode |= Rs232Device::MODE_PCK_CD_CTS;
		if (m_comMode & RS232_MODE_BIT_PCK_CHECK_DSR)
			internalComMode |= Rs232Device::MODE_PCK_CHECK_DSR;
		if (m_comMode & RS232_MODE_BIT_MODEM_CHECK_DSR)
			internalComMode |= Rs232Device::MODE_MODEM_CHECK_DSR;

		int internalComRTSMode = 0;
		switch (m_comRTSMode)
		{
		case RS232_RTSMODE_ALWAYSON:
			internalComRTSMode = Rs232Device::RTSMODE_ALWAYSON;
			break;
		case RS232_RTSMODE_TOGGLE:
			internalComRTSMode = Rs232Device::RTSMODE_TOGGLE;
			break;
		case RS232_RTSMODE_ALWAYSOFF:
			internalComRTSMode = Rs232Device::RTSMODE_ALWAYSOFF;
			break;
		}

		if (CreateHandle(port))						// setzt gleich m_handle!
		{
			if (InitHandle(baudrate, 8, Rs232Device::EPARITY_NONE, Rs232Device::STOPBITS_ONE, internalComMode, internalComRTSMode))
			{
				if (SetIOTimeoutsHandle())
				{
					SetStatus(Rs232Device::OPENED);
				}
			}
		}
	}
}

bool Rs232Device::Close()
{
	if (DestroyHandle())
	{
		SetStatus(Rs232Device::CLOSED);
		return true;
	}
	else
		return false;
}

int Rs232Device::Read(char* buf, unsigned size) const
{
	if (GetStatus() != Rs232Device::OPENED)
	{
		//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_ReadNotOpen);
		return -1;
	}

	unsigned long haveRead = 0;
	DWORD error;
	COMSTAT comStat;
	if (!ClearCommError(m_handle, &error, &comStat))
	{
		//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_ReadInQueInit, ::GetLastError());
		return -1;
	}

	if ((comStat.cbInQue == 0) && (m_readTimeout != 0))
	{
		if (!::SetCommMask(m_handle, EV_RXCHAR))
		{
			//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_ReadSetCommMask, ::GetLastError());
			return -1;
		}
		if (!::WaitCommEvent(m_handle, &m_handleComEventMask, &m_handleComEventOverlapped))
		{
			::WaitForSingleObject(m_handleComEventOverlapped.hEvent, m_readTimeout);			// Einige Zeit auf Daten warten, dabei schoen schlafen (nicht pollen).
		}
		if (!::SetCommMask(m_handle, 0))
		{
			Sleep(1);																			// Aus irgendeinem Grund schlaegt das manchmal fehl. Kurz warten und erneut probieren.
			if (!::SetCommMask(m_handle, 0))													// Wenn es erneut fehlschlägt, Exception werfen.
			{
				//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_ReadResetCommMask, ::GetLastError());
				return -1;
			}
		}
		::ResetEvent(&m_handleComEventOverlapped.hEvent);

		if (!ClearCommError(m_handle, &error, &comStat))
		{
			//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_ReadInQueWait, ::GetLastError());
			return -1;
		}
	}

	if (comStat.cbInQue > 0)
	{
		unsigned numBytesToRead = comStat.cbInQue > size ? size : comStat.cbInQue;
		DWORD syncHaveRead = 0;
		bool result = ::ReadFile(m_handle, buf, numBytesToRead, &syncHaveRead, &m_handleInputOverlapped) != 0;

		if (result)
		{
			haveRead = syncHaveRead;
		}
		else
		{
			DWORD error = ::GetLastError();
			DWORD asyncHaveRead = 0;
			if ((error == ERROR_IO_PENDING) && (::GetOverlappedResult(m_handle, &m_handleInputOverlapped, &asyncHaveRead, true)))
			{
				haveRead = asyncHaveRead;
			}
			else
			{
				//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_Read, error);
				return -1;
			}
		}
	}

	return (int)(haveRead);
}

int Rs232Device::Write(const char* buf, unsigned size) const
{
	if (GetStatus() != Rs232Device::OPENED)
	{
		//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_WriteNotOpen);
		return -1;
	}

	if (!IsDeviceConnected())
	{
		//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_SendNotConnected);
		return -1;
	}

	// Kollisionserkennung
	if (m_checkCDDelay != 0)
	{
		DWORD dwEvtMask = EV_RXCHAR;
		if (m_comMode & MODE_PCK_CD_CTS)
			dwEvtMask |= EV_CTS;
		if (!::SetCommMask(m_handle, dwEvtMask))
		{
			//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_WriteSetCommMask, ::GetLastError());
			return -1;
		}
		DWORD waitResult = WAIT_OBJECT_0;
		if (!::WaitCommEvent(m_handle, &m_handleComEventMask, &m_handleComEventOverlapped))
		{
			// mindestens 1 ms warten, ob jemand anders etwas sendet.
			waitResult = ::WaitForSingleObject(m_handleComEventOverlapped.hEvent, m_checkCDDelay);
		}
		if (!::SetCommMask(m_handle, 0))
		{
			//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_WriteResetCommMask, ::GetLastError());
			return -1;
		}
		::ResetEvent(&m_handleComEventOverlapped.hEvent);

		if (waitResult == WAIT_OBJECT_0)
			return 0; // Bus ist gerade belegt.
	}

	if (m_comRTSMode == RTSMODE_TOGGLE)
	{
		// RTS setzen: wir wollen senden!
		::EscapeCommFunction(m_handle, SETRTS);

		if (m_comMode & MODE_PCK_CHECK_DSR)
		{
			// Prüfen, ob DSR als Antwort gesetzt wurde.
			DWORD comStat;
			::GetCommModemStatus(m_handle, &comStat);
			if (!(comStat & MS_DSR_ON) || comStat & MS_CTS_ON)
			{
				::EscapeCommFunction(m_handle, CLRRTS);
				return 0; // Kein PC-Koppler angeschlossen oder Bus belegt.
			}
		}
	}

	DWORD syncHaveWritten = 0;
	bool result = ::WriteFile(m_handle, buf, size, &syncHaveWritten, &m_handleOutputOverlapped) != 0;
	DWORD error = ::GetLastError();

	if (m_comRTSMode == RTSMODE_TOGGLE)
	{
		Sleep(((size + 1) * RS232_RESET_RTS_DELAY_USEC_PER_BYTE) / 1000 + m_resetRTSDelay);
		// RTS zurücksetzen
		::EscapeCommFunction(m_handle, CLRRTS);
	}

	unsigned long haveWritten = 0;
	if (result)
	{
		haveWritten = syncHaveWritten;
	}
	else
	{
		DWORD asyncHaveWritten = 0;
		if ((error == ERROR_IO_PENDING) && (::GetOverlappedResult(m_handle, &m_handleOutputOverlapped, &asyncHaveWritten, true)))
		{
			haveWritten = asyncHaveWritten;
		}
		else
		{
			//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_Write, error);
			return -1;
		}
	}

	return static_cast<int>(haveWritten);
}

bool Rs232Device::IsReady() const
{
	return (GetStatus() == Rs232Device::OPENED && IsDeviceConnected());
}

unsigned Rs232Device::GetTypeID() const
{
	return IODEVICE_TYPEID_RS232;	// IO_TYPEID_IMPL_RS232;
}

IoDevice * Rs232Device::Clone() const
{
	Rs232Device *newImpl = new Rs232Device;
	newImpl->m_handle = m_handle;
	memcpy((void *)(&newImpl->m_handleInputOverlapped), (void *)(&m_handleInputOverlapped), sizeof(m_handleInputOverlapped));
	memcpy((void *)(&newImpl->m_handleOutputOverlapped), (void *)(&m_handleOutputOverlapped), sizeof(m_handleOutputOverlapped));
	memcpy((void *)(&newImpl->m_handleComEventOverlapped), (void *)(&m_handleComEventOverlapped), sizeof(m_handleComEventOverlapped));
	newImpl->m_handleComEventMask = m_handleComEventMask;
	newImpl->SetComPort(m_comPort);
	newImpl->m_baudrate = m_baudrate;
	newImpl->m_baudrate = m_baudrate;
	newImpl->m_comMode = m_comMode;
	newImpl->m_comRTSMode = m_comRTSMode;
	newImpl->m_resetRTSDelay = m_resetRTSDelay;
	newImpl->m_checkCDDelay = m_checkCDDelay;
	newImpl->m_readTimeout = m_readTimeout;
	newImpl->m_writeTimeout = m_writeTimeout;

	newImpl->SetStatus(GetStatus());
	return newImpl;
}

bool Rs232Device::SetComPort(const char* comPort)
{
#ifdef _WIN32
	for (unsigned comPortNum = 0; comPortNum <= RS232_PORT_MAX; ++comPortNum)
	{
		char buf[MAX_RS232_COM_STRING_LEN];
		sprintf_s(buf, MAX_RS232_COM_STRING_LEN, "COM%u", comPortNum + 1);
		if (strcmp(comPort, buf) == 0)
		{
			strcpy_s(m_comPort, MAX_RS232_COM_STRING_LEN, comPort);
			return true;
		}
	}
#else
	// Linux: Allow anything as port name
	strcpy_s(m_comPort, MAX_RS232_COM_STRING_LEN, comPort);
	return;
#endif
	//ToErrorMailbox(ErrMsgAbort, ERR_IO_RS232, ERR_IO_InvalidComPort);
	return false;
}


bool Rs232Device::SetBaudrate(unsigned baudrate)
{
	if (baudrate > RS232_BAUDRATE_MAX)
	{
		//ToErrorMailbox(ErrMsgAbort, ERR_IO_RS232, ERR_IO_InvalidBaudrate, baudrate);
		return false;
	}

	m_baudrate = baudrate;
	return true;
}

bool Rs232Device::SetComMode(int comMode)
{
	if (comMode > RS232_MODE_MAX)
	{
		//ToErrorMailbox(ErrMsgAbort, ERR_IO_RS232, ERR_IO_InvalidMode, comMode);
		return false;
	}

	m_comMode = comMode;
	return true;
}

bool Rs232Device::SetComRTSMode(int comRTSMode)
{
	if (comRTSMode > RS232_RTSMODE_MAX)
	{
		//ToErrorMailbox(ErrMsgAbort, ERR_IO_RS232, ERR_IO_InvalidMode, comRTSMode);
		return false;
	}

	m_comRTSMode = comRTSMode;
	return true;
}

bool Rs232Device::SetResetRTSDelay(unsigned delay)
{
	if (delay > RS232_RESET_RTS_DELAY_MAX)
	{
		//ToErrorMailbox(ErrMsgAbort, ERR_IO_RS232, ERR_IO_InvalidResetRTSDelay, delay);
		return false;
	}

	m_resetRTSDelay = delay;
	return true;
}

bool Rs232Device::SetCheckCDDelay(unsigned delay)
{
	if (delay > RS232_CHECK_CD_DELAY_MAX)
	{
		//ToErrorMailbox(ErrMsgAbort, ERR_IO_RS232, ERR_IO_InvalidCheckCTSDelay, delay);
		return false;
	}

	m_checkCDDelay = delay;
	return true;
}

void Rs232Device::SetReadTimeout(unsigned timeout)
{
	m_readTimeout = timeout;
}

void Rs232Device::SetWriteTimeout(unsigned timeout)
{
	m_writeTimeout = timeout;
}

//-------------- aus  Rs232Handle.cpp / .h ----------------------------------------------------

#define RS232_COM_STRING				"\\\\.\\%s"		// Unter Windows werden die seriellen Schnittstellen über \\.\COMx angesprochen.
#define RS232_COM_GUI_STRING			"%s"			// COMx würde nur bis COM9 funktionieren.
#define RS232_OUTPUT_BUF_SIZE			20				// Empfohlene Größe für den BS-internen Ausgangspuffer der seriellen Schnittstelle (in Bytes).
#define RS232_INPUT_BUF_SIZE			1024			// Empfohlene Größe für den BS-internen Eingangspuffer der seriellen Schnittstelle (in Bytes).

bool Rs232Device::CreateHandle(const char* device)
{
	// Den Namen der COM-Schnittstelle generieren
	TCHAR *pcCommPort = TEXT("COM1");
	char deviceName[MAX_RS232_COM_STRING_LEN];
	sprintf_s(deviceName, sizeof(deviceName), RS232_COM_STRING, device);

	// RS232-Device öffnen
	// Non-blocking (overlapped)
	HandleValue h = ::CreateFileA(pcCommPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	//HandleValue h = ::CreateFileA(pcCommPort, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (h == INVALID_HANDLE_VALUE) // nicht-fataler Fehler (-> USB!)
	{
		sprintf_s(deviceName, sizeof(deviceName), RS232_COM_GUI_STRING, device);
		printf("Invalid");
		//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_Open, ::GetLastError());
		return false;
	}

	// Handle setzen.
	SetHandleValue(h);
	return true;
}

bool Rs232Device::InitHandle(unsigned baudrate, unsigned char byteSize, Parity parity, StopBits stopBits, int comMode, int comRTSMode)
{
	DCB dcb;
	memset(&dcb, 0, sizeof(DCB));
	if (!::GetCommState(m_handle, &dcb))
	{
		//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_InitGetCommState, ::GetLastError());
		return false;
	}

	switch (baudrate)
	{
	case 0:		dcb.BaudRate = CBR_110;		break;
	case 1:		dcb.BaudRate = CBR_300;		break;
	case 2:		dcb.BaudRate = CBR_600;		break;
	case 3:		dcb.BaudRate = CBR_1200;	break;
	case 4:		dcb.BaudRate = CBR_2400;	break;
	case 5:		dcb.BaudRate = CBR_4800;	break;
	case 6:		dcb.BaudRate = CBR_9600;	break;
	case 7:		dcb.BaudRate = CBR_14400;	break;
	case 8:		dcb.BaudRate = CBR_19200;	break;
	case 9:		dcb.BaudRate = CBR_38400;	break;
	case 10:	dcb.BaudRate = CBR_56000;	break;
	case 11:	dcb.BaudRate = CBR_57600;	break;
	case 12:	dcb.BaudRate = CBR_115200;	break;
	case 13:	dcb.BaudRate = CBR_128000;	break;
	default:	dcb.BaudRate = CBR_256000;
	}
	dcb.fParity = TRUE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fNull = FALSE;
	dcb.fAbortOnError = FALSE;
	dcb.ByteSize = byteSize;
	switch (parity)
	{
	case EPARITY_NONE:	dcb.Parity = NOPARITY;		break;
	case EPARITY_ODD:	dcb.Parity = ODDPARITY;		break;
	case EPARITY_EVEN:	dcb.Parity = EVENPARITY;	break;
	case EPARITY_MARK:	dcb.Parity = MARKPARITY;	break;
	case EPARITY_SPACE:	dcb.Parity = SPACEPARITY;	break;
	default:			assert(false);
	}
	switch (stopBits)
	{
	case STOPBITS_ONE:	dcb.StopBits = ONESTOPBIT;		break;
	case STOPBITS_ONE5:	dcb.StopBits = ONE5STOPBITS;	break;
	case STOPBITS_TWO:	dcb.StopBits = TWOSTOPBITS;		break;
	default:			assert(false);
	}
	if (comRTSMode == RTSMODE_TOGGLE || comRTSMode == RTSMODE_ALWAYSOFF)
	{
		// Für die Kommunikation mit dem alten PCK:
		// RTS wird beim Senden getoggelt.
		dcb.fRtsControl = RTS_CONTROL_DISABLE;
		// XON/XOFF aus
		dcb.fTXContinueOnXoff = FALSE;
		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
	}
	else
	{
		// RTS aktivieren, damit Spannung nicht "zusammenbricht"
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
		// XON/XOFF an
		dcb.fTXContinueOnXoff = FALSE;
		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
	}

	// COM-Einstellungen setzen
	if (!::SetCommState(m_handle, &dcb))
	{
		//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_InitSetCommState, ::GetLastError());
		return false;
	}

	// COM-Event-Maske initialisieren
	if (!::SetCommMask(m_handle, 0))
	{
		//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_InitSetCommMask, ::GetLastError());
		return false;
	}

	// Betriebssystem-interne COM-Puffer setzen (Werte nur Empfehlungen für Treiber)
	if (!::SetupComm(m_handle, RS232_INPUT_BUF_SIZE, RS232_OUTPUT_BUF_SIZE))
	{
		//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_InitSetupComm, ::GetLastError());
		return false;
	}

	// Mode zwischenspeichern.
	m_comMode = comMode;
	m_comRTSMode = comRTSMode;

	return true;
}

bool Rs232Device::SetIOTimeoutsHandle()
{
	COMMTIMEOUTS to;

	to.ReadIntervalTimeout = 0;
	to.ReadTotalTimeoutMultiplier = 0;
	to.ReadTotalTimeoutConstant = m_readTimeout;
	to.WriteTotalTimeoutMultiplier = 0;
	to.WriteTotalTimeoutConstant = m_writeTimeout;

	// Timeouts setzen
	if (!::SetCommTimeouts(m_handle, &to))
	{
		//ToErrorMailbox(ErrMsgError, ERR_IO_RS232, ERR_IO_SetCommTimeouts, ::GetLastError());
		return false;
	}

	return true;
}

bool Rs232Device::InternalCloseHandle()
{
	::SetCommMask(m_handle, 0);
	// Handle-Daten neu initialisieren.
	CleanupHandleData();
	InitHandleData();
	return ::CloseHandle(m_handle) != 0;
}

bool Rs232Device::IsDeviceConnected() const
{
	bool retVal = m_handle != INVALID_HANDLE_VALUE;
	if (retVal && (m_comMode & MODE_MODEM_CHECK_DSR))
	{
		DWORD modemStatus = 0;
		if (!::GetCommModemStatus(m_handle, &modemStatus) || !(modemStatus & MS_DSR_ON))
			retVal = false;
	}
	return retVal;
}

void Rs232Device::SetHandleValue(HandleValue handle)
{
	if (m_handle != handle)
	{
		DestroyHandle();
		m_handle = handle;
	}
}

bool Rs232Device::DestroyHandle()
{
	bool retval = false;

	if (m_handle == INVALID_HANDLE_VALUE)
		retval = true;
	else
	{
		if (InternalCloseHandle())
		{
			m_handle = INVALID_HANDLE_VALUE;
			retval = true;
		}
	}
	return retval;
}


#include "../helper/include/timer.h"
//#include "include/TelegramTools.h"
//(const string& comPort, int checkCDDelay = RS232_CHECK_CD_DELAY_DEFAULT, bool useCTS = true, unsigned timeOutMsec = 1000, int sendModId = 1)

/*
bool Rs232Device::Test(const char *comPort)
{
	bool ret = false;
	//SetNoErrorMessage(true);						// hier wird es Fehlermeldungen geben. Die wollen wir gar nicht sehen.

	



//	try {
		//RS232Handle h;
		//h.Create(comPort.c_str());
/*		CreateHandle(comPort);

		int comMode = RS232_MODE_STD | RS232_MODE_BIT_PCK_CD_CTS;
		int comRTSMode = RS232_RTSMODE_ALWAYSON;  // Completely disable RTS. Means: Only LCN-PK/PKU will be able to send telegrams

		//h.Init(RS232_BAUDRATE_9600, 8, RS232Handle::EPARITY_NONE, RS232Handle::STOPBITS_ONE, comMode, comRTSMode);
		InitHandle(RS232_BAUDRATE_9600, 8, EPARITY_NONE, STOPBITS_ONE, comMode, comRTSMode);
		h.SetCheckCDDelay((unsigned)checkCDDelay);
*//*
		if (!SetComPort(comPort))
		{
			//SetNoErrorMessage(false);				// Fehlermeldungen wieder aktivieren
			return ret;
		}
		Open();
		/*
		Timer timer;
		timer.Start(1000);						// 1000 msec
		// Write: Empty command without ack. to self
//		unsigned char data[2] = { 0x00, 0x00 };
//		DataTelegram out(sendModId, false, false, 0, sendModId, 0x00, (char*)data, sizeof(data));
		LCNTelegram out;
		setTelegramHeader(out.dataByte, 0, 1, 1, TO_MODULE, NO_ACK, MSGLEN_8);
		out.c.commandByte = 0;
		out.c.subCommandByte = 0;
		out.dataByte[LCNTELEGRAM_DATABYTE1] = 0;
		out.dataByte[LCNTELEGRAM_DATABYTE2] = 0;
		doFinalTelegramCorrecting(out.dataByte);

		unsigned size;
		do
		{
			/*
			//size = (unsigned)h.Write(out.GetData(), out.GetSize());
			size = Write((char *)(out.dataByte), 8);
		} while (size == 0 && !timer.HasTimeoutElapsed());
		// Read back written telegram. If we don't find it, we probably have an old LCN-PC interface connected.
		if (size == 8 /*out.GetSize()*//*)
		{
			/*
			char in[256];  // Max. data to search
			unsigned pos = 0;
			do
			{
				//size = (unsigned)h.Read(in + pos, sizeof(in) - pos);

				size = Read(in + pos, sizeof(in) - pos);
				/*
				if (size > 0)
				{
					pos += size;
					// Check result
					for (unsigned i = 0; !ret && i + 8/*out.GetSize()*//* <= pos; ++i)
					/*
					{
						unsigned j;
						for (j = 0; j < 8/*out.GetSize()*//*; ++j)
						/*
						{
							if ((unsigned char)(in[i + j]) != out.dataByte[j])
								break;
						}
						if (j == 8/*out.GetSize()*//*)	// Found
						/*
							ret = true;
					}
				}
				//else
					/*
				{
					if (size < 0)						// Fehler aufgetreten?
						break;							// dann raus hier
					else
						::Sleep(100);					// 100 msec Pause beim Pollen
				}
//					MSLEEP(100);  // Don't poll too much
			} while (!ret && pos < sizeof(in) && !timer.HasTimeoutElapsed());
		}

		Close();
		
//	}
//	catch (const LcnException&) {}
	//SetNoErrorMessage(false);							// Fehlermeldungen wieder aktivieren
	return ret;
}
*/
