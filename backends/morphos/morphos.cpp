/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002 R�diger Hanke
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * MorphOS interface
 *
 * $Header$
 *
 */

#include "stdafx.h"
#include "scumm/scumm.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <devices/ahi.h>
#include <dos/dostags.h>
#include <intuition/screens.h>
#include <cybergraphics/cybergraphics.h>
#include <devices/inputevent.h>
#include <intuition/intuition.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/timer.h>
#include <proto/cdda.h>
#include <proto/cybergraphics.h>

#include <emul/emulinterface.h>

#include <time.h>

#include "morphos.h"
#include "morphos_sound.h"
#include "morphos_scaler.h"

static TagItem FindCDTags[] = { { CDFA_VolumeName, 0 },
										  { TAG_DONE,        0 }
										};
static TagItem PlayTags[] =   { { CDPA_StartTrack, 1 },
										  { CDPA_StartFrame, 0 },
										  { CDPA_EndTrack,   1 },
										  { CDPA_EndFrame,   0 },
										  { CDPA_Loops,      1 },
										  { TAG_DONE,		   0 }
										};

TagItem musicProcTags[] = { { NP_Entry,       0							       	  },
									 { NP_Name,        (ULONG)"ScummVM Music Thread"  },
									 { NP_Priority,    0  							        },
									 {	TAG_DONE,       0 					       		  }
								  };
TagItem soundProcTags[] = { { NP_Entry,       0							       	  },
									 { NP_Name,        (ULONG)"ScummVM Sound Thread"  },
									 { NP_Priority,    0  							        },
									 { TAG_DONE,       0 					       		  }
								  };

#define BLOCKSIZE_X	32
#define BLOCKSIZE_Y	8

#define BLOCKS_X 			(ScummBufferWidth/BLOCKSIZE_X)
#define BLOCKS_Y			(ScummBufferHeight/BLOCKSIZE_Y)
#define BLOCK_ID(x, y)  ((y/BLOCKSIZE_Y)*BLOCKS_X+(x/BLOCKSIZE_X))

OSystem_MorphOS *OSystem_MorphOS::create(int game_id, SCALERTYPE gfx_scaler, bool full_screen)
{
	OSystem_MorphOS *syst = new OSystem_MorphOS(game_id, gfx_scaler, full_screen);

	return syst;
}

OSystem_MorphOS::OSystem_MorphOS(int game_id, SCALERTYPE gfx_mode, bool full_screen)
{
	GameID = game_id;
	ScummScreen = NULL;
	ScummWindow = NULL;
	ScummBuffer = NULL;
	ScummScreenBuffer[0] = NULL;
	ScummScreenBuffer[1] = NULL;
	ScummRenderTo = NULL;
	ScummNoCursor = NULL;
	ScummMusicThread = NULL;
	ScummSoundThread = NULL;
	ScummWinX = -1;
	ScummWinY = -1;
	ScummDefaultMouse = false;
	ScummOrigMouse = false;
	ScummShakePos = 0;
	ScummScaler = gfx_mode;
	ScummScale = (gfx_mode == ST_NONE) ? 0 : 1;
	ScummDepth = 0;
	Scumm16ColFmt16 = false;
	ScummScrWidth = 0;
	ScummScrHeight = 0;
	ScreenChanged = false;
	DirtyBlocks = NULL;
	BlockColors = NULL;
	UpdateRects = 0;
	Scaler = NULL;
	FullScreenMode = full_screen;
	CDrive = NULL;
	CDDATrackOffset = 0;
	strcpy(ScummWndTitle, "ScummVM MorphOS");
	TimerMsgPort = NULL;
	TimerIORequest = NULL;

	OpenATimer(&TimerMsgPort, (IORequest **) &TimerIORequest, UNIT_MICROHZ);

	TimerBase = TimerIORequest->tr_node.io_Device;
	ScummNoCursor = (UWORD *) AllocVec(16, MEMF_CHIP | MEMF_CLEAR);
	UpdateRegion = NewRegion();
	NewUpdateRegion = NewRegion();
	if (!UpdateRegion || !NewUpdateRegion)
		error("Could not create region for screen update");
}

OSystem_MorphOS::~OSystem_MorphOS()
{
	if (DirtyBlocks)
	{
		FreeVec(DirtyBlocks);

		for (int b = 0; b < BLOCKS_X*BLOCKS_Y; b++)
			FreeVec(BlockColors[b]);
		FreeVec(BlockColors);
	}

	if (Scaler)
		delete Scaler;

	if (UpdateRegion)
		DisposeRegion(UpdateRegion);

	if (NewUpdateRegion)
		DisposeRegion(NewUpdateRegion);

	if (CDrive && CDDABase)
	{
		CDDA_Stop(CDrive);
		CDDA_ReleaseDrive(CDrive);
	}

	if (TimerIORequest)
	{
		CloseDevice((IORequest *) TimerIORequest);
		DeleteIORequest((IORequest *) TimerIORequest);
	}

	if (TimerMsgPort)
		DeleteMsgPort(TimerMsgPort);

	if (ScummMusicThread)
	{
		Signal((Task *) ScummMusicThread, SIGBREAKF_CTRL_C);
		ObtainSemaphore(&ScummMusicThreadRunning);	 /* Wait for thread to finish */
		ReleaseSemaphore(&ScummMusicThreadRunning);
	}

	if (ScummSoundThread)
	{
		Signal((Task *) ScummSoundThread, SIGBREAKF_CTRL_C);
		ObtainSemaphore(&ScummSoundThreadRunning);	 /* Wait for thread to finish */
		ReleaseSemaphore(&ScummSoundThreadRunning);
	}

	if (ScummNoCursor)
		FreeVec(ScummNoCursor);

	if (ScummBuffer)
		FreeVec(ScummBuffer);

	if (ScummRenderTo && !ScummScreen)
		FreeBitMap (ScummRenderTo);

	if (ScummWindow)
		CloseWindow(ScummWindow);

	if (ScummScreen)
	{
		if (ScummScreenBuffer[0])
			FreeScreenBuffer(ScummScreen, ScummScreenBuffer[0]);
		if( ScummScreenBuffer[1] )
			FreeScreenBuffer(ScummScreen, ScummScreenBuffer[1]);
		CloseScreen(ScummScreen);
	}
}

bool OSystem_MorphOS::OpenATimer(MsgPort **port, IORequest **req, ULONG unit, bool required)
{
	*req = NULL;
	const char *err_msg = NULL;
	
	*port = CreateMsgPort();
	if (*port)
	{
		*req = (IORequest *) CreateIORequest(*port, sizeof (timerequest));
		if (*req)
		{
			if (OpenDevice(TIMERNAME, unit, *req, 0))
			{
				DeleteIORequest(*req);
				*req = NULL;
				err_msg = "Failed to open timer device";
			}
		}
		else
			err_msg = "Failed to create IO request";
	}
	else
		err_msg = "Failed to create message port";

	if (err_msg)
	{
		if (required)
			error(err_msg);
		warning(err_msg);
	}

	return *req != NULL;
}

uint32 OSystem_MorphOS::get_msecs()
{
	int ticks = clock();
	ticks *= (1000/CLOCKS_PER_SEC);
	return ticks;
}

void OSystem_MorphOS::delay_msecs(uint msecs)
{
	TimerIORequest->tr_node.io_Command = TR_ADDREQUEST;
	TimerIORequest->tr_time.tv_secs = 0;
	TimerIORequest->tr_time.tv_micro = msecs*1000;
	DoIO((IORequest *) TimerIORequest);
}

void OSystem_MorphOS::set_timer(int timer, int (*callback)(int))
{
	warning("set_timer() unexpectedly called");
}

void *OSystem_MorphOS::create_thread(ThreadProc *proc, void *param)
{
	static EmulFunc ThreadEmulFunc;

	ThreadEmulFunc.Trap      = TRAP_FUNC;
	ThreadEmulFunc.Address	 = (ULONG)proc;
	ThreadEmulFunc.StackSize = 16000;
	ThreadEmulFunc.Extension = 0;
	ThreadEmulFunc.Arg1	    = (ULONG)param;
	musicProcTags[0].ti_Data = (ULONG)&ThreadEmulFunc;
	ScummMusicThread = CreateNewProc(musicProcTags);
	return ScummMusicThread;
}

void *OSystem_MorphOS::create_mutex(void)
{
	SignalSemaphore *sem = (SignalSemaphore *) AllocVec(sizeof (SignalSemaphore), MEMF_PUBLIC);

	if (sem)
		InitSemaphore(sem);

	return sem;
}

void OSystem_MorphOS::lock_mutex(void *mutex)
{
	ObtainSemaphore((SignalSemaphore *) mutex);
}

void OSystem_MorphOS::unlock_mutex(void *mutex)
{
	ReleaseSemaphore((SignalSemaphore *)mutex);
}

void OSystem_MorphOS::delete_mutex(void *mutex)
{
	FreeVec(mutex);
}

uint32 OSystem_MorphOS::property(int param, Property *value)
{
	switch (param)
	{
		case PROP_TOGGLE_FULLSCREEN:
			CreateScreen(CSDSPTYPE_TOGGLE);
			return 1;

		case PROP_SET_WINDOW_CAPTION:
			sprintf(ScummWndTitle, "ScummVM MorphOS - %s", value->caption);
			if (ScummWindow)
				SetWindowTitles(ScummWindow, ScummWndTitle, ScummWndTitle);
			return 1;

		case PROP_OPEN_CD:
			FindCDTags[0].ti_Data = (ULONG) ((GameID == GID_LOOM256) ? "LoomCD" : "Monkey1CD");
			if (!CDDABase) CDDABase = OpenLibrary("cdda.library", 2);
			if (CDDABase)
			{
				CDrive = CDDA_FindNextDrive(NULL, FindCDTags);
				if (!CDrive && GameID == GID_MONKEY)
				{
					FindCDTags[0].ti_Data = (ULONG) "Madness";
					CDrive = CDDA_FindNextDrive(NULL, FindCDTags);
				}
				if (CDrive)
				{
					if (!CDDA_ObtainDrive(CDrive, CDDA_SHARED_ACCESS, NULL))
					{
						CDrive = NULL;
						warning("Failed to obtain CD drive - music will not play");
					}
					else if (GameID == GID_LOOM256)
					{
						// Offset correction *may* be required
						CDS_TrackInfo ti = { sizeof (CDS_TrackInfo) };

						if (CDDA_GetTrackInfo(CDrive, 1, 0, &ti))
							CDDATrackOffset = ti.ti_TrackStart.tm_Format.tm_Frame-22650;
					}
				}
				else
					warning( "Could not find game CD inserted in CD-ROM drive - cd audio will not play" );
			}
			else
				warning( "Failed to open cdda.library - cd audio will not play" );
			break;

		case PROP_SHOW_DEFAULT_CURSOR:
			if (value->show_cursor)
				ClearPointer(ScummWindow);
			else
				SetPointer(ScummWindow, ScummNoCursor, 1, 1, 0, 0);
			ScummOrigMouse = ScummDefaultMouse = value->show_cursor;
			break;

		case PROP_GET_SAMPLE_RATE:
			return SAMPLES_PER_SEC;
	}

	return 0;
}

void OSystem_MorphOS::play_cdrom(int track, int num_loops, int start_frame, int length)
{
	if (CDrive && start_frame >= 0)
	{
		if (start_frame > 0)
			start_frame -= CDDATrackOffset;

		PlayTags[0].ti_Data = track;
		PlayTags[1].ti_Data = start_frame;
		PlayTags[2].ti_Data = (length == 0) ? track+1 : track;
		PlayTags[3].ti_Data = length ? start_frame+length : 0;
		PlayTags[4].ti_Data = (num_loops == 0) ? 1 : num_loops;
		CDDA_Play(CDrive, PlayTags);
	}
}

void OSystem_MorphOS::stop_cdrom()
{
	CDDA_Stop(CDrive);
}

bool OSystem_MorphOS::poll_cdrom()
{
	ULONG status;

	if (CDrive == NULL)
		return false;

	CDDA_GetAttr(CDDA_Status, CDrive, &status);
	return status == CDDA_Status_Busy;
}

void OSystem_MorphOS::update_cdrom()
{
}

void OSystem_MorphOS::quit()
{
	exit(0);
}

#define CVT8TO32(col)   ((col<<24) | (col<<16) | (col<<8) | col)

void OSystem_MorphOS::set_palette(const byte *colors, uint start, uint num)
{
	const byte *data = colors;
	UWORD changed_colors[256];
	UWORD num_changed = 0;

	for (uint i = start; i != start+num; i++)
	{
		ULONG color32 = (data[0] << 16) | (data[1] << 8) | data[2];
		if (color32 != ScummColors[i])
		{
			if (ScummDepth == 8)
				SetRGB32(&ScummScreen->ViewPort, i, CVT8TO32(data[0]), CVT8TO32(data[1]), CVT8TO32(data[2]));
			ScummColors16[i] = Scumm16ColFmt16 ? (((data[0]*31)/255) << 11) | (((data[1]*63)/255) << 5) | ((data[ 2 ]*31)/255) : (((data[0]*31)/255) << 10) | (((data[1]*31)/255) << 5) | ((data[2]*31)/255);
			ScummColors[i] = color32;
			changed_colors[num_changed++] = i;
		}
		data += 4;
	}

	if (ScummScale || ScummDepth != 8)
	{
		if (DirtyBlocks && num_changed < 200)
		{
			for (int b = 0; b < BLOCKS_X*BLOCKS_Y; b++)
			{
				UWORD *block_colors = BlockColors[b];
				UWORD *color_ptr = changed_colors;
				for (int c = 0; c < num_changed; c++)
				{
					if (block_colors[*color_ptr++])
					{
						UWORD x, y;
						x = b % BLOCKS_X;
						y = b / BLOCKS_X;
						DirtyBlocks[b] = true;
						AddUpdateRect(x*BLOCKSIZE_X, y*BLOCKSIZE_Y, BLOCKSIZE_X, BLOCKSIZE_Y);
						break;
					}
				}
			}
		}
		else
			AddUpdateRect(0, 0, ScummBufferWidth, ScummBufferHeight);
	}
}

void OSystem_MorphOS::CreateScreen(CS_DSPTYPE dspType)
{
	ULONG mode = INVALID_ID;
	int   depths[] = { 8, 15, 16, 24, 32, 0 };
	int   i;
	Screen *wb = NULL;

	if (dspType != CSDSPTYPE_KEEP)
		FullScreenMode = (dspType == CSDSPTYPE_FULLSCREEN) || (dspType == CSDSPTYPE_TOGGLE && !FullScreenMode);

	if (ScummRenderTo && !ScummScreen)
		FreeBitMap(ScummRenderTo);
	ScummRenderTo = NULL;

	if (ScummWindow)
	{
		if (ScummScreen == NULL)
		{
			ScummWinX = ScummWindow->LeftEdge;
			ScummWinY = ScummWindow->TopEdge;
		}
		CloseWindow (ScummWindow);
		ScummWindow = NULL;
	}

	if (ScummScreen)
	{
		if (ScummScreenBuffer[0])
			FreeScreenBuffer(ScummScreen, ScummScreenBuffer[0]);
		if (ScummScreenBuffer[1])
			FreeScreenBuffer(ScummScreen, ScummScreenBuffer[1]);
		CloseScreen(ScummScreen);
		ScummScreen = NULL;
	}
	
	ScummScrWidth  = ScummBufferWidth << ScummScale;
	ScummScrHeight = ScummBufferHeight << ScummScale;

	if (FullScreenMode)
	{
		for (i = ScummScale; mode == INVALID_ID && depths[i]; i++)
			mode = BestCModeIDTags(CYBRBIDTG_NominalWidth, 	  ScummScrWidth,
										  CYBRBIDTG_NominalHeight,   ScummScrHeight,
										  CYBRBIDTG_Depth,   		  depths[i],
										  TAG_DONE
										 );
		ScummDepth = depths[i-1];

		if (mode == INVALID_ID)
			error("Could not find suitable screenmode");

		ScummScreen = OpenScreenTags(NULL, 	SA_AutoScroll, TRUE,
														SA_Depth,		ScummDepth,
														SA_Width,		STDSCREENWIDTH,
														SA_Height,		STDSCREENHEIGHT,
														SA_DisplayID,	mode,
														SA_ShowTitle,	FALSE,
														SA_Type,			CUSTOMSCREEN,
														SA_Title,		"ScummVM MorphOS",
														TAG_DONE
											 );

		if (ScummScreen == NULL)
			error("Failed to open screen");

		ULONG	RealDepth = GetBitMapAttr(&ScummScreen->BitMap, BMA_DEPTH);
		if (RealDepth != ScummDepth)
		{
			warning("Screen did not open in expected depth");
			ScummDepth = RealDepth;
		}
		ScummScreenBuffer[0] = AllocScreenBuffer(ScummScreen, NULL, SB_SCREEN_BITMAP);
		ScummScreenBuffer[1] = AllocScreenBuffer(ScummScreen, NULL, 0);
		ScummRenderTo = ScummScreenBuffer[1]->sb_BitMap;
		ScummPaintBuffer = 1;

		if (ScummScreenBuffer[0] == NULL || ScummScreenBuffer[1] == NULL)
			error("Failed to allocate screen buffer");

		// Make both buffers black to avoid grey strip on bottom of screen
		RastPort rp;
		InitRastPort(&rp);
		SetRGB32(&ScummScreen->ViewPort, 0, 0, 0, 0);
		rp.BitMap = ScummScreenBuffer[0]->sb_BitMap;
		FillPixelArray(&ScummScreen->RastPort, 0, 0, ScummScreen->Width, ScummScreen->Height, 0);
		rp.BitMap = ScummRenderTo;
		FillPixelArray(&rp, 0, 0, ScummScreen->Width, ScummScreen->Height, 0);

		if (ScummDepth == 8)
		{
			for (int color = 0; color < 256; color++)
			{
				ULONG r, g, b;

				r = (ScummColors[color] >> 16) & 0xff;
				g = (ScummColors[color] >> 8) & 0xff;
				b = (ScummColors[color] >> 0) & 0xff;
				SetRGB32(&ScummScreen->ViewPort, color, CVT8TO32(r), CVT8TO32(g), CVT8TO32(b));
			}
		}
	}
	else
	{
		wb = LockPubScreen(NULL);
		if (wb == NULL)
			error("Could not lock default public screen");

		ScreenToFront(wb);
	}

	ScummWindow = OpenWindowTags(NULL,	WA_Left,				(wb && ScummWinX >= 0) ? ScummWinX : 0,
													WA_Top,				wb ? ((ScummWinY >= 0) ? ScummWinY : wb->BarHeight+1) : 0,
													WA_InnerWidth,	   FullScreenMode ? ScummScreen->Width : ScummScrWidth,
													WA_InnerHeight,   FullScreenMode ? ScummScreen->Height : ScummScrHeight,
													WA_Activate,		TRUE,
													WA_Title,		   wb ? ScummWndTitle : NULL,
													WA_ScreenTitle,	wb ? ScummWndTitle : NULL,
													WA_Borderless,		FullScreenMode,
													WA_CloseGadget,	!FullScreenMode,
													WA_DepthGadget,	!FullScreenMode,
													WA_DragBar,			!FullScreenMode,
													WA_ReportMouse,	TRUE,
													WA_RMBTrap,			TRUE,
													WA_IDCMP,			IDCMP_RAWKEY 		|
																			IDCMP_MOUSEMOVE 	|
																			IDCMP_CLOSEWINDOW |
																			IDCMP_MOUSEBUTTONS,
													WA_CustomScreen,  FullScreenMode ? (ULONG)ScummScreen : (ULONG)wb,
													TAG_DONE
										 );

	if (wb)
		UnlockPubScreen(NULL, wb);

	if (ScummWindow == NULL)
		error("Failed to open window");

	if (!ScummDefaultMouse)
	{
		SetPointer(ScummWindow, ScummNoCursor, 1, 1, 0, 0);
		ScummOrigMouse = false;
	}

	if (ScummScreen == NULL)
	{
		ScummDepth = GetCyberMapAttr(ScummWindow->RPort->BitMap, CYBRMATTR_DEPTH);
		if (ScummDepth == 8)
			error("Workbench screen must be running in 15 bit or higher");

		ScummRenderTo = AllocBitMap(ScummScrWidth, ScummScrHeight, ScummDepth, BMF_MINPLANES, ScummWindow->RPort->BitMap);
		if (ScummRenderTo == NULL)
			error("Failed to allocate bitmap");
	}

	if ((ScummDepth == 15 && Scumm16ColFmt16) || (ScummDepth == 16 && !Scumm16ColFmt16))
	{
		for (int col = 0; col < 256; col++)
		{
			int r = (ScummColors[col] >> 16) & 0xff;
			int g = (ScummColors[col] >> 8) & 0xff;
			int b = ScummColors[col] & 0xff;
			ScummColors16[col] = (Scumm16ColFmt16 == false) ? (((r*31)/255) << 11) | (((g*63)/255) << 5) | ((b*31)/255) : (((r*31)/255) << 10) | (((g*31)/255) << 5) | ((b*31)/255);
		}

		Scumm16ColFmt16 = (ScummDepth == 16);
	}

	if (Scaler)
	{
		delete Scaler;
		Scaler = NULL;
	}

	if (ScummScale)
	{
		Scaler = MorphOSScaler::Create(ScummScaler, ScummBuffer, ScummBufferWidth, ScummBufferHeight, ScummColors, ScummColors16, ScummRenderTo);
		if (Scaler == NULL)
		{
			warning("Failed to create scaler - scaling will be disabled");
			SwitchScalerTo(ST_NONE);
		}
	}

	AddUpdateRect(0, 0, ScummBufferWidth, ScummBufferHeight);
}

void OSystem_MorphOS::SwitchScalerTo(SCALERTYPE newScaler)
{
	if (newScaler == ST_NONE && ScummScale != 0)
	{
		if (Scaler)
		{
			delete Scaler;
			Scaler = NULL;
		}
		ScummScale = 0;
		ScummScaler = ST_NONE;
		CreateScreen(ScummScreen ? CSDSPTYPE_FULLSCREEN : CSDSPTYPE_WINDOWED);
	}
	else
	{
		if (ScummScale == 0)
		{
			ScummScale = 1;
			ScummScaler = newScaler;
			CreateScreen(ScummScreen ? CSDSPTYPE_FULLSCREEN : CSDSPTYPE_WINDOWED);
		}
		else if (ScummScaler != newScaler)
		{
			ScummScaler = newScaler;
			if (Scaler)
				delete Scaler;
			Scaler = MorphOSScaler::Create(ScummScaler, ScummBuffer, ScummBufferWidth, ScummBufferHeight, ScummColors, ScummColors16, ScummRenderTo);
			if (Scaler == NULL)
			{
				warning("Failed to create scaler - scaling will be disabled");
				SwitchScalerTo(ST_NONE);
			}
			else
				AddUpdateRect(0, 0, ScummBufferWidth, ScummBufferHeight);
		}
	}
}

bool OSystem_MorphOS::poll_event(Event *event)
{
	IntuiMessage *ScummMsg;

	if (ScummMsg = (IntuiMessage *) GetMsg(ScummWindow->UserPort))
	{
		switch (ScummMsg->Class)
		{
			case IDCMP_RAWKEY:
			{
				InputEvent FakedIEvent;
				char charbuf;
            int  qual = 0;

				memset(&FakedIEvent, 0, sizeof (InputEvent));
				FakedIEvent.ie_Class = IECLASS_RAWKEY;
				FakedIEvent.ie_Code = ScummMsg->Code;

				if (ScummMsg->Qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))
					qual |= KBD_ALT;
				if (ScummMsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
					qual |= KBD_SHIFT;
				if (ScummMsg->Qualifier & IEQUALIFIER_CONTROL)
					qual |= KBD_CTRL;
				event->kbd.flags = qual;

				event->event_code = EVENT_KEYDOWN;

				if (ScummMsg->Code >= 0x50 && ScummMsg->Code <= 0x59)
				{
					/*
					 * Function key
					 */
					event->kbd.ascii = (ScummMsg->Code-0x50)+315;
					event->kbd.keycode = 0;
				}
				else if (MapRawKey(&FakedIEvent, &charbuf, 1, NULL) == 1)
				{
					if (qual == KBD_CTRL)
					{
						switch (charbuf)
						{
							case 'z':
								ReplyMsg((Message *) ScummMsg);
								quit();
						}
					}
					else if (qual == KBD_ALT)
					{
						if (charbuf >= '0' && charbuf <= '9')
						{
							SCALERTYPE new_scaler = MorphOSScaler::FindByIndex(charbuf-'0');
							ReplyMsg((Message *) ScummMsg);
							if (new_scaler != ST_INVALID)
								SwitchScalerTo(new_scaler);
							return false;
						}
						else if (charbuf == 'x')
						{
							ReplyMsg((Message *) ScummMsg);
							quit();
						}
						else if (charbuf == 0x0d)
						{
							ReplyMsg((Message *) ScummMsg);
							CreateScreen(CSDSPTYPE_TOGGLE);
                     return false;
						}
					}

					event->kbd.ascii = charbuf;
					event->kbd.keycode = event->kbd.ascii;
				}
				break;
			}

			case IDCMP_MOUSEMOVE:
			{
				LONG newx, newy;

				newx = (ScummMsg->MouseX-ScummWindow->BorderLeft) >> ScummScale;
				newy = (ScummMsg->MouseY-ScummWindow->BorderTop) >> ScummScale;

				if (!FullScreenMode && !ScummDefaultMouse)
				{
					if (newx < 0 || newx > ScummBufferWidth ||
						 newy < 0 || newy > ScummBufferHeight
						)
					{
						if (!ScummOrigMouse)
						{
							ScummOrigMouse = true;
							ClearPointer(ScummWindow);
						}
					}
					else if (ScummOrigMouse)
					{
						ScummOrigMouse = false;
						SetPointer(ScummWindow, ScummNoCursor, 1, 1, 0, 0);
					}
				}
				else if (FullScreenMode)
					newy = newy <? (ScummScrHeight >> ScummScale)-2;

				event->event_code = EVENT_MOUSEMOVE;
				event->mouse.x = newx;
				event->mouse.y = newy;
				break;
			}

			case IDCMP_MOUSEBUTTONS:
			{
				int newx, newy;

				newx = (ScummMsg->MouseX-ScummWindow->BorderLeft) >> ScummScale;
				newy = (ScummMsg->MouseY-ScummWindow->BorderTop) >> ScummScale;

				switch (ScummMsg->Code)
				{
					case SELECTDOWN:
						event->event_code = EVENT_LBUTTONDOWN;
						break;

					case SELECTUP:
						event->event_code = EVENT_LBUTTONUP;
						break;

					case MENUDOWN:
						event->event_code = EVENT_RBUTTONDOWN;
						break;

					case MENUUP:
						event->event_code = EVENT_RBUTTONUP;
						break;

					default:
						ReplyMsg((Message *)ScummMsg);
						return false;
				}
				event->mouse.x = newx;
				event->mouse.y = newy;
				break;
			}

			case IDCMP_CLOSEWINDOW:
				ReplyMsg((Message *)ScummMsg);
				exit(0);
		}

		if (ScummMsg)
			ReplyMsg((Message *) ScummMsg);

		return true;
	}

	return false;
}

void OSystem_MorphOS::set_shake_pos(int shake_pos)
{
	ScummShakePos = shake_pos;
	AddUpdateRect(0, 0, 320, 200);
}

#define MOUSE_INTERSECTS(x, y, w, h) \
	(!((MouseOldX+MouseOldWidth <= x ) || (MouseOldX >= x+w) || \
		(MouseOldY+MouseOldHeight <= y) || (MouseOldY >= y+h)))

/* Copy part of bitmap */
void OSystem_MorphOS::copy_rect(const byte *src, int pitch, int x, int y, int w, int h)
{
	byte *dst;

	if (x < 0) { w+=x; src-=x; x = 0; }
	if (y < 0) { h+=y; src-=y*pitch; y = 0; }
	if (w >= ScummBufferWidth-x) { w = ScummBufferWidth - x; }
	if (h >= ScummBufferHeight-y) { h = ScummBufferHeight - y; }

	if (w <= 0 || h <= 0)
		return;

	if (MouseDrawn)
	{
		if (MOUSE_INTERSECTS(x, y, w, h))
			UndrawMouse();
	}

	AddUpdateRect(x, y, w, h);

	dst = (byte *)ScummBuffer+y*ScummBufferWidth + x;
	if (DirtyBlocks)
	{
		int cx, cy;
		int block = BLOCK_ID(x, y);
		int line_block = block;
		int start_block = BLOCKSIZE_X-(x % BLOCKSIZE_X);
		int start_y_block = BLOCKSIZE_Y-(y % BLOCKSIZE_Y);
		int next_block;
		int next_y_block;
		UWORD *block_cols = BlockColors[block];

		if (start_block == 0)
			start_block = BLOCKSIZE_X;
		if (start_y_block == 0)
			start_y_block = BLOCKSIZE_Y;

		next_block = start_block;
		next_y_block = start_y_block;
		for (cy = 0; cy < h; cy++)
		{
			for (cx = 0; cx < w; cx++)
			{
				UWORD old_pixel = *dst;
				UWORD src_pixel = *src++;
				if (old_pixel != src_pixel)
				{
					*dst++ = src_pixel;
					block_cols[old_pixel]--;
					block_cols[src_pixel]++;
				}
				else
					dst++;
				if (--next_block == 0)
				{
					block++;
					block_cols = BlockColors[block];
					next_block = BLOCKSIZE_X;
				}
			}
			if (--next_y_block == 0)
			{
				line_block += BLOCKS_X;
				next_y_block = BLOCKSIZE_Y;
			}
			block = line_block;
			block_cols = BlockColors[block];
			next_block = start_block;
			dst += ScummBufferWidth-w;
			src += pitch-w;
		}
	}
	else
	{
		do
		{
			memcpy(dst, src, w);
			dst += ScummBufferWidth;
			src += pitch;
		} while (--h);
	}
}

void OSystem_MorphOS::move_screen(int dx, int dy, int height) {

	if ((dx == 0) && (dy == 0))
		return;

	if (dx == 0) {
		// vertical movement
		if (dy > 0) {
			// move down
			// copy from bottom to top
			for (int y = height - 1; y >= dy; y--)
				copy_rect((byte *)ScummBuffer + ScummBufferWidth * (y - dy), ScummBufferWidth, 0, y, ScummBufferWidth, 1);
		} else {
			// move up
			// copy from top to bottom
			for (int y = 0; y < height + dx; y++)
				copy_rect((byte *)ScummBuffer + ScummBufferWidth * (y - dy), ScummBufferWidth, 0, y, ScummBufferWidth, 1);
		}
	} else if (dy == 0) {
		// horizontal movement
		if (dx > 0) {
			// move right
			// copy from right to left
			for (int x = ScummBufferWidth - 1; x >= dx; x--)
				copy_rect((byte *)ScummBuffer + x - dx, ScummBufferWidth, x, 0, 1, height);
		} else {
			// move left
			// copy from left to right
			for (int x = 0; x < ScummBufferWidth; x++)
				copy_rect((byte *)ScummBuffer + x - dx, ScummBufferWidth, x, 0, 1, height);
		}
	} else {
		// free movement
		// not neccessary for now
	}


}


bool OSystem_MorphOS::AddUpdateRect(WORD x, WORD y, WORD w, WORD h)
{
	if (UpdateRects > 25)
		return false;

	if (x < 0) { w+=x; x = 0; }
	if (y < 0) { h+=y; y = 0; }
	if (w >= ScummBufferWidth-x) { w = ScummBufferWidth - x; }
	if (h >= ScummBufferHeight-y) { h = ScummBufferHeight - y; }

	if (w <= 0 || h <= 0)
		return false;

	if (++UpdateRects > 25)
	{
		x = 0; y = 0;
		w = ScummBufferWidth; h = ScummBufferHeight;
	}

	Rectangle update_rect = { x, y, x+w, y+h };
	OrRectRegion(NewUpdateRegion, &update_rect);
	ScreenChanged = true;

	return true;
}

void OSystem_MorphOS::update_screen()
{
	DrawMouse();

	if (!ScreenChanged)
		return;

	OrRegionRegion(NewUpdateRegion, UpdateRegion);

	if (ScummShakePos)
	{
		RastPort rp;

		InitRastPort(&rp);
		rp.BitMap = ScummRenderTo;

		uint32 src_y = 0;
		uint32 dest_y = 0;
		if (ScummShakePos < 0)
			src_y = -ScummShakePos;
		else
			dest_y = ScummShakePos;

		if (!ScummScale)
		{
			if (ScummDepth == 8)
				WritePixelArray(ScummBuffer, 0, src_y, ScummBufferWidth, &rp, 0, dest_y, ScummBufferWidth, ScummBufferHeight-src_y-dest_y, RECTFMT_LUT8);
			else
				WriteLUTPixelArray(ScummBuffer, 0, src_y, ScummBufferWidth, &rp, ScummColors, 0, dest_y, ScummBufferWidth, ScummBufferHeight-src_y-dest_y, CTABFMT_XRGB8);
		}
		else if (Scaler->Prepare(ScummRenderTo))
		{
			Scaler->Scale(0, src_y, 0, dest_y, ScummBufferWidth, ScummBufferHeight-src_y-dest_y);
			Scaler->Finish();
		}

		if (ScummShakePos < 0)
			FillPixelArray(&rp, 0, (ScummBufferHeight-1) << ScummScale, ScummScrWidth, -ScummShakePos << ScummScale, 0);
		else
			FillPixelArray(&rp, 0, 0, ScummScrWidth, ScummShakePos << ScummScale, 0);
	}
	else if (!ScummScale)
	{
		RastPort rp;

		InitRastPort(&rp);
		rp.BitMap = ScummRenderTo;

		uint32 src_x, src_y;
		uint32 src_w, src_h;
		uint32 reg_x, reg_y;
		RegionRectangle *update_rect = UpdateRegion->RegionRectangle;

		reg_x = UpdateRegion->bounds.MinX;
		reg_y = UpdateRegion->bounds.MinY;
		while (update_rect)
		{
			src_x = update_rect->bounds.MinX;
			src_y = update_rect->bounds.MinY;
			src_w = update_rect->bounds.MaxX-src_x;
			src_h = update_rect->bounds.MaxY-src_y;
			src_x += reg_x;
			src_y += reg_y;

			if (src_x) src_x--;
			if (src_y) src_y--;
			src_w += 2;
			if (src_x+src_w >= ScummBufferWidth)
				src_w = ScummBufferWidth-src_x;
			src_h += 2;
			if (src_y+src_h >= ScummBufferHeight)
				src_h = ScummBufferHeight-src_y;

			if (ScummDepth == 8)
				WritePixelArray(ScummBuffer, src_x, src_y, ScummBufferWidth, &rp, src_x, src_y, src_w, src_h, RECTFMT_LUT8);
			else
				WriteLUTPixelArray(ScummBuffer, src_x, src_y, ScummBufferWidth, &rp, ScummColors, src_x, src_y, src_w, src_h, CTABFMT_XRGB8);

			update_rect = update_rect->Next;
		}
	}
	else
	{
		uint32 src_x, src_y;
		uint32 src_w, src_h;
		uint32 reg_x, reg_y;
		RegionRectangle *update_rect = UpdateRegion->RegionRectangle;

		reg_x = UpdateRegion->bounds.MinX;
		reg_y = UpdateRegion->bounds.MinY;

		if (!Scaler->Prepare(ScummRenderTo))
			update_rect = NULL;

		while (update_rect)
		{
			src_x = update_rect->bounds.MinX;
			src_y = update_rect->bounds.MinY;
			src_w = update_rect->bounds.MaxX-src_x;
			src_h = update_rect->bounds.MaxY-src_y;
			src_x += reg_x;
			src_y += reg_y;

			if (src_x) src_x--;
			if (src_y) src_y--;
			src_w += 2;
			if (src_x+src_w >= ScummBufferWidth)
				src_w = ScummBufferWidth-src_x;
			src_h += 2;
			if (src_y+src_h >= ScummBufferHeight)
				src_h = ScummBufferHeight-src_y;

			Scaler->Scale(src_x, src_y, src_x, src_y, src_w, src_h);
			update_rect = update_rect->Next;
		}
		Scaler->Finish();
	}

	if (ScummScreen)
	{
		while (!ChangeScreenBuffer(ScummScreen, ScummScreenBuffer[ScummPaintBuffer]));
		ScummPaintBuffer = !ScummPaintBuffer;
		ScummRenderTo = ScummScreenBuffer[ScummPaintBuffer]->sb_BitMap;
	}
	else
	{
		int32 x = (UpdateRegion->bounds.MinX-1) << ScummScale;
		int32 y = (UpdateRegion->bounds.MinY-1) << ScummScale;
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		int32 w = (UpdateRegion->bounds.MaxX << ScummScale)-x+(1 << ScummScale);
		int32 h = (UpdateRegion->bounds.MaxY << ScummScale)-y+(1 << ScummScale);
		if (x+w > ScummScrWidth)  w = ScummScrWidth-x;
		if (y+h > ScummScrHeight) h = ScummScrHeight-y;
		BltBitMapRastPort(ScummRenderTo, x, y, ScummWindow->RPort, ScummWindow->BorderLeft+x, ScummWindow->BorderTop+y, w, h, ABNC | ABC);
		WaitBlit();
	}

	Region *new_region_part = NewUpdateRegion;
	NewUpdateRegion = UpdateRegion;
	ClearRegion(NewUpdateRegion);
	UpdateRegion = new_region_part;

	ScreenChanged = false;
	memset(DirtyBlocks, 0, BLOCKS_X*BLOCKS_Y*sizeof (bool));
	UpdateRects = 0;
}

void OSystem_MorphOS::DrawMouse()
{
	int x,y;
	byte *dst,*bak;
	byte color;

	if (MouseDrawn || !MouseVisible)
		return;
	MouseDrawn = true;

	const int ydraw = MouseY - MouseHotspotY;
	const int xdraw = MouseX - MouseHotspotX;
	const int w = MouseWidth;
	const int h = MouseHeight;
	bak = MouseBackup;
	byte *buf = MouseImage;

	MouseOldX = xdraw;
	MouseOldY = ydraw;
	MouseOldWidth = w;
	MouseOldHeight = h;

	AddUpdateRect(xdraw, ydraw, w, h);

	dst = (byte*)ScummBuffer + ydraw*ScummBufferWidth + xdraw;
	bak = MouseBackup;

	for (y = 0; y < h; y++, dst += ScummBufferWidth, bak += MAX_MOUSE_W, buf += w)
	{
		if ((uint)(ydraw+y) < ScummBufferHeight)
		{
			for( x = 0; x<w; x++ )
			{
				if ((uint)(xdraw+x) < ScummBufferWidth)
				{
					bak[x] = dst[x];
					if ((color=buf[x])!=0xFF)
						dst[ x ] = color;
				}
			}
		}
		else
			break;
	}
}

void OSystem_MorphOS::UndrawMouse()
{
	int x,y;
	byte *dst,*bak;

	if (!MouseDrawn)
		return;
	MouseDrawn = false;

	dst = (byte*)ScummBuffer + MouseOldY*ScummBufferWidth + MouseOldX;
	bak = MouseBackup;

	AddUpdateRect(MouseOldX, MouseOldY, MouseOldWidth, MouseOldHeight);

	for (y = 0; y < MouseOldHeight; y++, bak += MAX_MOUSE_W, dst += ScummBufferWidth)
	{
		if ((uint)(MouseOldY + y) < ScummBufferHeight)
		{
			for (x = 0; x < MouseOldWidth; x++)
			{
				if ((uint)(MouseOldX + x) < ScummBufferWidth)
					dst[x] = bak[x];
			}
		}
		else
			break;
	}
}

bool OSystem_MorphOS::show_mouse(bool visible)
{
	if (MouseVisible == visible)
		return visible;

	bool last = MouseVisible;
	MouseVisible = visible;

	if (!visible)
		UndrawMouse();

	return last;
}

void OSystem_MorphOS::set_mouse_pos(int x, int y)
{
	if (x != MouseX || y != MouseY)
	{
		MouseX = x;
		MouseY = y;
		UndrawMouse();
	}
}

void OSystem_MorphOS::set_mouse_cursor(const byte *buf, uint w, uint h, int hotspot_x, int hotspot_y)
{
	MouseWidth = w;
	MouseHeight	= h;

	MouseHotspotX = hotspot_x;
	MouseHotspotY = hotspot_y;

	MouseImage = (byte*)buf;

	UndrawMouse();
}

bool OSystem_MorphOS::set_sound_proc(void *param, OSystem::SoundProc *proc, byte format)
{
	static EmulFunc MySoundEmulFunc;

	SoundProc = proc;
	SoundParam = param;

	/*
	 * Create Sound Thread
	 */
	MySoundEmulFunc.Trap      = TRAP_FUNC;
	MySoundEmulFunc.Address	  = (ULONG)&morphos_sound_thread;
	MySoundEmulFunc.StackSize = 8192;
	MySoundEmulFunc.Extension = 0;
	MySoundEmulFunc.Arg1	     = (ULONG)this;
	MySoundEmulFunc.Arg2	     = AHIST_S16S;

	soundProcTags[0].ti_Data = (ULONG)&MySoundEmulFunc;
	ScummSoundThread = CreateNewProc(soundProcTags);

	if (!ScummSoundThread)
	{
		puts("Failed to create sound thread");
		exit(1);
	}

	return true;
}

void OSystem_MorphOS::fill_sound(byte *stream, int len)
{
	if (SoundProc)
		SoundProc(SoundParam, stream, len);
	else
		memset(stream, 0x0, len);
}

void OSystem_MorphOS::init_size(uint w, uint h)
{
	/*
	 * Allocate image buffer
	 */
	ScummBuffer = AllocVec(w*h, MEMF_CLEAR);
	
	if (ScummBuffer == NULL)
	{
		puts("Couldn't allocate image buffer");
		exit(1);
	}

	memset(ScummColors, 0, 256*sizeof (ULONG));

	ScummBufferWidth = w;
	ScummBufferHeight = h;

	DirtyBlocks = (bool *) AllocVec(BLOCKS_X*BLOCKS_Y*sizeof (bool), MEMF_CLEAR);
	if (DirtyBlocks)
	{
		BlockColors = (UWORD **) AllocVec(BLOCKS_X*BLOCKS_Y*sizeof (UWORD *), MEMF_CLEAR);
		if (BlockColors)
		{
			int b;

			for (b = 0; b < BLOCKS_X*BLOCKS_Y; b++)
			{
				BlockColors[b] = (UWORD *) AllocVec(256*sizeof (UWORD), MEMF_CLEAR);
				if (BlockColors[b] == NULL)
					break;
				BlockColors[b][0] = BLOCKSIZE_X*BLOCKSIZE_Y;
			}

			if (b < BLOCKS_X*BLOCKS_Y)
			{
				for (--b; b >= 0; --b)
					FreeVec(BlockColors[b]);
				FreeVec(BlockColors);
				BlockColors = NULL;
			}
		}

		if (!BlockColors)
		{
			FreeVec(DirtyBlocks);
			DirtyBlocks = NULL;
		}
	}

	CreateScreen(CSDSPTYPE_KEEP);
}

