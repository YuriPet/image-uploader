/*

    Image Uploader -  free application for uploading images/files to the Internet

    Copyright 2007-2018 Sergey Svistunov (zenden2k@yandex.ru)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

 */
#ifndef SEARCHBYIMAGEDLG_H
#define SEARCHBYIMAGEDLG_H

#pragma once
#include "atlheaders.h"
#include "resource.h"       // main symbols
#include <Gui/Controls/PictureExWnd.h>


// CSearchByImageDlg
class SearchByImage;

class CSearchByImageDlg :
    public CDialogImpl<CSearchByImageDlg>
{
    public:
        explicit CSearchByImageDlg(CString fileName);
        ~CSearchByImageDlg();
        enum { IDD = IDD_SEARCHBYIMAGEDLG};

        BEGIN_MSG_MAP(CSearchByImageDlg)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
        END_MSG_MAP()
        // Handler prototypes:
        //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        bool IsRunning();
private:
    CString fileName_;
    std::unique_ptr<SearchByImage> seeker_;
    bool cancelPressed_;
    CPictureExWnd wndAnimation_;
    void onSeekerFinished(bool success, const std::string& msg);
};

#endif // STATUSDLG_H
