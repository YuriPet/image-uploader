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

#ifndef MEDIAINFODLG_H
#define MEDIAINFODLG_H

// MediaInfoDlg.h : Declaration of the CMediaInfoDlg
// 
// This dialog window shows technical information 
// about  video/audio file that user had selected


#pragma once
#include "atlheaders.h"
#include "maindlg.h"
#include "resource.h"       

// CMediaInfoDlg

class CMediaInfoDlg:        public CDialogImpl <CMediaInfoDlg>,
                                public CDialogResize <CMediaInfoDlg>,
                                public CThreadImpl <CMediaInfoDlg>
{
    public:
        enum InfoType { itSummary = 0, itFullInformation };

        CMediaInfoDlg(InfoType type = itSummary);
        ~CMediaInfoDlg();
        void ShowInfo(LPCTSTR FileName);

        enum { IDD = IDD_MEDIAINFODLG };
      
    protected:
        BEGIN_MSG_MAP(CMediaInfoDlg)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
            COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
            COMMAND_HANDLER(IDC_COPYALL, BN_CLICKED, OnBnClickedCopyall)
            CHAIN_MSG_MAP(CDialogResize<CMediaInfoDlg>)
        END_MSG_MAP()

        BEGIN_DLGRESIZE_MAP(CMediaInfoDlg)
            DLGRESIZE_CONTROL(IDC_FILEINFOEDIT, DLSZ_SIZE_X|DLSZ_SIZE_Y)
            DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X|DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_COPYALL, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_SUMMARYRADIOBUTTON, DLSZ_MOVE_Y)
            DLGRESIZE_CONTROL(IDC_FULLINFORADIOBUTTON, DLSZ_MOVE_Y)
        END_DLGRESIZE_MAP()
        
        // Handler prototypes:
        //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
        LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnBnClickedCopyall(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        DWORD Run();

        CString m_FileName;
        CFont editFont_;
        InfoType infoType_;
        CString summary_, fullInfo_;
};




#endif // MEDIAINFODLG_H