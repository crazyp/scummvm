/* Copyright (C) 1994-2003 Revolution Software Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 */

#include "common/stdafx.h"
#include "common/file.h"
#include "sword2/sword2.h"
#include "sword2/defs.h"
#include "sword2/interpreter.h"

namespace Sword2 {

enum {
	INS_talk		= 1,
	INS_anim		= 2,
	INS_reverse_anim	= 3,
	INS_walk		= 4,
	INS_turn		= 5,
	INS_face		= 6,
	INS_trace		= 7,
	INS_no_sprite		= 8,
	INS_sort		= 9,
	INS_foreground		= 10,
	INS_background		= 11,
	INS_table_anim		= 12,
	INS_reverse_table_anim	= 13,
	INS_walk_to_anim	= 14,
	INS_set_frame		= 15,
	INS_stand_after_anim	= 16,
	INS_quit		= 42
};

int32 Logic::fnAddSubject(int32 *params) {
	// params:	0 id
	//		1 daves reference number

	if (IN_SUBJECT == 0) {
		// This is the start of the new subject list
		// Set the default repsonse id to zero in case we're never
		// passed one
		_defaultResponseId = 0;
	}

	// - this just means we'd get the response for the 1st icon in the
	// chooser which is better than crashing

	if (params[0] == -1)
	{
		// this isn't an icon at all, it's telling us the id of the
		// default response

		// and here it is - this is the ref number we will return if
		_defaultResponseId = params[1];

		// a luggage icon is clicked on someone when it wouldn't have
		// been in the chooser list (see fnChoose below)
	} else {
		_subjectList[IN_SUBJECT].res = params[0];
		_subjectList[IN_SUBJECT].ref = params[1];

		debug(5, "fnAddSubject res %d, uid %d", params[0], params[1]);

		IN_SUBJECT++;
	}

	return IR_CONT;
}

int32 Logic::fnChoose(int32 *params) {
	// params:	none

	// the human is switched off so there will be no normal mouse engine

	MouseEvent *me;
	uint32 i;
	int hit;
	uint8 *icon;

	AUTO_SELECTED = 0;	// see below

	// new thing to intercept objects held at time of clicking on a person

	if (OBJECT_HELD) {
		// So that, if there is no match, the speech script uses the
		// default text for objects that are not accounted for

		int response = _defaultResponseId;

		// If we are using a luggage icon on the person, scan the
		// subject list to see if this icon would have been available
		// at this time.
		//
 		// If it is there, return the relevant 'ref' number (as if it
		// had been selected from within the conversation). If not,
		// just return a special code to get the default text line(s)
		// for unsupported objects.
		//
		// Note that we won't display the subject icons in this case!

		// scan the subject list for a match with our 'object_held'

		for (i = 0; i < IN_SUBJECT; i++) {
			if (_subjectList[i].res == OBJECT_HELD) {
				// Return special subject chosen code (same
				// as in normal chooser routine below)
				response = _subjectList[i].ref;
				break;
			}
		}

		OBJECT_HELD = 0; // clear it so it doesn't keep happening!
		IN_SUBJECT = 0;	 // clear the subject list

		return IR_CONT | (response << 3);
	}

	// new thing for skipping chooser with "nothing else to say" text

	// If this is the 1st time the chooser is coming up in this
	// conversation, AND there's only 1 subject, AND it's the EXIT icon

	if (CHOOSER_COUNT_FLAG == 0 && IN_SUBJECT == 1 && _subjectList[0].res == EXIT_ICON) {
		AUTO_SELECTED = 1;	// for speech script
		IN_SUBJECT = 0;		// clear the subject list

		// return special subject chosen code (same as in normal
		// chooser routine below)
		return IR_CONT | (_subjectList[0].ref << 3);
	}

	if (!_choosing) {
		// new choose session
		// build menus from subject_list

		if (!IN_SUBJECT)
			error("fnChoose with no subjects :-O");

		// init top menu from master list
		// all icons are highlighted / full colour

		for (i = 0; i < 15; i++) {
			if (i < IN_SUBJECT) {
				debug(5, " ICON res %d for %d", _subjectList[i].res, i);
				icon = _vm->_resman->openResource(_subjectList[i].res) + sizeof(StandardHeader) + RDMENU_ICONWIDE * RDMENU_ICONDEEP;
				_vm->_graphics->setMenuIcon(RDMENU_BOTTOM, (uint8) i, icon);
				_vm->_resman->closeResource(_subjectList[i].res);
			} else {
				// no icon here
				debug(5, " NULL for %d", i);
				_vm->_graphics->setMenuIcon(RDMENU_BOTTOM, (uint8) i, NULL);
			}
		}

		// start menus appearing
		_vm->_graphics->showMenu(RDMENU_BOTTOM);

		// lets have the mouse pointer back
		_vm->setMouse(NORMAL_MOUSE_ID);

		_choosing = true;

		// again next cycle
		return IR_REPEAT;
	}

	// menu is there - we're just waiting for a click
	debug(5, "choosing");

	me = _vm->_input->mouseEvent();

	// we only care about left clicks
	// we ignore mouse releases

	if (!me || !(me->buttons & RD_LEFTBUTTONDOWN) || _vm->_input->_mouseY < 400) {
		debug(5, "end choose");
		return IR_REPEAT;
	}

	// Check for click on a menu. If so then end the choose, highlight only
	// the chosen, blank the mouse and return the ref code * 8

	hit = _vm->menuClick(IN_SUBJECT);

	if (hit < 0) {
		debug(5, "end choose");
		return IR_REPEAT;
	}

	debug(5, "Icons available:");

	// change icons
	for (i = 0; i < IN_SUBJECT; i++) {
		debug(5, "%s", _vm->fetchObjectName(_subjectList[i].res));

		// change all others to grey
		if (i != (uint32) hit) {
			icon = _vm->_resman->openResource(_subjectList[i].res) + sizeof(StandardHeader);
			_vm->_graphics->setMenuIcon(RDMENU_BOTTOM, (uint8) i, icon);
			_vm->_resman->closeResource(_subjectList[i].res);
		}
	}

	debug(2, "Selected: %s", _vm->fetchObjectName(_subjectList[hit].res));

	// this is our looping flag
	_choosing = false;

	IN_SUBJECT = 0;

	// blank mouse again
	_vm->setMouse(0);

	debug(5, "hit %d - ref %d  ref*8 %d", hit, _subjectList[hit].ref, _subjectList[hit].ref * 8);

	// for non-speech scripts that manually
	// call the chooser
	RESULT = _subjectList[hit].res;

	// return special subject chosen code
	return IR_CONT | (_subjectList[hit].ref << 3);
}

int32 Logic::fnStartConversation(int32 *params) {
	// Start conversation

	// reset 'chooser_count_flag' at the start of each conversation:

	// Note that fnStartConversation might accidently be called every time
	// the script loops back for another chooser but we only want to reset
	// the chooser count flag the first time this function is called ie.
	// when talk flag is zero

	// params:	none

	if (TALK_FLAG == 0)
		CHOOSER_COUNT_FLAG = 0;	// see fnChooser & speech scripts

	fnNoHuman(params);
	return IR_CONT;
}

int32 Logic::fnEndConversation(int32 *params) {
	// end conversation

	// params:	none

	_vm->_graphics->hideMenu(RDMENU_BOTTOM);

	if (_vm->_input->_mouseY > 399) {
		// will wait for cursor to move off the bottom menu
		_vm->_mouseMode = MOUSE_holding;
		debug(5, "   holding");
	}

	TALK_FLAG = 0;	// in-case DC forgets

	// restart george's base script
	// totalRestart();

	//drop out without saving pc and go around again
	return IR_CONT;
}

int32 Logic::fnTheyDo(int32 *params) {
	// doesn't send the command until target is waiting - once sent we
	// carry on

	// params:	0 target
	//		1 command
	//		2 ins1
	//		3 ins2
	//		4 ins3
	//		5 ins4
	//		6 ins5

	uint32 null_pc = 5;		// 4th script - get-speech-state
	char *raw_script_ad;
	StandardHeader *head;
	int32 target = params[0];

	// request status of target
	head = (StandardHeader *) _vm->_resman->openResource(target);
	if (head->fileType != GAME_OBJECT)
		error("fnTheyDo %d not an object", target);

	raw_script_ad = (char *) head;

	// call the base script - this is the graphic/mouse service call
	runScript(raw_script_ad, raw_script_ad, &null_pc);

	_vm->_resman->closeResource(target);

	// result is 1 for waiting, 0 for busy

	if (RESULT == 1 && !INS_COMMAND) {
		// its waiting and no other command is queueing
		// reset debug flag now that we're no longer waiting - see
		// debug.cpp

		_speechScriptWaiting = 0;

		SPEECH_ID = params[0];
		INS_COMMAND = params[1];
		INS1 = params[2];
		INS2 = params[3];
		INS3 = params[4];
		INS4 = params[5];
		INS5 = params[6];

		return IR_CONT;
	}

	// debug flag to indicate who we're waiting for - see debug.cpp
	_speechScriptWaiting = target;

	// target is busy so come back again next cycle
	return IR_REPEAT;
}

int32 Logic::fnTheyDoWeWait(int32 *params) {
	// give target a command and wait for it to register as finished

	// params:	0 pointer to ob_logic
	//		1 target
	//		2 command
	//		3 ins1
	//		4 ins2
	//		5 ins3
	//		6 ins4
	//		7 ins5

	// 'looping' flag is used as a sent command yes/no

	ObjectLogic *ob_logic;

	uint32 null_pc = 5;		// 4th script - get-speech-state
	char *raw_script_ad;
	StandardHeader *head;
	int32 target = params[1];

	// ok, see if the target is busy - we must request this info from the
	// target object

	head = (StandardHeader *) _vm->_resman->openResource(target);
	if (head->fileType != GAME_OBJECT)
		error("fnTheyDoWeWait %d not an object", target);

	raw_script_ad = (char *) head;

	// call the base script - this is the graphic/mouse service call
	runScript(raw_script_ad, raw_script_ad, &null_pc);

	_vm->_resman->closeResource(target);

	ob_logic = (ObjectLogic *) _vm->_memory->intToPtr(params[0]);

	if (!INS_COMMAND && RESULT == 1 && ob_logic->looping == 0) {
		// first time so set up targets command if target is waiting

		debug(5, "fnTheyDoWeWait sending command to %d", target);

		SPEECH_ID = params[1];
		INS_COMMAND = params[2];
		INS1 = params[3];
		INS2 = params[4];
		INS3 = params[5];
		INS4 = params[6];
		INS5 = params[7];

		ob_logic->looping = 1;

		// debug flag to indicate who we're waiting for - see debug.cpp
		_speechScriptWaiting = target;

		// finish this cycle - but come back again to check for it
		// being finished
		return IR_REPEAT;
	} else if (ob_logic->looping == 0) {
		// did not send the command
		// debug flag to indicate who we're waiting for - see debug.cpp
		_speechScriptWaiting = target;

		// come back next go and try again to send the instruction
		return IR_REPEAT;
	}

	// ok, the command has been sent - has the target actually done it yet?

	// result is 1 for waiting, 0 for busy

	if (RESULT == 1) {
		// its waiting now so we can be finished with all this
		debug(5, "fnTheyDoWeWait finished");

		// not looping anymore
		ob_logic->looping = 0;

		// reset debug flag now that we're no longer waiting - see
		// debug.cpp
		_speechScriptWaiting = 0;
		return IR_CONT;
	}

	debug(5, "fnTheyDoWeWait just waiting");

	// debug flag to indicate who we're waiting for - see debug.cpp
	_speechScriptWaiting = target;

	// see ya next cycle
	return IR_REPEAT;
}

int32 Logic::fnWeWait(int32 *params) {
	// loop until the target is free

	// params:	0 target

	uint32 null_pc = 5;		// 4th script - get-speech-state
	char *raw_script_ad;
	StandardHeader *head;
	int32 target = params[0];

	// request status of target
	head = (StandardHeader *) _vm->_resman->openResource(target);
	if (head->fileType != GAME_OBJECT)
		error("fnWeWait: %d not an object", target);

	raw_script_ad = (char *) head;

	// call the base script - this is the graphic/mouse service call
	runScript(raw_script_ad, raw_script_ad, &null_pc);

	_vm->_resman->closeResource(target);

	// result is 1 for waiting, 0 for busy

	if (RESULT == 1) {
		// reset debug flag now that we're no longer waiting - see
		// debug.cpp
		_speechScriptWaiting = 0;
		return IR_CONT;
	}

	// debug flag to indicate who we're waiting for - see debug.cpp
	_speechScriptWaiting = target;

	// target is busy so come back again next cycle
	return IR_REPEAT;
}

int32 Logic::fnTimedWait(int32 *params) {
	// loop until the target is free but only while the timer is high
	// useful when clicking on a target to talk to them - if they never
	// reply then this'll fall out avoiding a lock up

	// params:	0 ob_logic
	//		1 target
	//		2 number of cycles before give up

	uint32 null_pc = 5;		// 4th script - get-speech-state
	char *raw_script_ad;
	ObjectLogic *ob_logic;
	StandardHeader *head;
	int32 target = params[1];

	ob_logic = (ObjectLogic *) _vm->_memory->intToPtr(params[0]);

	if (!ob_logic->looping)
		ob_logic->looping = params[2];	// first time in

	// request status of target
	head = (StandardHeader *) _vm->_resman->openResource(target);
	if (head->fileType != GAME_OBJECT)
		error("fnTimedWait %d not an object", target);

	raw_script_ad = (char *) head;

	// call the base script - this is the graphic/mouse service call
	runScript(raw_script_ad, raw_script_ad, &null_pc);

	_vm->_resman->closeResource(target);

	// result is 1 for waiting, 0 for busy

	if (RESULT == 1) {
		// reset because counter is likely to be still high
		ob_logic->looping = 0;

		//means ok
		RESULT = 0;

		// reset debug flag now that we're no longer waiting - see
		// debug.cpp
		_speechScriptWaiting = 0;

		return IR_CONT;
	}

	ob_logic->looping--;

	if (!ob_logic->looping) {	// time up - caller must check RESULT
		// not ok
		RESULT = 1;

		// clear the event that hasn't been picked up - in theory,
		// none of this should ever happen
		killAllIdsEvents(target);

		debug(5, "EVENT timed out");

		// reset debug flag now that we're no longer waiting - see
		// debug.cpp
		_speechScriptWaiting = 0;

		return IR_CONT;
	}

	// debug flag to indicate who we're waiting for - see debug.cpp
	_speechScriptWaiting = target;

	// target is busy so come back again next cycle
	return IR_REPEAT;
}

int32 Logic::fnSpeechProcess(int32 *params) {
	// Receive and sequence the commands sent from the conversation
	// script.

	// We have to do this in a slightly tweeky manner as we can no longer
	// have generic scripts.

	// This function comes in with all the structures that will be
	// required.

	// params:	0 pointer to ob_graphic
	//		1 pointer to ob_speech
	//		2 pointer to ob_logic
	//		3 pointer to ob_mega
	//		4 pointer to ob_walkdata

	// note - we could save a var and ditch wait_state and check
	// 'command' for non zero means busy

	ObjectSpeech *ob_speech;
	int32 pars[9];

	ob_speech = (ObjectSpeech *) _vm->_memory->intToPtr(params[1]);

	debug(5, "  SP");

	while (1) {
		//we are currently running a command
		switch (ob_speech->command) {
		case 0:
			// Do nothing
			break;
		case INS_talk:
			pars[0] = params[0];		// ob_graphic
			pars[1] = params[1];		// ob_speech
			pars[2] = params[2];		// ob_logic
			pars[3] = params[3];		// ob_mega
			pars[4] = ob_speech->ins1;	// encoded text number
			pars[5] = ob_speech->ins2;	// wav res id
			pars[6] = ob_speech->ins3;	// anim res id
			pars[7] = ob_speech->ins4;	// anim table res id
			pars[8] = ob_speech->ins5;	// animation mode - 0 lip synced, 1 just straight animation

			debug(5, "speech-process talk");

			// run the function - (it thinks it's been called from
			// script - bloody fool)

			if (fnISpeak(pars) != IR_REPEAT) {
				debug(5, "speech-process talk finished");

				// command finished
				ob_speech->command = 0;

				// waiting for command
				ob_speech->wait_state = 1;
			}

			// come back again next cycle
			return IR_REPEAT;
		case INS_turn:
			pars[0] = params[2];		// ob_logic
			pars[1] = params[0];		// ob_graphic
			pars[2] = params[3];		// ob_mega
			pars[3] = params[4];		// ob_walkdata
			pars[4] = ob_speech->ins1;	// direction to turn to

			if (fnTurn(pars) != IR_REPEAT) {
				// command finished
				ob_speech->command = 0;

				// waiting for command
				ob_speech->wait_state = 1;
			}

			// come back again next cycle
			return IR_REPEAT;
		case INS_face:
			pars[0] = params[2];		// ob_logic
			pars[1] = params[0];		// ob_graphic
			pars[2] = params[3];		// ob_mega
			pars[3] = params[4];		// ob_walkdata
			pars[4] = ob_speech->ins1;	// target

			if (fnFaceMega(pars) != IR_REPEAT) {
				// command finished
				ob_speech->command = 0;

				// waiting for command
				ob_speech->wait_state = 1;
			}

			// come back again next cycle
			return IR_REPEAT;
		case INS_anim:
			pars[0] = params[2];		// ob_logic
			pars[1] = params[0];		// ob_graphic
			pars[2] = ob_speech->ins1;	// anim res

			if (fnAnim(pars) != IR_REPEAT) {
				// command finished
				ob_speech->command = 0;

				// waiting for command
				ob_speech->wait_state = 1;
			}

			// come back again next cycle
			return IR_REPEAT;
		case INS_reverse_anim:
			pars[0] = params[2];		// ob_logic
			pars[1] = params[0];		// ob_graphic
			pars[2] = ob_speech->ins1;	// anim res

			if (fnReverseAnim(pars) != IR_REPEAT) {
				// command finished
				ob_speech->command = 0;

				// waiting for command
				ob_speech->wait_state = 1;
			}

			// come back again next cycle
			return IR_REPEAT;
		case INS_table_anim:
			pars[0] = params[2];		// ob_logic
			pars[1] = params[0];		// ob_graphic
			pars[2] = params[3];		// ob_mega
			pars[3] = ob_speech->ins1;	// pointer to anim table

			if (fnMegaTableAnim(pars) != IR_REPEAT) {
				// command finished
				ob_speech->command = 0;

				// waiting for command
				ob_speech->wait_state = 1;
			}

			// come back again next cycle
			return IR_REPEAT;
		case INS_reverse_table_anim:
			pars[0] = params[2];		// ob_logic
			pars[1] = params[0];		// ob_graphic
			pars[2] = params[3];		// ob_mega
			pars[3] = ob_speech->ins1;	// pointer to anim table

			if (fnReverseMegaTableAnim(pars) != IR_REPEAT) {
				// command finished
				ob_speech->command = 0;

				// waiting for command
				ob_speech->wait_state = 1;
			}

			// come back again next cycle
			return IR_REPEAT;
		case INS_no_sprite:
			fnNoSprite(params);		// ob_graphic
			ob_speech->command = 0;		// command finished
			ob_speech->wait_state = 1;	// waiting for command
			return IR_REPEAT ;
		case INS_sort:
			fnSortSprite(params);		// ob_graphic
			ob_speech->command = 0;		// command finished
			ob_speech->wait_state = 1;	// waiting for command
			return IR_REPEAT;
		case INS_foreground:
			fnForeSprite(params);		// ob_graphic
			ob_speech->command = 0;		// command finished
			ob_speech->wait_state = 1;	// waiting for command
			return IR_REPEAT;
		case INS_background:
			fnBackSprite(params);		// ob_graphic
			ob_speech->command = 0;		// command finished
			ob_speech->wait_state = 1;	// waiting for command
			return IR_REPEAT;
		case INS_walk:
			pars[0] = params[2];		// ob_logic
			pars[1] = params[0];		// ob_graphic
			pars[2] = params[3];		// ob_mega
			pars[3] = params[4];		// ob_walkdata
			pars[4] = ob_speech->ins1;	// target x
			pars[5] = ob_speech->ins2;	// target y
			pars[6] = ob_speech->ins3;	// target direction

			if (fnWalk(pars) != IR_REPEAT) {
				debug(5, "speech-process walk finished");

				// command finished
				ob_speech->command = 0;

				//waiting for command
				ob_speech->wait_state = 1;
			}

			// come back again next cycle
			return IR_REPEAT;
		case INS_walk_to_anim:
			pars[0] = params[2];		// ob_logic
			pars[1] = params[0];		// ob_graphic
			pars[2] = params[3];		// ob_mega
			pars[3] = params[4];		// ob_walkdata
			pars[4] = ob_speech->ins1;	// anim resource

			if (fnWalkToAnim(pars) != IR_REPEAT) {
				debug(5, "speech-process walk finished");

				// command finished
				ob_speech->command = 0;

				// waiting for command
				ob_speech->wait_state = 1;
			}

			// come back again next cycle
			return IR_REPEAT;
		case INS_stand_after_anim:
			pars[0] = params[0];		// ob_graphic
			pars[1] = params[3];		// ob_mega
			pars[2] = ob_speech->ins1;	// anim resource
			fnStandAfterAnim(pars);
			ob_speech->command = 0;		// command finished
			ob_speech->wait_state = 1;	// waiting for command
			return IR_REPEAT;		// come back again next cycle
		case INS_set_frame:
			pars[0] = params[0];		// ob_graphic
			pars[1] = ob_speech->ins1;	// anim_resource
			pars[2] = ob_speech->ins2;	// FIRST_FRAME or LAST_FRAME
			fnSetFrame(pars);
			ob_speech->command = 0;		// command finished
			ob_speech->wait_state = 1;	// waiting for command
			return IR_REPEAT;		// come back again next cycle
		case INS_quit:
			debug(5, "speech-process - quit");

			ob_speech->command = 0;		// finish with all this
			// ob_speech->wait_state = 0;	// start with waiting for command next conversation
			return IR_CONT;			// thats it, we're finished with this
		default:
			ob_speech->command = 0;		// not yet implemented - just cancel
			ob_speech->wait_state = 1;	// waiting for command
			break;
		}

		if (SPEECH_ID == ID) {
			// new command for us!
			// clear this or it could trigger next go
			SPEECH_ID = 0;

			// grab the command - potentially, we only have this
			// cycle to do this

			ob_speech->command = INS_COMMAND;
			ob_speech->ins1 = INS1;
			ob_speech->ins2 = INS2;
			ob_speech->ins3 = INS3;
			ob_speech->ins4 = INS4;
			ob_speech->ins5 = INS5;

			// the current send has been received - i.e. separate
			// multiple they-do's

			INS_COMMAND = 0;

			// now busy
			ob_speech->wait_state = 0;

			debug(5, "received new command %d", INS_COMMAND);

			// we'll drop off and be caught by the while(1), so
			// kicking in the new command straight away
		} else {
			// no new command
			// we could run a blink anim (or something) here

			// now free
			ob_speech->wait_state = 1;

			// come back again next cycle
			return IR_REPEAT;
		}
	}
}

enum {
	S_OB_GRAPHIC	= 0,
	S_OB_SPEECH	= 1,
	S_OB_LOGIC	= 2,
	S_OB_MEGA	= 3,

	S_TEXT		= 4,
	S_WAV		= 5,
	S_ANIM		= 6,
	S_DIR_TABLE	= 7,
	S_ANIM_MODE	= 8
};

int32 Logic::fnISpeak(int32 *params) {
	// its the super versatile fnSpeak
	// text and wavs can be selected in any combination

	// we can assume no human - there should be no human at least!

	// params:	0 pointer to ob_graphic
	//		1 pointer to ob_speech
	//		2 pointer to ob_logic
	//		3 pointer to ob_mega
	//		4 encoded text number
	//		5 wav res id
	//		6 anim res id
	//		7 anim table res id
	//		8 animation mode	0 lip synced,
	//					1 just straight animation

	MouseEvent *me;
	AnimHeader *anim_head;
	ObjectLogic *ob_logic;
	ObjectGraphic *ob_graphic;
	ObjectMega *ob_mega;
	uint8 *anim_file;
	uint32 local_text;
	uint32 text_res;
	uint8 *text;
	static bool speechRunning;
	int32 *anim_table;
	bool speechFinished = false;
	int8 speech_pan;
	char speechFile[256];
	static bool cycle_skip = false;
  	uint32 rv;

	// for text/speech testing & checking for correct file type
	StandardHeader *head;

	// set up the pointers which we know we'll always need

	ob_logic = (ObjectLogic *) _vm->_memory->intToPtr(params[S_OB_LOGIC]);
	ob_graphic = (ObjectGraphic *) _vm->_memory->intToPtr(params[S_OB_GRAPHIC]);

	// FIRST TIME ONLY: create the text, load the wav, set up the anim,
	// etc.

	if (!ob_logic->looping) {
		// New fudge to wait for smacker samples to finish
		// since they can over-run into the game

		if (_vm->_sound->getSpeechStatus() != RDSE_SAMPLEFINISHED)
			return IR_REPEAT;
		
		// New fudge for 'fx' subtitles
		// If subtitles switched off, and we don't want to use a wav
		// for this line either, then just quit back to script right
		// now!

		if (!_vm->_gui->_subtitles && !wantSpeechForLine(params[S_WAV]))
			return IR_CONT;

		if (!cycle_skip) {
			// drop out for 1st cycle to allow walks/anims to end
			// & display last frame/ before system locks while
			// speech loaded

			cycle_skip = true;
			return IR_REPEAT;
		} else
			cycle_skip = false;

		_vm->_debugger->_textNumber = params[S_TEXT];	// for debug info

		// For testing all text & speech!
		// A script loop can send any text number to fnISpeak and it
		// will only run the valid ones or return with 'result' equal
		// to '1' or '2' to mean 'invalid text resource' and 'text
		// number out of range' respectively
		//
		// See 'testing_routines' object in George's Player Character
		// section of linc

		if (SYSTEM_TESTING_TEXT) {
			RESULT = 0;

			text_res = params[S_TEXT] / SIZE;
			local_text = params[S_TEXT] & 0xffff;

			// if the resource number is within range & it's not
			// a null resource

			if (_vm->_resman->checkValid(text_res)) {
				// open the resource
				head = (StandardHeader *) _vm->_resman->openResource(text_res);

				if (head->fileType == TEXT_FILE) {
					// if it's not an animation file
					// if line number is out of range
					if (!_vm->checkTextLine((uint8 *) head, local_text)) {
						// line number out of range
						RESULT = 2;
					}
				} else {
					// invalid (not a text resource)
					RESULT = 1;
				}

				// close the resource
				_vm->_resman->closeResource(text_res);

				if (RESULT)
					return IR_CONT;
			} else {
				// not a valid resource number - invalid (null
				// resource)
				RESULT = 1;
				return IR_CONT;
			}
		}

		// Pull out the text line to get the official text number
		// (for wav id). Once the wav id's go into all script text
		// commands, we'll only need this for _SWORD2_DEBUG

		text_res = params[S_TEXT] / SIZE;
		local_text = params[S_TEXT] & 0xffff;

		// open text file & get the line
		text = _vm->fetchTextLine(_vm->_resman->openResource(text_res), local_text);
		_officialTextNumber = READ_LE_UINT16(text);

		// now ok to close the text file
		_vm->_resman->closeResource(text_res);

		// prevent dud lines from appearing while testing text & speech
		// since these will not occur in the game anyway

		if (SYSTEM_TESTING_TEXT) {	// if testing text & speech
			// if actor number is 0 and text line is just a 'dash'
			// character
			if (_officialTextNumber == 0 && text[2] == '-' && text[3] == 0) {
				// dud line - return & continue script
				RESULT = 3;
				return IR_CONT;
			}
		}

		// set the 'looping_flag' & the text-click-delay
 
		ob_logic->looping = 1;

		// can't left-click past the text for the first half second
		_leftClickDelay = 6;

		// can't right-click past the text for the first quarter second
		_rightClickDelay = 3;

		// Write to walkthrough file (zebug0.txt)
		// if (player_id != george), then player is controlling Nico

		if (PLAYER_ID != CUR_PLAYER_ID)
			debug(5, "(%d) Nico: %s", _officialTextNumber, text + 2);
		else
			debug(5, "(%d) %s: %s", _officialTextNumber, _vm->fetchObjectName(ID), text + 2);

		// Set up the speech animation

		if (params[S_ANIM]) {
			// just a straight anim
			_animId = params[S_ANIM];

			// anim type
			_speechAnimType = SPEECHANIMFLAG;

			// set the talker's graphic to this speech anim now
			ob_graphic->anim_resource = _animId;

			// set to first frame
			ob_graphic->anim_pc = 0;
		} else if (params[S_DIR_TABLE]) {
			// use this direction table to derive the anim
			// NB. ASSUMES WE HAVE A MEGA OBJECT!!

			ob_mega = (ObjectMega *) _vm->_memory->intToPtr(params[S_OB_MEGA]);

			// pointer to anim table
			anim_table = (int32 *) _vm->_memory->intToPtr(params[S_DIR_TABLE]);

			// appropriate anim resource is in 'table[direction]'
			_animId = anim_table[ob_mega->current_dir];

			// anim type
			_speechAnimType = SPEECHANIMFLAG;

			// set the talker's graphic to this speech anim now
			ob_graphic->anim_resource = _animId;

			// set to first frame
			ob_graphic->anim_pc = 0;
		} else {
			// no animation choosen
			_animId = 0;
		}

		// Default back to looped lip synced anims.
		SPEECHANIMFLAG = 0;

		// set up '_textX' & '_textY' for speech-pan and/or
		// text-sprite position

		locateTalker(params);

		// is it to be speech or subtitles or both?

		// assume not running until know otherwise
		speechRunning = false;

		// New fudge for 'fx' subtitles
		// if speech is selected, and this line is allowed speech
		// (not if it's an fx subtitle!)

		if (!_vm->_sound->isSpeechMute() && wantSpeechForLine(_officialTextNumber)) {
			// if the wavId paramter is zero because not yet
			// compiled into speech command, we can still get it
			// from the 1st 2 chars of the text line

			if (!params[S_WAV])
				params[S_WAV] = (int32) _officialTextNumber;

#define SPEECH_VOLUME	16	// 0..16
#define SPEECH_PAN	0	// -16..16

			speech_pan = ((_textX - 320) * 16) / 320;

			// '_textX'	'speech_pan'
			//  0		-16
			//  320		0
			//  640		16

			// keep within limits of -16..16, just in case
			if (speech_pan < -16)
				speech_pan = -16;
			else if (speech_pan > 16)
				speech_pan = 16;

			// set up path to speech cluster
			// first checking if we have speech1.clu or
			// speech2.clu in current directory (for translators
			// to test)

			File fp;

			sprintf(speechFile, "speech%d.clu", _vm->_resman->whichCd());

			if (fp.open(speechFile))
				fp.close();
			else
				strcpy(speechFile, "speech.clu");

			// Load speech but don't start playing yet
			rv = _vm->_sound->playCompSpeech(speechFile, params[S_WAV], SPEECH_VOLUME, speech_pan);
			if (rv == RD_OK) {
				// ok, we've got something to play
				speechRunning = true;

				// set it playing now (we might want to do
				// this next cycle, don't know yet)
				_vm->_sound->unpauseSpeech();
			} else {
				debug(5, "ERROR: PlayCompSpeech(speechFile=\"%s\", wav=%d (res=%d pos=%d)) returned %.8x", speechFile, params[S_WAV], text_res, local_text, rv);
			}
		}

		// if we want subtitles, or speech failed to load
		if (_vm->_gui->_subtitles || !speechRunning) {
			// then we're going to show the text
			// so create the text sprite
			formText(params);
		}
	}

	// EVERY TIME: run a cycle of animation, if there is one

	if (_animId) {
		// there is an animation
		// increment the anim frame number
		ob_graphic->anim_pc++;

		// open the anim file
		anim_file = _vm->_resman->openResource(ob_graphic->anim_resource);
		anim_head = _vm->fetchAnimHeader(anim_file);

		if (!_speechAnimType) {
			// ANIM IS TO BE LIP-SYNC'ED & REPEATING
			// if finished the anim
			if (ob_graphic->anim_pc == (int32) (anim_head->noAnimFrames)) {
				// restart from frame 0
				ob_graphic->anim_pc = 0;
			} else if (speechRunning) {
				// if playing a sample
				if (!_unpauseZone) {
					// if we're at a quiet bit
					if (_vm->_sound->amISpeaking() == RDSE_QUIET) {
						// restart from frame 0
						// ('closed mouth' frame)
						ob_graphic->anim_pc = 0;
					}
				}
			}
		} else {
			// ANIM IS TO PLAY ONCE ONLY
			if (ob_graphic->anim_pc == (int32) (anim_head->noAnimFrames) - 1) {
				// reached the last frame of the anim
				// hold anim on this last frame
				_animId = 0;
			}
		}

		// close the anim file
		_vm->_resman->closeResource(ob_graphic->anim_resource);
	} else if (_speechAnimType) {
		// Placed here so we actually display the last frame of the
		// anim.
		_speechAnimType = 0;
	}

	// EVERY TIME: FIND OUT IF WE NEED TO STOP THE SPEECH NOW...

	// if there is a wav then we're using that to end the speech naturally

	// if playing a sample

	if (speechRunning) {
		if (!_unpauseZone) {
			// has it finished?
			if (_vm->_sound->getSpeechStatus() == RDSE_SAMPLEFINISHED)
				speechFinished = true;
		} else
			_unpauseZone--;
	} else if (!speechRunning && _speechTime) {
		// counting down text time because there is no sample - this
		// ends the speech

		// if no sample then we're using _speechTime to end speech
		// naturally

		_speechTime--;
		if (!_speechTime)
			speechFinished = true;
	}

	// ok, all is running along smoothly - but a click means stop
	// unnaturally

	// so that we can go to the options panel while text & speech is
	// being tested
	if (SYSTEM_TESTING_TEXT == 0 || _vm->_input->_mouseY > 0) {
		me = _vm->_input->mouseEvent();

		// Note that we now have TWO click-delays - one for LEFT
		// button, one for RIGHT BUTTON

		if ((!_leftClickDelay && me && (me->buttons & RD_LEFTBUTTONDOWN)) ||
		    (!_rightClickDelay && me && (me->buttons & RD_RIGHTBUTTONDOWN))) {
			// mouse click, after click_delay has expired -> end
			// the speech we ignore mouse releases

			// if testing text & speech
			if (SYSTEM_TESTING_TEXT) {
				// and RB used to click past text
				if (me->buttons & RD_RIGHTBUTTONDOWN) {
					// then we want the previous line again
					SYSTEM_WANT_PREVIOUS_LINE = 1;
				} else {
					// LB just want next line again
					SYSTEM_WANT_PREVIOUS_LINE = 0;
				}
			}

			// Trash anything that's buffered
			while (_vm->_input->mouseEvent())
				;

			speechFinished = true;

			// if speech sample playing
			if (speechRunning) {
				// halt the sample prematurely
				_vm->_sound->stopSpeech();
			}
		}
	}

	// if we are finishing the speech this cycle, do the business

	// !speechAnimType, as we want an anim which is playing once to have
	// finished.

	if (speechFinished && !_speechAnimType) {
		// if there is text
		if (_speechTextBlocNo) {
			// kill the text block
			_vm->_fontRenderer->killTextBloc(_speechTextBlocNo);
			_speechTextBlocNo = 0;
		}

		// if there is a speech anim
		if (_animId) {
			// end it on 1st frame (closed mouth)
			_animId = 0;
			ob_graphic->anim_pc = 0;
		}

		speechRunning = false;

		// no longer in a script function loop
		ob_logic->looping = 0;

		// reset for debug info
		_vm->_debugger->_textNumber = 0;

		// reset to zero, in case text line not even extracted (since
		// this number comes from the text line)
		_officialTextNumber = 0;

		RESULT = 0;	// ok
		return IR_CONT;
	}

	// speech still going, so decrement the click_delay if it's still
	// active

	// count down to clickability

	if (_leftClickDelay)
		_leftClickDelay--;

 	if (_rightClickDelay)
		_rightClickDelay--;

	// back again next cycle
	return IR_REPEAT;
}

#define GAP_ABOVE_HEAD	20	// distance kept above talking sprite

void Logic::locateTalker(int32 *params) {
	// sets '_textX' & '_textY' for position of text sprite
	// but '_textX' also used to calculate speech-pan

	// params:	0 pointer to ob_graphic
	//		1 pointer to ob_speech
	//		2 pointer to ob_logic
	//		3 pointer to ob_mega
	//		4 encoded text number
	//		5 wav res id
	//		6 anim res id
	//		7 pointer to anim table
	//		8 animation mode	0 lip synced,
	//					1 just straight animation

	ObjectMega *ob_mega;

	uint8 *file;
	FrameHeader *frame_head;
	AnimHeader *anim_head;
	CdtEntry *cdt_entry;
	uint16 scale;

	// if there's no anim
	if (_animId == 0) {
		// assume it's Voice-Over text, so it goes at bottom of screen
		_textX = 320;
		_textY = 400;
	} else {
		// Note: this code has been adapted from Register_frame() in
		// build_display.cpp

		// open animation file & set up the necessary pointers
		file = _vm->_resman->openResource(_animId);

		anim_head = _vm->fetchAnimHeader(file);

		// '0' means 1st frame
		cdt_entry = _vm->fetchCdtEntry(file, 0);

		// '0' means 1st frame
		frame_head = _vm->fetchFrameHeader(file, 0);

		// check if this frame has offsets ie. this is a scalable
		// mega frame

		if (cdt_entry->frameType & FRAME_OFFSET) {
			// this may be NULL
			ob_mega = (ObjectMega *) _vm->_memory->intToPtr(params[S_OB_MEGA]);

			// calc scale at which to print the sprite, based on
			// feet y-coord & scaling constants (NB. 'scale' is
			// actually 256 * true_scale, to maintain accuracy)

			// Ay+B gives 256 * scale ie. 256 * 256 * true_scale
			// for even better accuracy, ie. scale = (Ay + B) / 256
			scale = (uint16) ((ob_mega->scale_a * ob_mega->feet_y + ob_mega->scale_b) / 256);

			// calc suitable centre point above the head, based on
			// scaled height

			// just use 'feet_x' as centre
			_textX = (int16) (ob_mega->feet_x);

			// add scaled y-offset to feet_y coord to get top of
			// sprite
			_textY = (int16) (ob_mega->feet_y + (cdt_entry->y * scale) / 256);
		} else {
			// it's a non-scaling anim - calc suitable centre
			// point above the head, based on scaled width

			// x-coord + half of width
			_textX = cdt_entry->x + (frame_head->width) / 2;
			_textY = cdt_entry->y;
		}

		// leave space above their head
		_textY -= GAP_ABOVE_HEAD;
			
		// adjust the text coords for RDSPR_DISPLAYALIGN

		_textX -= _vm->_thisScreen.scroll_offset_x;
		_textY -= _vm->_thisScreen.scroll_offset_y;

		// release the anim resource
		_vm->_resman->closeResource(_animId);
	}
}

void Logic::formText(int32 *params) {
	// its the first time in so we build the text block if we need one
	// we also bring in the wav if there is one
	// also setup the animation if there is one

	// anim is optional - anim can be a repeating lip-sync or a run-once
	// anim

	// if there is no wav then the text comes up instead
	// there can be any combination of text/wav playing

	// params	0 pointer to ob_graphic
	// 		1 pointer to ob_speech
	//		2 pointer to ob_logic
	//		3 pointer to ob_mega
	//		4 encoded text number
	//		5 wav res id
	//		6 anim res id
	//		7 pointer to anim table
	//		8 animation mode	0 lip synced,
	//					1 just straight animation

	uint32 local_text;
	uint32 text_res;
	uint8 *text;
	uint32 textWidth;
	ObjectSpeech *ob_speech;

	// should always be a text line, as all text is derived from line of
	// text

	if (params[S_TEXT]) {
	 	ob_speech = (ObjectSpeech *) _vm->_memory->intToPtr(params[S_OB_SPEECH]);

		// establish the max width allowed for this text sprite

		// if a specific width has been set up for this character,
		// then override the default

		if (ob_speech->width)
			textWidth = ob_speech->width;
		else
			textWidth = 400;

		// pull out the text line & make the sprite & text block

		text_res = params[S_TEXT] / SIZE;
		local_text = params[S_TEXT] & 0xffff;

		// open text file & get the line
		text = _vm->fetchTextLine(_vm->_resman->openResource(text_res), local_text);

		// 'text + 2' to skip the first 2 bytes which form the line
		// reference number

		_speechTextBlocNo = _vm->_fontRenderer->buildNewBloc(
			text + 2, _textX, _textY,
			textWidth, ob_speech->pen,
			RDSPR_TRANS | RDSPR_DISPLAYALIGN,
			_vm->_speechFontId, POSITION_AT_CENTRE_OF_BASE);

		// now ok to close the text file
		_vm->_resman->closeResource(text_res);

		// set speech duration, in case not using wav
		// no. of cycles = (no. of chars) + 30

		_speechTime = strlen((char *) text) + 30;
	} else {
		// no text line passed? - this is bad
		debug(5, "no text line for speech wav %d", params[S_WAV]);
	}
}

// For preventing sfx subtitles from trying to load speech samples
// - since the sfx are implemented as normal sfx, so we don't want them as
// speech samples too
// - and we only want the subtitles if selected, not if samples can't be found!

bool Logic::wantSpeechForLine(uint32 wavId) {
	switch (wavId) {
	case 1328:	// AttendantSpeech
			//	SFX(Phone71);
			//	FX <Telephone rings>
	case 2059:	// PabloSpeech
			//	SFX (2059);
			//	FX <Sound of sporadic gunfire from below>
	case 4082:	// DuaneSpeech
			//	SFX (4082);
			//	FX <Pffffffffffft! Frp. (Unimpressive, flatulent noise.)>
	case 4214:	// cat_52
			//	SFX (4214);
			//	4214FXMeow!
	case 4568:	// trapdoor_13
 			//	SFX (4568);
			//	4568fx<door slamming>
	case 4913:	// LobineauSpeech
			//	SFX (tone2);
			//	FX <Lobineau hangs up>
	case 5120:	// bush_66
			//	SFX (5120);
			//	5120FX<loud buzzing>
	case 528:	// PresidentaSpeech
			//	SFX (528);
			//	FX <Nearby Crash of Collapsing Masonry>
	case 920:	// location 62
	case 923:	// location 62
	case 926:	// location 62
		// don't want speech for these lines!
		return false;
	default:
		// ok for all other lines
		return true;
	}
}

} // End of namespace Sword2
