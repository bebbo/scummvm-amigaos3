#include "amigaos3-timer.h"

#include "assert.h"
#include "common/debug.h"
#include "common/util.h"

#include <clib/alib_protos.h>  // DeleteTask
#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/realtime.h>

AmigaOS3TimerManager *AmigaOS3TimerManager::_s_instance = nullptr;

void __saveds AmigaOS3TimerManager::TimerTask(void) {
	AmigaOS3TimerManager *const tm = _s_instance;

	// Increase own priority, so we'll hopefully never miss a trigger
	struct Task *self = FindTask(NULL);
	SetTaskPri(self, 21);

	// Tell main thread we're alive
	Signal(tm->_mainTask, SIGBREAKF_CTRL_F);

	while (true) {
		ULONG signals = Wait(tm->_timerSignalMask | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);
		if (signals & SIGBREAKF_CTRL_C) {
			// Main task wants to end thread
			break;
		}

		ULONG playerSignals = signals & ~(SIGBREAKF_CTRL_F | SIGBREAKF_CTRL_C);
		UBYTE playerId = 0;
		while (playerSignals) {
			if ((playerSignals & 1) && tm->_allTimers[playerId].player) {
				const auto &slot = tm->_allTimers[playerId];
				assert(slot.player);
				assert(slot.player->pl_PlayerID == playerId);

					LONG res = SetPlayerAttrs(slot.player, PLAYER_AlarmTime, slot.player->pl_AlarmTime + slot.tics,
											  PLAYER_Ready, TRUE, TAG_END);

				((Common::TimerManager::TimerProc)slot.player->pl_UserData)(slot.refCon);
			}
			playerId++;
			playerSignals >>= 1;
		}

		// This is just a wakup call, for instance when main thread adds or removes
		// a player and thus needed to update timerSignalMask
		if (signals & SIGBREAKF_CTRL_F) {
			Signal(tm->_mainTask, SIGBREAKF_CTRL_F);
		}
	}

	Signal(tm->_mainTask, SIGBREAKF_CTRL_F);
	Wait(SIGBREAKF_CTRL_C);
}

AmigaOS3TimerManager::AmigaOS3TimerManager()
  : _timerTask(nullptr), _mainTask(nullptr), _timerSignalMask(0), _numTimers(0) {
	assert(RealTimeBase);
	assert(!_s_instance);
	_s_instance = this;

	memset(_allTimers, 0, sizeof(_allTimers));
	_mainTask = FindTask(NULL);

	if ((_timerProcess = CreateNewProcTags(NP_Name, (Tag) "ScummVM TimerManager", NP_Priority, 10, NP_Entry,
										   (Tag)&TimerTask, NP_StackSize, 100000, TAG_END)) == NULL) {
		error("Can't create TimerManager subtask");
	}
	_timerTask = FindTask("ScummVM TimerManager");
	assert(_timerTask);

	// wait for Timertask to report ready
	Wait(SIGBREAKF_CTRL_F);
	debug(1, "AmigaOS3TimerManager() created \n");
}

AmigaOS3TimerManager::~AmigaOS3TimerManager() {
	if (_timerTask) {
		// tell the timerTask to finish
		Signal(_timerTask, SIGBREAKF_CTRL_C);
		// Wait for the Task to exit the loop, but don't kill it quite yet
		// until we killed all players that might reference it
		Wait(SIGBREAKF_CTRL_F);
	}
	if (_numTimers) {
		// Find the first active player to stop the conductor
		for (auto &slot : _allTimers) {
			if (slot.player) {
				SetConductorState(slot.player, CONDSTATE_STOPPED, 0);
			}
		}
	}
	for (auto &slot : _allTimers) {
		if (slot.player) {
			UBYTE signalBit = (UBYTE)slot.player->pl_PlayerID;
			DeletePlayer(slot.player);
			// FIXME:
			FreeSignal(signalBit);
		}
	}

	if (_timerTask) {
		// now tell the timerTask to quit
		Signal(_timerTask, SIGBREAKF_CTRL_C);
	}

	_s_instance = nullptr;
}

bool AmigaOS3TimerManager::installTimerProc(Common::TimerManager::TimerProc proc, int32 interval, void *refCon,
											const Common::String &id) {
	// DefaultTimerManager seems to check that neither the same id nor the same proc gets
	// installed twice and would error out on it.
	// I'm just relying on that premise and do not do those checks here.
	// never install or remove a timer from within the timer task
	assert(FindTask(NULL) != _timerTask);

	{
		Common::StackLock lock(_mutex);

		LONG tics = MAX(1, (interval * TICK_FREQ) / 1000000);

		// FIXME: this is all wrong, but miraculously working.
		// You are not allowed to allocate signals in thread A and wait on them in thread B.
		// Instead, thread B needs to allocate the signals it is waiting on.
		// This will require restructuring all of this code here.
		BYTE timerSignalBit = AllocSignal(-1L);
		if (timerSignalBit == -1) {
			error("AmigaOS3TimerManager() Couldn't allocate additional signal bit\n");
			return false;
		}
		assert(!_allTimers[timerSignalBit].player);

		// HACK HACK HACK: pretend  _timerTask actually allocated the signal bit
		assert(!(_timerTask->tc_SigAlloc & (1UL << timerSignalBit)));
		Forbid();
		_timerTask->tc_SigAlloc |= (1UL << timerSignalBit);
		Permit();

		struct Player *player = nullptr;

		{
			ULONG err = 0;
			player =
			  CreatePlayer(PLAYER_Name, (Tag)id.c_str(), PLAYER_Conductor, (Tag) "ScummVM TimerManager Conductor",
						   PLAYER_AlarmSigTask, (Tag)_timerTask, PLAYER_AlarmSigBit, (Tag)timerSignalBit, PLAYER_ID,
						   (Tag)timerSignalBit, PLAYER_UserData, (Tag)proc, PLAYER_ErrorCode, (Tag)&err, TAG_END);
			if (!player) {
				FreeSignal(timerSignalBit);
				error("AmigaOS3TimerManager() can't create player; error: %lu\n", err);
				return false;
			}

			if (!_numTimers) {
				// This is the first timer, get the ball rolling
				err = SetConductorState(player, CONDSTATE_RUNNING, 0);
				if (err) {
					DeletePlayer(player);
					FreeSignal(timerSignalBit);
					error("SetConductorState failed: %lu", err);
					return false;
				}
			}

			BOOL success =
			  SetPlayerAttrs(player, PLAYER_AlarmTime, RealTimeBase->rtb_Time + tics, PLAYER_Ready, TRUE, TAG_END);
			if (!success) {
				DeletePlayer(player);
				FreeSignal(timerSignalBit);

				error("SetPlayerAttrs failed");
				return false;
			}
		}

		auto &slot = _allTimers[timerSignalBit];
		slot.player = player;
		slot.tics = (ULONG)tics;
		slot.refCon = refCon;

		++_numTimers;
		debug(1,
			  "AmigaOS3TimerManager() '%s' (proc %p) as timer '%d' at %ld tics/s, %d timers alive, current time: %lu\n",
			  id.c_str(), (void *)proc, timerSignalBit, tics, _numTimers, RealTimeBase->rtb_Time);

		_timerSignalMask |= 1L << timerSignalBit;
	}
	// tell the timer task that the timerSignalMask has changed
	Signal(_timerTask, SIGBREAKF_CTRL_F);
	Wait(SIGBREAKF_CTRL_F);

	return true;
}

void AmigaOS3TimerManager::removeTimerProc(Common::TimerManager::TimerProc proc) {
	Common::StackLock lock(_mutex);

	// never install or remove a timer from within the timer task
	assert(FindTask(NULL) != _timerTask);

	// never install or remove a timer from within the timer task
	assert(FindTask(NULL) != _timerTask);

	if (_numTimers <= 0) {
		// It seems, sometimes the same procedure gets removed more than once
		debug(1, "AmigaOS3TimerManager::removeTimerProc() no timers left to remove for proc %p", (void *)proc);
		return;
	}

	for (auto &slot : _allTimers) {
		auto &player = slot.player;
		if (player && player->pl_UserData == (void *)proc) {
			UBYTE signalBit = (UBYTE)player->pl_PlayerID;

			// Prevent the timer thread from waiting on it.
			// Not sure if I'm allowed to free a signal that is still potentially waited on.
			// But since this is never the only signal the timerTask is waiting on,
			// it would hardly block it.
			_timerSignalMask &= ~(1UL << signalBit);
			Signal(_timerTask, SIGBREAKF_CTRL_F);  // signal the change of g_timerSignalMask
			Wait(SIGBREAKF_CTRL_F);				   // wait for acknowledge

			// HACK HACK HACK: pretend  _timerTask actually allocated the signal bit
			assert(_timerTask->tc_SigAlloc & (1UL << signalBit));
			Forbid();
			_timerTask->tc_SigAlloc &= ~(1UL << signalBit);
			Permit();

			FreeSignal(signalBit);

				DeletePlayer(player);
			player = nullptr;

			_numTimers--;
			debug(1, "AmigaOS3TimerManager::removeTimerProc() timer %d removed or proc %p; %d timers left\n", signalBit,
				  (void *)proc, _numTimers);
			break;
		}
	}
}
