/*

    Image Uploader -  free application for uploading images/files to the Internet

    Copyright 2007-2018 Sergey Svistunov (zenden2k@gmail.com)

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

#include "ThumbsView.h"

#include "Func/MyDropSource.h"
#include "Func/MyDataObject.h"
#include "Func/Common.h"
#include "Gui/Dialogs/LogWindow.h"
#include "Core/i18n/Translator.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Core/ServiceLocator.h"
#include "Core/Images/Utils.h"
#include "Func/IuCommonFunctions.h"
#include "Gui/CommonDefines.h"
#include "Gui/Dialogs/MainDlg.h"

#define THUMBNAIL_WIDTH 170   // constants
#define THUMBNAIL_HEIGHT 120

// CThumbsView
CThumbsView::CThumbsView() :deletePhysicalFiles_(false)
{
    maxwidth = 0;
    maxheight = 0;
    callbackLastCallTime_ = 0;
    ExtendedView = false;
}

CThumbsView::~CThumbsView()
{
    ImageList.Destroy();
    //ImageView.DestroyWindow();
    StopBackgroundThread(true);
}

void CThumbsView::Init(bool Extended)
{
    Start(THREAD_PRIORITY_BELOW_NORMAL);
    ExtendedView = Extended;
    ImageView.Create(m_hWnd);
    DWORD rtlStyle = ServiceLocator::instance()->translator()->isRTL() ? ILC_MIRROR | ILC_PERITEMMIRROR : 0;
    ImageList.Create(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT + (ExtendedView ? 20 : 0), ILC_COLOR24 | rtlStyle, 0, 3);
    SetImageList(ImageList, LVSIL_NORMAL);
    DWORD style = GetExtendedListViewStyle();
    style = style | LVS_EX_DOUBLEBUFFER | LVS_EX_BORDERSELECT;
    SetExtendedListViewStyle(style);
    SetIconSpacing(THUMBNAIL_WIDTH + 5, THUMBNAIL_HEIGHT + 25 + (ExtendedView ? 20 : 0));
}

int CThumbsView::AddImage(LPCTSTR FileName, LPCTSTR Title, bool ensureVisible, Gdiplus::Image* Img)
{
    if (!FileName) {
        return -1;
    }
    
    int n = GetItemCount();

    RECT rc;
    GetClientRect(&rc);

    if(ImageList.GetImageCount() < 1)
        LoadThumbnail(-1, nullptr, nullptr);

    AddItem(n, 0, Title, 0);

    ThumbsViewItem * TVI = new ThumbsViewItem;

    TVI->ThumbOutDate = FALSE;
    TVI->FileName = FileName;
    //TVI->Image = nullptr;
    SetItemData(n, reinterpret_cast<DWORD_PTR>(TVI));

    if (Img) {
        LoadThumbnail(n, TVI, Img);
    }

    Arrange(LVA_ALIGNTOP);
     
    if (ensureVisible) {
        EnsureVisible(n, FALSE);
    }

    RedrawItems(n, n);
    return n;
}

bool CThumbsView::MyDeleteItem(int ItemIndex)
{
    if( ItemIndex < 0 || ItemIndex > GetItemCount()-1) return false;

    SimpleDelete(ItemIndex, true, deletePhysicalFiles_);
    DeleteItem(ItemIndex);   

    Arrange(LVA_ALIGNTOP);

    EnsureVisible(ItemIndex,true);

    return true;
}

LRESULT CThumbsView::OnMButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    POINT p = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

    int ItemIndex = HitTest(p, 0); //Getting the index of item was clicked (by middle button)
    if(ItemIndex < 0) return 0;

    if(GetItemState(ItemIndex, LVIS_SELECTED) != LVIS_SELECTED)
    {
        MyDeleteItem(ItemIndex);
        NotifyItemCountChanged();
    }
    else if(GetNextItem(-1,LVNI_SELECTED)>=0)
    {
        DeleteSelected();
    }

    bHandled = true;
    return 0;
}

int CThumbsView::DeleteSelected(void)
{
    if(GetItemCount() < 1) return 0;

    SetRedraw(false);
    int nItem=0;
    do
    {
        nItem = GetNextItem(nItem-1,LVNI_SELECTED    );
        if(nItem == -1) break;
        SimpleDelete(nItem, true, deletePhysicalFiles_);
        DeleteItem(nItem);
    }
    while(nItem!=-1);

    SetRedraw(true);
    Arrange(LVA_ALIGNTOP);
    NotifyItemCountChanged();
    return 0;
}

void CThumbsView::UpdateImageIndexes(int StartIndex)
{
    if(StartIndex==-1) return;
    int n = GetItemCount();
    for (int i=StartIndex;i<n;i++)
    {
        if(GetImageIndex(i)>0)
            SetItem(i, 0, LVIF_IMAGE, 0,i+1, 0, 0, 0);
    }
}

void CThumbsView::MyDeleteAllItems()
{
    int n = GetItemCount();
    for (int i = 0; i < n; i++) {
        SimpleDelete(i, false);
    }
    DeleteAllItems();
    clearImageList();
}

bool CThumbsView::SimpleDelete(int ItemIndex, bool DeleteThumb, bool deleteFile)
{
    auto *TVI = reinterpret_cast<ThumbsViewItem *>(GetItemData(ItemIndex));

    if (deletePhysicalFiles_ && deleteFile && !TVI->FileName.IsEmpty()) {
        DeleteFile(TVI->FileName); // delete file from disk (enabled only on videograbber page)
    }

    SetItemData(ItemIndex, 0);
    delete TVI;

    return true;
}

LPCTSTR CThumbsView::GetFileName(int ItemIndex)
{
    auto *TVI = reinterpret_cast<ThumbsViewItem *>(GetItemData(ItemIndex));
    if(!TVI) {
        return _T("");
    } 
    return TVI->FileName;
}

LRESULT CThumbsView::OnKeyDown(TCHAR vk, UINT cRepeat, UINT flags)
{
    if(vk == VK_DELETE)
        DeleteSelected();
    else if(vk == _T('A'))
    {
        if(GetKeyState(VK_CONTROL)) //Check if Ctrl key is hold
        {
            SetRedraw(FALSE); // Turn off control redraw
            int n = GetItemCount();
            for (int i=0; i<n; i++)
            {
                SetItemState(i, LVIS_SELECTED, 2);
            }
            SetRedraw(TRUE);
        }
    } else if ( vk == _T('C') && GetKeyState(VK_CONTROL) ) {
        CopySelectedItemsToClipboard();
    } else {
        SetMsgHandled(FALSE);
    }
    return 0;
}

bool CThumbsView::LoadThumbnail(int ItemID, ThumbsViewItem* tvi, Gdiplus::Image *Img)
{
    using namespace Gdiplus;
    if(ItemID>GetItemCount()-1) 
    {
        return false;
    }
    std::unique_ptr<Bitmap> ImgBuffer;
    std::unique_ptr<Image> bm;
    CString filename;
    if(ItemID>=0) 
    {
        filename = /*GetFileName(ItemID);*/tvi->FileName; 
    }
    int width, height, imgwidth = 0, imgheight = 0, newwidth=0, newheight=0;
    width = THUMBNAIL_WIDTH/*rc.right-2*/;
    height = THUMBNAIL_HEIGHT/*rc.bottom-16*/;
    int thumbwidth = THUMBNAIL_WIDTH;
    int  thumbheight = THUMBNAIL_HEIGHT;
    bool isImage = Img || IuCommonFunctions::IsImage(filename);
    if ( isImage)
    {
        if (Img) {
            //bm = Img;

            imgwidth = Img->GetWidth();
            imgheight = Img->GetHeight();
            bm = ImageUtils::GetThumbnail(Img, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, 0);
            if (bm) {
                newwidth = bm->GetWidth();
                newheight = bm->GetHeight();
            }
        }
           
        else 
            if(ItemID>=0) 
            {
                Gdiplus::Size originalImageSize;
                bm = ImageUtils::GetThumbnail(filename, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, &originalImageSize);
                if (bm) {
                    imgwidth = originalImageSize.Width;
                    imgheight = originalImageSize.Height;
                    newwidth = bm->GetWidth();
                    newheight = bm->GetHeight();
                }
          
            }
    }
    if (imgwidth>maxwidth) maxwidth = imgwidth;
    if (imgheight>maxheight) maxheight = imgheight;
    Graphics g(m_hWnd,true);
    ImgBuffer = std::make_unique<Bitmap>(thumbwidth, thumbheight+30, &g);


    Graphics gr(ImgBuffer.get());
    gr.SetInterpolationMode(InterpolationModeHighQualityBicubic );
    gr.Clear(Color(255,255,255,255));

    RectF bounds(1, 1, float(width), float(height));

    if ( (isImage &&  ItemID >= 0 ) && (!bm || !bm->GetWidth() ) ) {
        LinearGradientBrush 
            brush(bounds, Color(130, 255, 0, 0), Color(255, 0, 0, 0), 
            LinearGradientModeBackwardDiagonal); 

        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        Font font(L"Arial", 12, FontStyleBold);
        ServiceLocator::instance()->logger()->write(ILogger::logWarning, TR("List of Images"), TR("Cannot load thumbnail for image."), CString(TR("File:")) + _T(" ") + filename);
        gr.DrawString(TR("Unable to load picture"), -1, &font, bounds, &format, &brush);
    }

    // 

    else {
        LinearGradientBrush 
            br(bounds, Color(255, 255, 255, 255), Color(255, 210, 210, 210), 
            LinearGradientModeBackwardDiagonal/* LinearGradientModeVertical*/); 

        if(IuCommonFunctions::IsImage(filename))
            gr.FillRectangle(&br,1, 1, width-1,height-1);
        gr.SetInterpolationMode(InterpolationModeHighQualityBicubic );

        if(ItemID>=0 && !Img && !IuCommonFunctions::IsImage(filename))
        {


            WORD id;
            HICON ggg =//GetAssociatedIcon (filename,false);
                ExtractAssociatedIcon(
                GetModuleHandle(0),
                (LPWSTR)(LPCTSTR)filename,
                &id);
            if(ggg)
            {
                GuiTools::IconInfo ii = GuiTools::GetIconInfo(ggg);
                int iconWidth = ii.nWidth;
                int iconHeight = ii.nHeight;
                if ( iconWidth ) {
                    HDC dc=GetDC();
                    
                    
                    HDC memDC = CreateCompatibleDC(dc);
                    HBITMAP memBm = CreateCompatibleBitmap(dc,iconWidth,iconHeight);
                    SelectObject(memDC, memBm);
                    RECT r={0,0,iconWidth,iconHeight};
                    FillRect(memDC,    // handle to device context 
                        &r,    // pointer to structure with rectangle  
                        GetSysColorBrush(COLOR_WINDOW)    // handle to brush 
                        );


                    DrawIcon(memDC, 0,0,ggg);
                    Bitmap *bitmap=Bitmap::FromHBITMAP(memBm,0);
                    ;
                    gr.DrawImage(/*backBuffer*/bitmap, (int)((width-bitmap->GetWidth())/2)+1, (int)((height-bitmap->GetHeight())/2), (int)bitmap->GetWidth(),(int)bitmap->GetHeight());
                    delete bitmap;
                    
                    DeleteObject(memDC);
                    DeleteObject(memBm);
                    ReleaseDC(dc);
                }
                DestroyIcon(ggg);
            }
        }

        try{
            if(bm)
                gr.DrawImage(/*backBuffer*/bm.get(), (int)((width-newwidth)/2)+1, (int)((height-newheight)/2), (int)newwidth,(int)newheight);
        }
        catch(...)
        {
        }

        RectF bounds(0, float(height), float(width), float(20));

        if (ExtendedView)
        {
            LinearGradientBrush 
                br2(bounds, Color(240, 255, 255, 255), Color(200, 210, 210, 210), 
                LinearGradientModeBackwardDiagonal /*LinearGradientModeVertical*/); 
            gr.FillRectangle(&br2,(float)1, (float)height+1, (float)width-2, (float)height+20-1);
        }

        if(ItemID>=0)
        {
            SolidBrush brush(Color(255,0,0,0));
            StringFormat format;
            format.SetAlignment(StringAlignmentCenter);
            format.SetLineAlignment(StringAlignmentCenter);
            Font font(L"Tahoma", 8, FontStyleRegular );
            CString Filename = /*GetFileName(ItemID);*/filename;
            CString Buffer;
            //int f = MyGetFileSize(GetFileName(ItemID));
            WCHAR buf2[25];
            std::string fileSizeStr = IuCoreUtils::FileSizeToString(IuCoreUtils::GetFileSize(WCstringToUtf8(Filename)));
            lstrcpy(buf2, Utf8ToWCstring(fileSizeStr));
            //NewBytesToString(f, buf2, 25);
            WCHAR FileExt[25];
            lstrcpy(FileExt, WinUtils::GetFileExt(Filename));
            if(!lstrcmpi(FileExt, _T("jpg"))) 
                lstrcpy(FileExt,_T("JPEG"));
            if(IuCommonFunctions::IsImage(filename) && bm) {
                Buffer.Format(_T("%s %dx%d (%s)"), (LPCTSTR)FileExt, imgwidth, imgheight, (LPCTSTR)buf2);
            }
            else {
                Buffer = buf2;
            }
            gr.DrawString(Buffer, -1, &font, bounds, &format, &brush);
        }
    }

    HBITMAP bmp = nullptr;
    ImgBuffer->GetHBITMAP(Color(255,255,255), &bmp);

    if (tvi) {
        tvi->ThumbLoaded = true;
        tvi->ThumbnailRequested = false;
        
        int oldImageIndex = GetImageIndex(ItemID);
        if (oldImageIndex != 0) {
            ImageList.Replace(oldImageIndex, bmp, nullptr);
            RedrawItems(ItemID, ItemID);
           // SetItem(ItemID, 0, LVIF_IMAGE, 0, oldImageIndex, 0, 0, 0);
        } else {
            int imageIndex = ImageList.Add(bmp, (COLORREF)0);
            SetItem(ItemID, 0, LVIF_IMAGE, 0, imageIndex, 0, 0, 0);
        }

    } else {
        defaultImage_ = bmp;
        ImageList.Add(bmp, (COLORREF)0);
    }

    return true;
}

int CThumbsView::GetImageIndex(int ItemIndex) const
{
    LV_ITEM item;

    item.iItem = ItemIndex;
    item.iSubItem = 0;
    item.mask = LVIF_IMAGE;
    GetItem(&item);
    return item.iImage;
}

LRESULT CThumbsView::OnLButtonDblClk(UINT Flags, CPoint Pt)
{
    ViewSelectedImage();
    return 0;
}

// The Thread which loads Thumbnails in ImageList
DWORD CThumbsView::Run()
{
    while (true) {
        int itemIndex;
        {
            std::unique_lock<std::mutex> lck(thumbQueueMutex_);
            thumbQueueCondition_.wait(lck, [&] {return ShouldStop() || !thumbQueue_.empty(); });

            if (ShouldStop()) {
                break; // Exiting thread
            }

            itemIndex = thumbQueue_.front();
            thumbQueue_.pop_front();
        }
        ThumbsViewItem* item = reinterpret_cast<ThumbsViewItem*>(GetItemData(itemIndex));
        if (!item) {
            continue;
        }
        LoadThumbnail(itemIndex, item, nullptr);
    }
    return 0;
}

void CThumbsView::ViewSelectedImage()
{
    int nCurItem;
    if ((nCurItem = GetNextItem(-1, LVNI_ALL|LVNI_SELECTED)) < 0) return;

    LPCTSTR FileName = GetFileName(nCurItem);

    if(!FileName || !IuCommonFunctions::IsImage(FileName)) return;
    CImageViewItem imgViewItem;
    imgViewItem.index = nCurItem;
    imgViewItem.fileName = FileName;
    HWND wizardDlg = ::GetParent(GetParent());
    ImageView.ViewImage(imgViewItem, wizardDlg);
    ImageView.setCallback(this);
}

LRESULT CThumbsView::OnLvnBeginDrag(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    if(GetItemCount() < 1) return 0;

    //NM_LISTVIEW *pnmv = (NM_LISTVIEW FAR *) pnmh;  
    DWORD dwEffect = 0;

    CMyDropSource* pDropSource = new CMyDropSource();
    CMyDataObject* pDataObject = new CMyDataObject();

    int nItem = -1;
    do {
        nItem = GetNextItem(nItem, LVNI_SELECTED);
        if (nItem == -1) break;
        pDataObject->AddFile(GetFileName(nItem));
    }
    while (true);

    //Disable drag-n-drop to parent window
    SendMessage(GetParent(), MYWM_ENABLEDROPTARGET, 0, 0);

    /*DWORD dwResult = */
    ::DoDragDrop(pDataObject, pDropSource, DROPEFFECT_COPY, &dwEffect);
    pDropSource->Release();
    pDataObject->Release();

    SendMessage(GetParent(), MYWM_ENABLEDROPTARGET, 1, 0);
    return 0;
}

LRESULT CThumbsView::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int count = GetItemCount();
    for (int i = 0; i < count; i++) {
        auto *tvi = reinterpret_cast<ThumbsViewItem *>(GetItemData(i));
        delete tvi;
    }
    return 0;
}

void CThumbsView::OutDateThumb(int nIndex)
{
    auto *TVI = reinterpret_cast<ThumbsViewItem *>(GetItemData(nIndex));
    if (!TVI) {
        return;
    }
    TVI->ThumbOutDate = TRUE;
    TVI->ThumbLoaded = false;
    getThumbnail(nIndex);
}

bool CThumbsView::StopBackgroundThread(bool wait)
{
    bool isRunning = IsRunning();
    if (isRunning) {
        {
            std::lock_guard<std::mutex> lk(thumbQueueMutex_);
            thumbQueue_.clear();
        }
        SignalStop();
        thumbQueueCondition_.notify_one();
        if (wait) {
            MsgWaitForSingleObject(m_hThread, INFINITE);
        }
    }
    return isRunning;
}

void CThumbsView::SelectLastItem()
{
    int nItem = -1;
    do {
        nItem = GetNextItem(nItem, LVNI_SELECTED);
        if (nItem == -1) break;
        SetItemState(nItem, 0, LVIS_SELECTED);
    } while (nItem != -1);
    SetItemState(GetItemCount() - 1, LVIS_SELECTED, LVIS_SELECTED);
    EnsureVisible(GetItemCount() - 1, FALSE);

}

LRESULT CThumbsView::OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    LPNMLISTVIEW pNotificationInfo = reinterpret_cast<LPNMLISTVIEW>(pnmh);
    if (pNotificationInfo->iItem == -1) return FALSE;
    auto* TVI = reinterpret_cast<ThumbsViewItem*>(pNotificationInfo->lParam);
    delete TVI;
    return 0;
}

void CThumbsView::SetDeletePhysicalFiles(bool doDelete) {
    deletePhysicalFiles_ = doDelete;
}

CImageViewItem CThumbsView::getNextImgViewItem(const CImageViewItem& currentItem) {
    CImageViewItem result;
    result.index = -1;

    int index; 
    for (index = currentItem.index + 1; index < GetItemCount(); index++) {
        LPCTSTR FileName = GetFileName(index);
        if (FileName && IuCommonFunctions::IsImage(FileName)){
            result.index = index;
            result.fileName = FileName;
            return result;
        }
    }

    for (index = 0; index <= currentItem.index && index < GetItemCount(); index++) {
        LPCTSTR FileName = GetFileName(index);
        if (FileName && IuCommonFunctions::IsImage(FileName)) {
            result.index = index;
            result.fileName = FileName;
            return result;
        }
    }
    return result;
}

CImageViewItem CThumbsView::getPrevImgViewItem(const CImageViewItem& currentItem) {
    CImageViewItem result;
    result.index = -1;

    int index;
    for (index = currentItem.index - 1; index >=0; index--) {
        LPCTSTR FileName = GetFileName(index);
        if (FileName && IuCommonFunctions::IsImage(FileName)) {
            result.index = index;
            result.fileName = FileName;
            return result;
        }
    }

    for (index = GetItemCount() - 1; index >= currentItem.index && index >= 0; index--) {
        LPCTSTR FileName = GetFileName(index);
        if (FileName && IuCommonFunctions::IsImage(FileName)) {
            result.index = index;
            result.fileName = FileName;
            return result;
        }
    }

    return result;
}

void CThumbsView::NotifyItemCountChanged(bool selected) {  
    if (callback_) {
        callback_(this, selected);
    }
}

bool CThumbsView::CopySelectedItemsToClipboard() const {
    return ::SendMessage(GetParent(), WM_COMMAND, MAKELPARAM(CMainDlg::MENUITEM_COPYFILETOCLIPBOARD, 0), 0) != FALSE;
}

LRESULT CThumbsView::OnItemChanged(int, LPNMHDR hdr, BOOL&) {
    NMLISTVIEW* lpStateChange = reinterpret_cast<NMLISTVIEW*>(hdr);
    if ((lpStateChange->uNewState ^ lpStateChange->uOldState) & LVIS_SELECTED) {
        NotifyItemCountChanged((lpStateChange->uNewState & LVIS_SELECTED) == LVIS_SELECTED);
    }
    return 0;
}

void CThumbsView::SetOnItemCountChanged(ItemCountChangedCallback&& callback) {  
    callback_ = std::move(callback);
}

LRESULT CThumbsView::OnCustomDraw(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pnmh);
    if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage) {

        return CDRF_NOTIFYITEMDRAW;
    } else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage) {
        // This is the pre-paint stage for an item.  We need to make another
        // request to be notified during the post-paint stage.
        // If this item is selected, re-draw the icon in its normal
        // color (not blended with the highlight color).

        LVITEM rItem;
        int    nItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);

        // Get the image index and state of this item.  Note that we need to
        // check the selected state manually.  The docs _say_ that the
        // item's state is in pLVCD->nmcd.uItemState, but during my testing
        // it was always equal to 0x0201, which doesn't make sense, since
        // the max CDIS_* constant in commctrl.h is 0x0100.

        ZeroMemory(&rItem, sizeof(LVITEM));
        rItem.mask = LVIF_IMAGE /*| LVIF_STATE*/;
        rItem.iItem = nItem;
        //rItem.stateMask = 0;
        GetItem(&rItem);

        // If this item is selected, redraw the icon with its normal colors.
        if (/*rItem.state & LVIS_SELECTED*/ true) {
            /*CDC*  pDC = CDC::FromHandle(pLVCD->nmcd.hdc);*/
            CRect rcIcon;

            // Get the rect that holds the item's icon.
            GetItemRect(nItem, &rcIcon, LVIR_ICON);

            CRect clientRect;
            GetClientRect(clientRect);

            rcIcon.IntersectRect(rcIcon, clientRect);

            if (!rcIcon.IsRectEmpty()/*true*/) {
                /*HBITMAP bm = */getThumbnail(nItem);
                /*if (bm) {
                CDC compDc;
                compDc.CreateCompatibleDC(pLVCD->nmcd.hdc);

                HBITMAP oldBm = compDc.SelectBitmap(bm);
                CRect thumbnailRect(rcIcon.left, rcIcon.top, rcIcon.left + std::min<>(THUMBNAIL_WIDTH, rcIcon.Width()), rcIcon.top + std::min<>(THUMBNAIL_HEIGHT + (ExtendedView ? 20 : 0), rcIcon.Height()));
                //thumbnailRect.OffsetRect();
                thumbnailRect = ImageUtils::CenterRect(thumbnailRect, rcIcon);

                BitBlt(pLVCD->nmcd.hdc, thumbnailRect.left, thumbnailRect.top, thumbnailRect.Width(), thumbnailRect.Height(), compDc, 0, 0, SRCCOPY);
                compDc.SelectBitmap(oldBm);
                }*/

                return CDRF_DODEFAULT;/*CDRF_SKIPDEFAULT*/
            }
        }
        return/* CDRF_NOTIFYPOSTPAINT*/CDRF_DODEFAULT;
    } else if (CDDS_ITEMPOSTPAINT == pLVCD->nmcd.dwDrawStage) {
        return CDRF_DODEFAULT;
    }

    return CDRF_DODEFAULT;
}

void CThumbsView::getThumbnail(int itemIndex) {
    auto* tvi = reinterpret_cast<ThumbsViewItem *>(GetItemData(itemIndex));
    if (!tvi || tvi->ThumbLoaded || tvi->ThumbnailRequested) {
        return;
    }

    tvi->ThumbnailRequested = true;
    {
        std::lock_guard<std::mutex> lk(thumbQueueMutex_);
        //tvi->Index = itemIndex;
        thumbQueue_.push_front(itemIndex);
    }
    //LOG(ERROR) << "thumbnail requested:" << std::endl << tvi->FileName;
    thumbQueueCondition_.notify_one();
}

void CThumbsView::clearImageList() {
    // Default thumbnail (index=0) will be regenerated in AddImage()
    ImageList.RemoveAll();
}