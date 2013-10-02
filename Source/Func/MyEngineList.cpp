/*
    Image Uploader - program for uploading images/files to Internet
    Copyright (C) 2007-2011 ZendeN <zenden2k@gmail.com>

    HomePage:    http://zenden.ws/imageuploader

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Func/MyEngineList.h"

#include "Func/MyUtils.h"
#include "Func/Common.h"
#include "Func/Settings.h"
#include "Core/Upload/DefaultUploadEngine.h"
#include "Core/Upload/ScriptUploadEngine.h"
#include "Gui/Dialogs/LogWindow.h"

CMyEngineList::CMyEngineList()
{
	m_prevUpEngine = 0;
}

CMyEngineList::~CMyEngineList()
{
	delete m_prevUpEngine;
	for ( std::map<std::string, HICON>::iterator it = serverIcons_.begin(); it != serverIcons_.end(); ++it) {
		DestroyIcon(it->second);
	}
}

CUploadEngineData* CMyEngineList::byName(const CString& name)
{
	return CUploadEngineList_Base::byName(WCstringToUtf8(name));
}

int CMyEngineList::GetUploadEngineIndex(const CString Name)
{
	return CUploadEngineList_Base::GetUploadEngineIndex(WCstringToUtf8(Name));
}

const CString CMyEngineList::ErrorStr()
{
	return m_ErrorStr;
}

CAbstractUploadEngine* CMyEngineList::getUploadEngine(CUploadEngineData* data)
{
	CAbstractUploadEngine* result = NULL;
	if (data->UsingPlugin)
	{
		result = iuPluginManager.getPlugin(data->PluginName, Settings.ServerByUtf8Name(data->Name));
	}
	else
	{
		if (m_prevUpEngine)
		{
			if (m_prevUpEngine->getUploadData()->Name == data->Name)
				result = m_prevUpEngine;
			else
			{
				delete m_prevUpEngine;
				m_prevUpEngine = 0;
			}
		}
		if (!m_prevUpEngine)
			m_prevUpEngine = new CDefaultUploadEngine();
		result = m_prevUpEngine;
	}
	if (!result)
		return 0;
	result->setUploadData(data);
	result->setServerSettings(Settings.ServerByUtf8Name(data->Name));
	result->onErrorMessage.bind(DefaultErrorHandling::ErrorMessage);
	return result;
}

CAbstractUploadEngine* CMyEngineList::getUploadEngine(std::string name)
{
	return getUploadEngine(CUploadEngineList_Base::byName(name));
}

CAbstractUploadEngine* CMyEngineList::getUploadEngine(int index)
{
	return getUploadEngine(CUploadEngineList_Base::byIndex(index));
}

bool CMyEngineList::LoadFromFile(const CString& filename)
{
	if (!IuCoreUtils::FileExists(WCstringToUtf8(filename)))
	{
		m_ErrorStr = "File not found.";
		return 0;
	}
	return CUploadEngineList::LoadFromFile(WCstringToUtf8(filename));
}

bool CMyEngineList::DestroyCachedEngine(const std::string& name)
{
	if (m_prevUpEngine == 0)
		return false;

	CUploadEngineData* ued = m_prevUpEngine->getUploadData();
	if (!ued)
		return false;
	if (ued->Name == name)
	{
		delete m_prevUpEngine;
		m_prevUpEngine = 0;
		return true;
	}
	return false;
}

HICON CMyEngineList::getIconForServer(const std::string& name) {
	std::map<std::string, HICON>::iterator iconIt = serverIcons_.find(name);
	if ( iconIt != serverIcons_.end() )
		return iconIt->second;

	HICON icon = (HICON)LoadImage(0,IU_GetDataFolder()+_T("Favicons\\")+Utf8ToWCstring(name)+_T(".ico"),IMAGE_ICON	,16,16,LR_LOADFROMFILE);
	if ( !icon ) {
		return 0;
	}
	serverIcons_[name] = icon;
	return icon;
}