/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/*
 * This file is based on WME Lite.
 * http://dead-code.org/redir.php?target=wmelite
 * Copyright (c) 2011 Jan Nedoma
 */

#include "engines/wintermute/dcgf.h"
#include "engines/wintermute/Ad/AdRegion.h"
#include "engines/wintermute/Base/BParser.h"
#include "engines/wintermute/Base/BDynBuffer.h"
#include "engines/wintermute/Base/scriptables/ScValue.h"
#include "engines/wintermute/Base/scriptables/ScScript.h"
#include "engines/wintermute/Base/BGame.h"
#include "engines/wintermute/Base/BFileManager.h"

namespace WinterMute {

IMPLEMENT_PERSISTENT(CAdRegion, false)

//////////////////////////////////////////////////////////////////////////
CAdRegion::CAdRegion(CBGame *inGame): CBRegion(inGame) {
	_blocked = false;
	_decoration = false;
	_zoom = 0;
	_alpha = 0xFFFFFFFF;
}


//////////////////////////////////////////////////////////////////////////
CAdRegion::~CAdRegion() {
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdRegion::loadFile(const char *Filename) {
	byte *Buffer = Game->_fileManager->readWholeFile(Filename);
	if (Buffer == NULL) {
		Game->LOG(0, "CAdRegion::LoadFile failed for file '%s'", Filename);
		return E_FAIL;
	}

	HRESULT ret;

	_filename = new char [strlen(Filename) + 1];
	strcpy(_filename, Filename);

	if (FAILED(ret = loadBuffer(Buffer, true))) Game->LOG(0, "Error parsing REGION file '%s'", Filename);


	delete [] Buffer;

	return ret;
}


TOKEN_DEF_START
TOKEN_DEF(REGION)
TOKEN_DEF(TEMPLATE)
TOKEN_DEF(NAME)
TOKEN_DEF(ACTIVE)
TOKEN_DEF(ZOOM)
TOKEN_DEF(SCALE)
TOKEN_DEF(BLOCKED)
TOKEN_DEF(DECORATION)
TOKEN_DEF(POINT)
TOKEN_DEF(ALPHA_COLOR)
TOKEN_DEF(ALPHA)
TOKEN_DEF(EDITOR_SELECTED_POINT)
TOKEN_DEF(EDITOR_SELECTED)
TOKEN_DEF(SCRIPT)
TOKEN_DEF(CAPTION)
TOKEN_DEF(PROPERTY)
TOKEN_DEF(EDITOR_PROPERTY)
TOKEN_DEF_END
//////////////////////////////////////////////////////////////////////////
HRESULT CAdRegion::loadBuffer(byte  *Buffer, bool Complete) {
	TOKEN_TABLE_START(commands)
	TOKEN_TABLE(REGION)
	TOKEN_TABLE(TEMPLATE)
	TOKEN_TABLE(NAME)
	TOKEN_TABLE(ACTIVE)
	TOKEN_TABLE(ZOOM)
	TOKEN_TABLE(SCALE)
	TOKEN_TABLE(BLOCKED)
	TOKEN_TABLE(DECORATION)
	TOKEN_TABLE(POINT)
	TOKEN_TABLE(ALPHA_COLOR)
	TOKEN_TABLE(ALPHA)
	TOKEN_TABLE(EDITOR_SELECTED_POINT)
	TOKEN_TABLE(EDITOR_SELECTED)
	TOKEN_TABLE(SCRIPT)
	TOKEN_TABLE(CAPTION)
	TOKEN_TABLE(PROPERTY)
	TOKEN_TABLE(EDITOR_PROPERTY)
	TOKEN_TABLE_END

	byte *params;
	int cmd;
	CBParser parser(Game);

	if (Complete) {
		if (parser.GetCommand((char **)&Buffer, commands, (char **)&params) != TOKEN_REGION) {
			Game->LOG(0, "'REGION' keyword expected.");
			return E_FAIL;
		}
		Buffer = params;
	}

	for (int i = 0; i < _points.GetSize(); i++) delete _points[i];
	_points.RemoveAll();

	int ar = 255, ag = 255, ab = 255, alpha = 255;

	while ((cmd = parser.GetCommand((char **)&Buffer, commands, (char **)&params)) > 0) {
		switch (cmd) {
		case TOKEN_TEMPLATE:
			if (FAILED(loadFile((char *)params))) cmd = PARSERR_GENERIC;
			break;

		case TOKEN_NAME:
			setName((char *)params);
			break;

		case TOKEN_CAPTION:
			setCaption((char *)params);
			break;

		case TOKEN_ACTIVE:
			parser.ScanStr((char *)params, "%b", &_active);
			break;

		case TOKEN_BLOCKED:
			parser.ScanStr((char *)params, "%b", &_blocked);
			break;

		case TOKEN_DECORATION:
			parser.ScanStr((char *)params, "%b", &_decoration);
			break;

		case TOKEN_ZOOM:
		case TOKEN_SCALE: {
			int j;
			parser.ScanStr((char *)params, "%d", &j);
			_zoom = (float)j;
		}
		break;

		case TOKEN_POINT: {
			int x, y;
			parser.ScanStr((char *)params, "%d,%d", &x, &y);
			_points.Add(new CBPoint(x, y));
		}
		break;

		case TOKEN_ALPHA_COLOR:
			parser.ScanStr((char *)params, "%d,%d,%d", &ar, &ag, &ab);
			break;

		case TOKEN_ALPHA:
			parser.ScanStr((char *)params, "%d", &alpha);
			break;

		case TOKEN_EDITOR_SELECTED:
			parser.ScanStr((char *)params, "%b", &_editorSelected);
			break;

		case TOKEN_EDITOR_SELECTED_POINT:
			parser.ScanStr((char *)params, "%d", &_editorSelectedPoint);
			break;

		case TOKEN_SCRIPT:
			addScript((char *)params);
			break;

		case TOKEN_PROPERTY:
			parseProperty(params, false);
			break;

		case TOKEN_EDITOR_PROPERTY:
			parseEditorProperty(params, false);
			break;
		}
	}
	if (cmd == PARSERR_TOKENNOTFOUND) {
		Game->LOG(0, "Syntax error in REGION definition");
		return E_FAIL;
	}

	CreateRegion();

	_alpha = DRGBA(ar, ag, ab, alpha);

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
// high level scripting interface
//////////////////////////////////////////////////////////////////////////
HRESULT CAdRegion::scCallMethod(CScScript *script, CScStack *stack, CScStack *thisStack, const char *name) {
	/*
	    //////////////////////////////////////////////////////////////////////////
	    // SkipTo
	    //////////////////////////////////////////////////////////////////////////
	    if (strcmp(name, "SkipTo")==0) {
	        stack->CorrectParams(2);
	        _posX = stack->Pop()->GetInt();
	        _posY = stack->Pop()->GetInt();
	        stack->PushNULL();

	        return S_OK;
	    }

	    else*/ return CBRegion::scCallMethod(script, stack, thisStack, name);
}


//////////////////////////////////////////////////////////////////////////
CScValue *CAdRegion::scGetProperty(const char *name) {
	_scValue->SetNULL();

	//////////////////////////////////////////////////////////////////////////
	// Type
	//////////////////////////////////////////////////////////////////////////
	if (strcmp(name, "Type") == 0) {
		_scValue->SetString("ad region");
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// Name
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Name") == 0) {
		_scValue->SetString(_name);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// Blocked
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Blocked") == 0) {
		_scValue->SetBool(_blocked);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// Decoration
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Decoration") == 0) {
		_scValue->SetBool(_decoration);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// Scale
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Scale") == 0) {
		_scValue->SetFloat(_zoom);
		return _scValue;
	}

	//////////////////////////////////////////////////////////////////////////
	// AlphaColor
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AlphaColor") == 0) {
		_scValue->SetInt((int)_alpha);
		return _scValue;
	}

	else return CBRegion::scGetProperty(name);
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdRegion::scSetProperty(const char *name, CScValue *value) {
	//////////////////////////////////////////////////////////////////////////
	// Name
	//////////////////////////////////////////////////////////////////////////
	if (strcmp(name, "Name") == 0) {
		setName(value->GetString());
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// Blocked
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Blocked") == 0) {
		_blocked = value->GetBool();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// Decoration
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Decoration") == 0) {
		_decoration = value->GetBool();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// Scale
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "Scale") == 0) {
		_zoom = value->GetFloat();
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	// AlphaColor
	//////////////////////////////////////////////////////////////////////////
	else if (strcmp(name, "AlphaColor") == 0) {
		_alpha = (uint32)value->GetInt();
		return S_OK;
	}

	else return CBRegion::scSetProperty(name, value);
}


//////////////////////////////////////////////////////////////////////////
const char *CAdRegion::scToString() {
	return "[ad region]";
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdRegion::saveAsText(CBDynBuffer *Buffer, int Indent) {
	Buffer->putTextIndent(Indent, "REGION {\n");
	Buffer->putTextIndent(Indent + 2, "NAME=\"%s\"\n", _name);
	Buffer->putTextIndent(Indent + 2, "CAPTION=\"%s\"\n", getCaption());
	Buffer->putTextIndent(Indent + 2, "BLOCKED=%s\n", _blocked ? "TRUE" : "FALSE");
	Buffer->putTextIndent(Indent + 2, "DECORATION=%s\n", _decoration ? "TRUE" : "FALSE");
	Buffer->putTextIndent(Indent + 2, "ACTIVE=%s\n", _active ? "TRUE" : "FALSE");
	Buffer->putTextIndent(Indent + 2, "SCALE=%d\n", (int)_zoom);
	Buffer->putTextIndent(Indent + 2, "ALPHA_COLOR { %d,%d,%d }\n", D3DCOLGetR(_alpha), D3DCOLGetG(_alpha), D3DCOLGetB(_alpha));
	Buffer->putTextIndent(Indent + 2, "ALPHA = %d\n", D3DCOLGetA(_alpha));
	Buffer->putTextIndent(Indent + 2, "EDITOR_SELECTED=%s\n", _editorSelected ? "TRUE" : "FALSE");

	int i;
	for (i = 0; i < _scripts.GetSize(); i++) {
		Buffer->putTextIndent(Indent + 2, "SCRIPT=\"%s\"\n", _scripts[i]->_filename);
	}

	if (_scProp) _scProp->saveAsText(Buffer, Indent + 2);

	for (i = 0; i < _points.GetSize(); i++) {
		Buffer->putTextIndent(Indent + 2, "POINT {%d,%d}\n", _points[i]->x, _points[i]->y);
	}

	CBBase::saveAsText(Buffer, Indent + 2);

	Buffer->putTextIndent(Indent, "}\n\n");

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
HRESULT CAdRegion::persist(CBPersistMgr *persistMgr) {
	CBRegion::persist(persistMgr);

	persistMgr->transfer(TMEMBER(_alpha));
	persistMgr->transfer(TMEMBER(_blocked));
	persistMgr->transfer(TMEMBER(_decoration));
	persistMgr->transfer(TMEMBER(_zoom));

	return S_OK;
}

} // end of namespace WinterMute
