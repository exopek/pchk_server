/**
 * @file	include/lcnrtl/timer.h
 * BS-Unabhängiger Timer mit Millisekunden-Genauigkeit.
 * @author	Lothar May
 * @since	2004-04-05
 * @version	$Id: timer.h 695 2014-02-04 15:58:25Z TobiasJ $
 */

#ifndef INC_LCNRTL_TIMER_H_
#define INC_LCNRTL_TIMER_H_

#include "timerhelper.h"
#include "defs.h"


/**
 * Timer-Klasse, die auf der Standard-C-Funktion "clock" basiert.
 * Der von clock zurueckgegebene Wert muss mindestens eine Aufloesung von
 * Millisekunden haben.
 */

class LCNRTL_IMPEXP Timer
	{
	public:

		/**
		 * Konstruktor.
		 * Erzeut ein neues Timer-Objekt. Initialisiert den Software-Timer.
		 */

		Timer();

		/**
		 * Setze maximale Zeitdifferenz zwischen zwei Aufrufen von GetTimeLeft()
		 */

		void SetMaxDiff(TimerInt64 maxDiff)
		{
			m_maxDiff = maxDiff;
		}

		/**
		 * Startet den Timer (Aufloesung: Millisekunden).
		 * Für endlos: LCNRTL_WAIT_INFINITE
		 */

		void Start(unsigned timeOutMsec);

		/**
		 * Gibt zurück, wieviel Zeit noch übrig ist.
		 */

		TimerInt64 GetTimeLeftNeg();

		unsigned GetTimeLeft()
		{
			TimerInt64 timeLeft = GetTimeLeftNeg();
			return timeLeft < 0 ? 0 : (unsigned)timeLeft;
		}

		/**
		 * Prueft, ob das Timeout abgelaufen ist.
		 */

		bool HasTimeoutElapsed()
		{
			return this->GetTimeLeft() == 0;
		}

		/**
		 * Stoppt den Timer. Ein Timer ist nach dem Starten aktiv, bis er gestoppt
		 * wird.
		 */

		void Stop();

		/**
		 * Prueft, ob der Timer aktiv ist.
		 */

		bool IsActive() const
		{
			return m_active;
		}

		/**
		 * Setzt den Timer manuell auf den einen Wert der abgelaufen entspricht
		 */

		void ManualElapse();


	protected:

		/**
		 * Maximale Zeitdifferenz zwischen zwei Aufrufen von GetTimeLeft()
		 */

		TimerInt64 m_maxDiff;

		/**
		 * Zeitmarkierung beim Starten des Timers.
		 */

		TimerInt64 m_clockStart;

		/**
		 * Das konfigurierte Timeout.
		 */

		TimerInt64 m_clockTimeout;

		/**
		 * Repraesentiert, ob der Timer aktiv ist.
		 */

		bool m_active;

	};


#endif
