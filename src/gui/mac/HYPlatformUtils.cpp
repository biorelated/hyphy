#include "HYUtils.h"
#include "Quickdraw.h"
#include "hy_strings.h"
#include "Icons.h"
#include "Fonts.h"
#include "Dialogs.h"
#include "ColorPicker.h"
#include "HYWindow.h"
#include "errorfns.h"
#include "calcnode.h"
#include "ToolUtils.h"
#include "Navigation.h"
#include "batchlan.h"
#include "Lists.h"
#include "math.h"
#include "string.h"
#include "likefunc.h"
#include "Appearance.h"
#include "HYDialogs.h"
#include "time.h"
#include <URLAccess.h>
#include <Scrap.h>
#include "HYConsoleWindow.h"

#ifndef  TARGET_API_MAC_CARBON
#include "StandardFile.h"
#endif


void    GetFullPathName (FSSpec& theReply, _String& feedback);
void    ListToPopUpMenu (_List& menuOptions, MenuHandle listMenu);
void    SetStatusBarValue (long, _Parameter, _Parameter);

#define LIST_PUTFILE_BOX  200
#define LIST_PUTFILE_MENU 1112
#define GENERIC_POPUP_ID  1123

long    putFileMenuChoice;

extern  bool                updateTimer;

extern  time_t              timerStart,
        lastTimer;


extern  PixPatHandle        whiteFill;

extern  CIconHandle         pullDownArrowsIcon;

RGBColor
blackMenuText = {0,0,0},
grayMenuText  = {0x7fff,0x7fff,0x7fff},
menuLine1     = {0xA000,0xA000,0xA000},
menuLine2   = {0x0400,0x0400,0x0400};


extern _SimpleList windowObjects, windowPtrs;

pascal void     HYSavePopEventProc      (NavEventCallbackMessage ,NavCBRecPtr ,void * );
pascal Boolean  customModalProc         (DialogPtr , EventRecord *, short* );
void            StringToStr63           (_String& , Str63& );
pascal void     PopUpThemeDrawProc      (const Rect *,ThemeButtonKind ,const ThemeButtonDrawInfo *,UInt32 ,SInt16 ,Boolean );
MenuHandle      BuildMenuFromList       (_List& , long , bool );
OSStatus        DownloadPB              (void * , EventRecord * );

extern  _String   VerbosityLevelString;

//________________________________________________________
pascal Boolean  customModalProc (DialogPtr theDialog, EventRecord *theEvent, short* itemHit)
{
    if ((theEvent->what == activateEvt)||(theEvent->what == updateEvt)) {
        if (theEvent->message != (long)theDialog) {
            long k = windowPtrs.Find((long)theEvent->message);
            if (k>=0) {
                _HYPlatformWindow* clickedWindow = (_HYPlatformWindow*)windowObjects (k);
                clickedWindow->_ProcessOSEvent ((Ptr)theEvent);
            }
        }
    } else if ((theEvent->what == keyDown)||(theEvent->what == autoKey)) {
        unsigned char keyCode = (theEvent->message&keyCodeMask)>>8;
        if ((keyCode == 0x4C)||(keyCode == 0x24)) { // return
            *itemHit = kStdOkItemIndex;
            return true;
        }
        if ((keyCode==0x35)||((theEvent->modifiers & cmdKey)&&(keyCode==0x2F))) { // cancel
            *itemHit = kStdCancelItemIndex;
            return true;
        }
        if ((keyCode==0x2D)&&(theEvent->modifiers & cmdKey)) {
            *itemHit = 3;
            return true;
        }
    }
    return FALSE;
}

//________________________________________________________

#ifdef TARGET_API_MAC_CARBON
ModalFilterUPP   myFilterProc = NewModalFilterUPP((ModalFilterProcPtr)customModalProc);
#else
UniversalProcPtr myFilterProc = NewModalFilterUPP((ModalFilterProcPtr)customModalProc);
#endif

//________________________________________________________
void    ToggleAnalysisMenu (bool running)
{
    MenuHandle anMenu = GetMenuHandle (131);
    if (running) {
        EnableMenuItem (anMenu,1);
        EnableMenuItem (anMenu,2);
        DisableMenuItem (anMenu,6);
    } else {
        DisableMenuItem (anMenu,1);
        DisableMenuItem (anMenu,2);
        EnableMenuItem (anMenu,6);
        SetStatusBarValue (-1,1,0);
        SetStatusLine ("Idle");
    }
}

//________________________________________________________
Ptr     ProcureIconResource (long iconID)
{
    return (Ptr)GetCIcon (iconID);
}

//________________________________________________________
long    GetVisibleStringWidth (_String& s, _HYFont& f)
{
    static _String lastFont;
    static short   fontID = -1;
    if (f.face!=lastFont) {
        Str255 fName;
        StringToStr255 (f.face,fName);
        GetFNum (fName,&fontID);
        lastFont = f.face;
    }

    GrafPtr thisPort;
    GetPort (&thisPort);
#ifdef OPAQUE_TOOLBOX_STRUCTS
    short   savedFace = GetPortTextFont (thisPort),
            savedSize = GetPortTextSize (thisPort);

    Style   savedStyle = GetPortTextFace (thisPort);
#else
    short   savedFace = thisPort->txFont,
            savedSize = thisPort->txSize;

    Style   savedStyle = thisPort->txFace;
#endif
    TextFont (fontID);
    TextSize (f.size);
    TextFace (f.style);
    long res = TextWidth (s.sData,0,s.sLength);
    TextFont (savedFace);
    TextSize (savedSize);
    TextFace (savedStyle);
    return res;
}

//________________________________________________________
long    GetVisibleStringWidth (_String& s)
{
    long res = TextWidth (s.sData,0,s.sLength);
    return res;
}

//________________________________________________________
long    GetMaxCharWidth (_HYFont& f)
{
    static _String lastFont;
    static short   fontID = -1;
    if (f.face!=lastFont) {
        Str255 fName;
        StringToStr255 (f.face,fName);
        GetFNum (fName,&fontID);
        lastFont = f.face;
    }
    GrafPtr thisPort;
    GetPort (&thisPort);
#ifdef OPAQUE_TOOLBOX_STRUCTS
    short   savedFace = GetPortTextFont (thisPort),
            savedSize = GetPortTextSize (thisPort);

    Style   savedStyle = GetPortTextFace (thisPort);
#else
    short   savedFace = thisPort->txFont,
            savedSize = thisPort->txSize;

    Style   savedStyle = thisPort->txFace;
#endif
    TextFont (fontID);
    TextSize (f.size);
    TextFace (f.style);
    //FontInfo fi;
    //GetFontInfo (&fi);
    //long res = fi.widMax;
    long res = CharWidth ('W');
    //printf ("\n%d\n",CharWidth('W'));
    TextFont (savedFace);
    TextSize (savedSize);
    TextFace (savedStyle);
    return res;
}

//________________________________________________________
/*pascal short  saveDialogProc (short item, DialogPtr theDialog, void* theData)
{
    static Rect putFilePullDownMenu;
    _List*  menuItems = (_List*)theData;
    if (item!=sfHookFirstCall)
    {
        if (putFileMenuChoice<0)
        {
            putFileMenuChoice = 0;
            DrawMenuPlaceHolder (putFilePullDownMenu,*(_String*)(*menuItems)(putFileMenuChoice));
        }
    }
    else
    {
        short   dummy;
        Handle  dummy2;
        GetDialogItem (theDialog,14,&dummy,&dummy2,&putFilePullDownMenu);
    }
    switch (item)
    {
        case 14:
        {
            Point theCoord;
            theCoord.v = putFilePullDownMenu.top;
            theCoord.h = putFilePullDownMenu.left;
            LocalToGlobal (&theCoord);
            long newSel = PopUpMenuSelect (GetMenuHandle(LIST_PUTFILE_MENU),
                                           theCoord.v,
                                           theCoord.h,
                                           putFileMenuChoice+1);
            if (newSel&0xffff0000)
            {
                putFileMenuChoice = (newSel&0x0000ffff)-1;
                DrawMenuPlaceHolder (putFilePullDownMenu,*(_String*)(*menuItems)(putFileMenuChoice));
            }
        }

    }
    return item;
}*/

//________________________________________________________

pascal void HYSavePopEventProc (NavEventCallbackMessage callBackSelector,
                                NavCBRecPtr callBackParms,
                                void *callBackUD )
{
    if (callBackSelector==kNavCBPopupMenuSelect) {
        NavMenuItemSpec* menuItem = (NavMenuItemSpec*)callBackParms->eventData.eventDataParms.param;
        *((long*)callBackUD) = menuItem->menuType;
    } else if (callBackSelector==kNavCBEvent) {
        EventRecord* theEvent = callBackParms->eventData.eventDataParms.event;
        if ((theEvent->what == activateEvt)||(theEvent->what == updateEvt)) {
            if (theEvent->message != (long)callBackParms->window) {
                long k = windowPtrs.Find((long)theEvent->message);
                if (k>=0) {
                    _HYPlatformWindow* clickedWindow = (_HYPlatformWindow*)windowObjects (k);
                    clickedWindow->_ProcessOSEvent ((Ptr)theEvent);
                }
                //else
                //  SIOUXHandleOneEvent (theEvent);
            }
        }
    }
}

extern      Str255   hpName;
extern      _String  menuSeparator;

//________________________________________________________

long    SaveFileWithPopUp (_String& fileName, _String& prompt, _String& defFileName,
                           _String& listLabel, _List& menuOptions)
{
    OSErr               navErr;
    NavReplyRecord      navRR;
    NavDialogOptions    navDO;
#ifdef TARGET_API_MAC_CARBON
    NavEventUPP         navEF = NewNavEventUPP  (HYSavePopEventProc);
#else
    NavEventUPP         navEF = NewNavEventProc (HYSavePopEventProc);
#endif
    Handle              extensionListH = NewHandle (sizeof(NavMenuItemSpec)*menuOptions.lLength);
    checkPointer        (extensionListH);
    long                result;

    HLock (extensionListH);

    NavMenuItemSpec* extensionList = (NavMenuItemSpec*)*extensionListH;

    for (result = 0; result<menuOptions.lLength; result++) {
        extensionList[result].version = kNavMenuItemSpecVersion;
        extensionList[result].menuType = result+20;
        extensionList[result].menuCreator = 'MuSe';
        if (((_String*)menuOptions(result))->Equal(&menuSeparator)) {
            extensionList[result].menuItemName[0]=2;
            extensionList[result].menuItemName[1]='-';
            extensionList[result].menuItemName[2]='-';
        } else {
            StringToStr255 (*(_String*)menuOptions(result),extensionList[result].menuItemName);
        }
    }
    HUnlock (extensionListH);

    navDO.version       =   kNavDialogOptionsVersion;
    navDO.location      =   (Point) {
        -1,-1
    };
    navDO.dialogOptionFlags
        =   kNavDontAddTranslateItems;
    StringToStr255      (prompt,navDO.windowTitle);
    StringToStr255      (defFileName,navDO.savedFileName);
    memcpy (navDO.clientName,hpName,hpName[0]+1);
    navDO.actionButtonLabel[0]
        =   0;
    navDO.cancelButtonLabel[0]
        =   0;
    navDO.preferenceKey =   0;
    navDO.message[0]    =   0;

    if (menuOptions.lLength) {
        navDO.popupExtension=(NavMenuItemSpecHandle)extensionListH;
    } else {
        navDO.popupExtension=nil;
    }

    result              = 0;

    navErr              = NavPutFile (nil,
                                      &navRR,
                                      &navDO,
                                      navEF,
                                      nil,
                                      kNavGenericSignature,
                                      &result);


    if (navErr == noErr) {
        if (navRR.validRecord) {
            long countAED;
            if (noErr==AECountItems(&navRR.selection,&countAED)) {
                if (countAED==1) {
                    char    fileRec [2048];
                    Size    actualSize;
                    AEKeyword   keywd;
                    DescType    returnedType;
                    FSSpec*     fSR;
                    navErr= AEGetNthPtr(&navRR.selection, 1, typeFSS, &keywd,
                                        &returnedType, fileRec,
                                        sizeof(FSSpec), &actualSize);
                    if (navErr==noErr) {
                        fSR = (FSSpec*)fileRec;
                        GetFullPathName (*fSR, fileName);
                        NavDisposeReply (&navRR);
                        DisposeHandle (extensionListH);
#ifdef TARGET_API_MAC_CARBON
                        DisposeNavEventUPP (navEF);
#endif
                        return  result-20;
                    }
                }
            }
        }
        NavDisposeReply (&navRR);
    } else {
        if (navErr !=  userCanceledErr) {

            if (menuOptions.lLength) {
                _SimpleList         all,
                                    std,
                                    r;
                _List               ms;

                std << 0;
                std << 0;

                for (result = 0; result < menuOptions.lLength; result++) {
                    _String* option = (_String*)menuOptions (result);
                    _List    d (option);
                    ms && & d;
                    if (!option->Equal (&menuSeparator)) {
                        all << result;
                    }
                }

                DisposeHandle (extensionListH);
                result = HandleListSelection (ms,std,all,"Choose a file format",r,1);
                if (result>=0) {
                    result = all.lData[result];
                    _List dummy;
                    SaveFileWithPopUp (fileName,prompt,defFileName, listLabel, dummy);
                }
#ifdef TARGET_API_MAC_CARBON
                DisposeNavEventUPP (navEF);
#endif
                return result;
            } else {
                _String      errMsg ("System Error ");
                errMsg = errMsg & (long)navErr & " occured in SaveFileWithPopUp.";
                ProblemReport(errMsg);
            }
        }
    }
    DisposeHandle (extensionListH);
#ifdef TARGET_API_MAC_CARBON
    DisposeNavEventUPP (navEF);
#endif
    return -1;
}

//________________________________________________________
_HYColor        SelectAColor (_HYColor& currentColor, _String& prompt)
{
    RGBColor start, end;
    Point    center = {-1,-1};
    Str255   buffer;
    StringToStr255 (prompt,buffer);
    start.red   = currentColor.R*256;
    start.blue  = currentColor.B*256;
    start.green = currentColor.G*256;
    if (GetColor (center,buffer,&start,&end)) {
        _HYColor res;
        res.R = end.red/256;
        res.G = end.green/256;
        res.B = end.blue/256;
        return res;
    }
    return currentColor;
}


//________________________________________________________
char        YesNoCancelPrompt (_String& prompt)
{
    Str255 buffer;
    StringToStr255(prompt,buffer);
    ParamText(buffer,nil,nil,nil);
    return (Alert (132,myFilterProc));
}


//________________________________________________________
_HYRect     GetScreenDimensions (void)
{

    _HYRect   res = {0,0,0,0,0};

    /*RgnHandle dskRgn = GetGrayRgn();
    #ifdef OPAQUE_TOOLBOX_STRUCTS
        Rect      dskRect;
        GetRegionBounds(dskRgn,&dskRect);
    #else
        Rect      dskRect = (*dskRgn)->rgnBBox;
    #endif*/

    Rect      dskRect;
    GetAvailableWindowPositioningBounds(GetMainDevice(),&dskRect);

    res.right  = dskRect.right-dskRect.left;
    res.bottom = dskRect.bottom-dskRect.top;

    return res;
}

//________________________________________________________
void        CenterWindow (_HYGuiObject* g)
{
    _HYWindow* w = (_HYWindow*)g;

    if (!(w->flags & HY_WINDOW_SHEET)) {
        _HYRect   screen = GetScreenDimensions();

        long      cleft = 0, ctop = 0;

        if (screen.right>w->right) {
            cleft = (screen.right-w->right)/2;
        }
        if (screen.bottom>w->bottom) {
            ctop = (screen.bottom-w->bottom)/2;
        }

        w->_SetPosition (cleft,ctop);
    }
}

//__________________________________________________________________________________
void    StringToStr255 (_String& str, Str255& str255)
{
    long cpData = str.sLength>255?255:str.sLength;
    str255[0] = cpData;
    str255[cpData]=0;
    memcpy(str255+1, str.sData, cpData);

}

//__________________________________________________________________________________
void    StringToStr63 (_String& str, Str63& str63)
{
    long cpData = str.sLength>63?63:str.sLength;
    str63[0] = cpData;
    str63[63]=0;
    memcpy(str63+1, str.sData, cpData);

}

//__________________________________________________________________________________
void    Str255ToStr (_String& str, Str255& str255)
{
    if (str255[0] == 255) {
        str255[0]=254;
    }
    str255[str255[0]+1]=0;
    str = (char*)(str255+1);
}

//__________________________________________________________________________________

void    DrawEmbossedBox (Rect& theBox)
{
    DrawThemeListBoxFrame (&theBox,kThemeStateActive);
}

//__________________________________________________________________________________

pascal void PopUpThemeDrawProc (const Rect *bounds,ThemeButtonKind ,const ThemeButtonDrawInfo *,UInt32 userData,SInt16 ,Boolean )
{
    MoveTo (bounds->left+5, (bounds->top+bounds->bottom)/2+5);
    DrawString ((unsigned char*)userData);
}

ThemeButtonDrawUPP drawPT = NewThemeButtonDrawUPP (PopUpThemeDrawProc);

//__________________________________________________________________________________

void    DrawMenuPlaceHolder (Rect& theBox, _String& menuChoice, bool enabled)
{
    GrafPtr savedPtr;
    Style  savedFF, savedFNT, savedFS, savedMode;
    RGBColor
    savedColor;


    long   stringWidth, avWidth = theBox.right-theBox.left-24;

    GetPort (&savedPtr);
    GetForeColor (&savedColor);
#ifdef OPAQUE_TOOLBOX_STRUCTS
    savedFNT = GetPortTextFont (savedPtr),
    savedFS  = GetPortTextSize  (savedPtr);
    savedFF  = GetPortTextFace (savedPtr);
    savedMode = GetPortTextMode (savedPtr);
#else
    savedFF = savedPtr->txFace;
    savedFNT = savedPtr->txFont;
    savedFS = savedPtr->txSize;
    savedMode = savedPtr->txMode;
#endif

    TextSize (12);
    TextFace (normal);
    TextFont (0);
    TextMode (srcOr);
    Str255          boxTitle;

    RGBColor blackC = {0,0,0};
    RGBForeColor (&blackC);

    boxTitle[0] = menuChoice.sLength>255?255:menuChoice.sLength;
    BlockMove(menuChoice.sData, boxTitle+1,boxTitle[0]);
    stringWidth = StringWidth (boxTitle);
    while (stringWidth>avWidth) {
        boxTitle[0]--;
        stringWidth = StringWidth (boxTitle);
    }

    theBox.right++;
    theBox.bottom++;
    ThemeButtonDrawInfo binfo = {enabled?kThemeStateActive:kThemeStateInactive,kThemeButtonOff,kThemeAdornmentNone};
    DrawThemeButton (&theBox,kThemePopupButton,&binfo,nil,nil,drawPT,(UInt32)boxTitle);
    //FrameRoundRect (&theBox,6,6);
    theBox.right--;
    theBox.bottom--;
    TextFace (savedFF);
    TextFont (savedFNT);
    TextSize (savedFS);
    TextMode (savedMode);
    RGBForeColor(&savedColor);

}

//__________________________________________________________________________________

void    ListToPopUpMenu (_List& menuOptions, MenuHandle listMenu)
{
    Str255 buffer;
    for (long counter=0; counter<menuOptions.lLength; counter++) {
        _String *postItem = (_String*)(menuOptions(counter));
        if (*postItem==_String("SEPARATOR")) {
            AppendMenu (listMenu,"\p(-");
        } else {
            StringToStr255 (*postItem,buffer);
            AppendMenu (listMenu,buffer);
        }
    }
}

//__________________________________________________________________________________

_String HandlePullDown (_List& menuOptions, long l, long t,long startPos)
{
    if (menuOptions.lLength) {
        MenuHandle listMenu = NewMenu (GENERIC_POPUP_ID,"\p");
        ListToPopUpMenu (menuOptions, listMenu);
        InsertMenu(listMenu,hierMenu);

        long    res = PopUpMenuSelect (listMenu,t,l,startPos);
        DeleteMenu (GENERIC_POPUP_ID);
        DisposeMenu(listMenu);

        if (HiWord(res)) {
            return *(_String*)menuOptions (LoWord(res)-1);
        }
    }

    return empty;
}

//__________________________________________________________________________________

long HandlePullDownWithFont (_List& menuOptions, long l, long t,long startPos,_String fName,long fSize)
{
    MenuHandle listMenu = NewMenu (GENERIC_POPUP_ID,"\p");
    ListToPopUpMenu (menuOptions, listMenu);
    short  fontID;
    Str255 fName255;
    StringToStr255 (fName,fName255);
    GetFNum (fName255,&fontID);
    SetMenuFont     (listMenu,fontID,fSize);
    InsertMenu(listMenu,hierMenu);

    if ((startPos<1)||(startPos>menuOptions.lLength)) {
        startPos = 1;
    } else {
        SetItemMark (listMenu,startPos,0xA5);
    }
    long    res = PopUpMenuSelect (listMenu,t,l,startPos);
    DeleteMenu (GENERIC_POPUP_ID);
    DisposeMenu(listMenu);

    if (HiWord(res)) {
        return LoWord(res)-1;
    }
    return -1;

}
//__________________________________________________________________________________
char    ScanDirectoryForFileNames (_String& source, _List& rec, bool recurse)
{
    CInfoPBRec  fileRec;
    Str255 buffer;
    StringToStr255 (source,buffer);
    DirInfo*    dInfo = (DirInfo*)&fileRec;
    HFileInfo*  fInfo = (HFileInfo*)&fileRec;

    dInfo->ioNamePtr = buffer;
    dInfo->ioVRefNum = 0;
    dInfo->ioFDirIndex = 0;
    dInfo->ioDrDirID = 0;
    long  errCode;
    if ((errCode=PBGetCatInfo (&fileRec,false))==noErr)
        /* source directory exists */
    {
        long   counter = 1;
        dInfo->ioFDirIndex = 1;
        long   saveDirID = dInfo->ioDrDirID;
        while ((errCode=PBGetCatInfo (&fileRec,false))==noErr)
            /* index thru the items */
        {
            _String childDir ((unsigned long)dInfo->ioNamePtr[0],true);
            for (long k=1; k<=dInfo->ioNamePtr[0]; k++) {
                childDir << dInfo->ioNamePtr[k];
            }
            childDir.Finalize();
            if (childDir.sData[0] != '.') { // invisible file
                childDir = source & ':' & childDir;
                if (dInfo->ioFlAttrib&0x10)
                    // a directory
                {
                    if (recurse&&(!(dInfo->ioDrUsrWds.frFlags&fInvisible))) {
                        ScanDirectoryForFileNames (childDir,rec,true);
                    }
                } else if (!(fInfo->ioFlFndrInfo.fdFlags&fInvisible)) {
                    rec && & childDir;
                }
            }
            dInfo->ioDrDirID = saveDirID;
            dInfo->ioFDirIndex = ++counter;
        }
    }
    return ':';
}


//_________________________________________________________________________

void    SetWindowFont (short fID, short fSize, Style fStyle, bool onOff)
{
    static short sID, sSize;
    static short sStyle;

    if (onOff) {
        GrafPtr thisPort;
        GetPort (&thisPort);
#ifdef OPAQUE_TOOLBOX_STRUCTS
        sID = GetPortTextFont (thisPort),
        sSize  = GetPortTextSize    (thisPort);
        sStyle = GetPortTextFace (thisPort);
#else
        sID = thisPort->txFont;
        sSize = thisPort->txSize;
        sStyle = thisPort->txFace;
#endif
        TextFont (fID);
        TextSize (fSize);
        TextFace (fStyle);
    } else {
        TextFont (sID);
        TextSize (sSize);
        TextFace (sStyle);
    }

}

//_________________________________________________________________________

void    GenerateFontList (_List& fonts)
{
    fonts.Clear();
    MenuHandle fts = NewMenu (11111,"\p");
    AppendResMenu (fts,'FONT');
    {
        long icount = CountMenuItems(fts);
        for (long k=0; k<icount; k++) {
            Str255 mItem;
            GetMenuItemText (fts,k+1,mItem);
            _String fontName;
            Str255ToStr (fontName,mItem);
            fonts && & fontName;
        }
    }
    DisposeMenu (fts);
}

//_________________________________________________________________________
_HYColor    GetDialogBackgroundColor (void)
{
    RGBColor    bgc = {0xffff,0xffff,0xffff};
    GetThemeBrushAsColor (kThemeBrushDialogBackgroundActive,24,true,&bgc);

    _HYColor    res = {bgc.red/256,bgc.green/256,bgc.blue/256};
    return      res;
}

//_________________________________________________________________________
void    DelayNMs (long ms)
{
    unsigned long ticks,
             ftick;

    ticks = ms/16.66666667;
    Delay (ticks, &ftick);
}

//_________________________________________________________________________
void    PositionWindow          (_HYGuiObject* twp, _String* args)
{
    _List * argL = args->Tokenize (",");
    _HYWindow*   tw = (_HYWindow*)twp;
    if (argL->lLength>=4) {
        long R[5],
             k;

        for (k=0; k<4; k++) {
            R[k] = ((_String*)(*argL)(k))->toNum();
        }
        if (argL->lLength>4) {
            R[4] = ((_String*)(*argL)(4))->toNum();
        } else {
            R[4] = 0;
        }

        _HYRect   wR = GetScreenDimensions  (),
                  wiR;
        long      W[4] = {wR.left,wR.top, wR.right, wR.bottom};
        for (k=0; k<4; k++)
            if (R[k]<0) {
                R[k] = W[k] + ((k<2)?-1:1)*R[k];
            }

        wiR.left    = R[0];
        wiR.right   = R[2];
        wiR.top     = R[1];
        wiR.bottom  = R[3];

        if (wiR.left>=wiR.right) {
            wiR.right = 1+wiR.left;
        }
        if (wiR.top>=wiR.bottom) {
            wiR.bottom = 1+wiR.top;
        }

        tw->SetPosition        (wiR.left,wiR.top);
        tw->SetWindowRectangle (0,0,wiR.bottom-wiR.top,wiR.right-wiR.left);

        if (R[4]>0) {
            wiR.top         = wiR.bottom+2;
            wiR.bottom      = wR.bottom - 2;
            wiR.left        = 5;
            wiR.right       = wR.right - 2;
            MoveConsoleWindow (wiR);
        }

    }

    DeleteObject (argL);
}

//________________________________________________________

MenuHandle      BuildMenuFromList (_List& menuItems, long menuID, bool )
{
    MenuHandle itemPopUpMenu = NewMenu (menuID, "\p");
    checkPointer ((Ptr)itemPopUpMenu);
    Str255          menuBuffer;
    for (long counter = 0; counter < menuItems.lLength; counter++) {
        if (menuSeparator.Equal((_String*)menuItems.lData[counter])) {
            InsertMenuItem (itemPopUpMenu,"\p(-;",0x6FFF);
        } else {
            StringToStr255 (*((_String*)menuItems.lData[counter]), menuBuffer);
            InsertMenuItem (itemPopUpMenu,menuBuffer,0x6FFF);
        }
    }
    InsertMenu (itemPopUpMenu,hierMenu);
    return itemPopUpMenu;
}

//________________________________________________________

void    UpdateStatusLine(Ptr pW)
{
    WindowPtr  theWindow = (WindowPtr)pW;
    _Parameter verbLevel;
    checkParameter (VerbosityLevelString, verbLevel, 0.0);
    if (verbLevel>=-0.5) {
#ifdef TARGET_API_MAC_CARBON
        if (aquaInterfaceOn) {
            Rect wr,
                 box;
            GetWindowBounds (theWindow, kWindowGlobalPortRgn, &wr);
            OffsetRect (&wr,-wr.left,-wr.top);
            SetRect(&box,0,wr.bottom-15,wr.right - 15,wr.bottom);
            RgnHandle   dirtyRgn = NewRgn ();
            checkPointer (dirtyRgn);
            RectRgn (dirtyRgn, &box);
            QDFlushPortBuffer (GetWindowPort (theWindow), nil);
            DisposeRgn (dirtyRgn);
        }
#endif
    }
}

//________________________________________________________

OSStatus DownloadPB (void * , EventRecord * )
{
    // place holder
    return noErr;
}

//________________________________________________________

bool    Get_a_URL   (_String& urls, _String* fileName)
{
    URLReference    url;
    OSStatus        errCode;


    if ((errCode=URLNewReference (urls.sData, &url)) == noErr) {
        //URLSystemEventUPP gMySystemEventUPP = NewURLSystemEventUPP((URLSystemEventProcPtr)DownloadPB);
        if (fileName == nil) {
            Handle   h = NewHandle (0);

            errCode = URLDownload (url, nil, h,  kURLDisplayProgressFlag | kURLDisplayAuthFlag, nil, nil);

            if (errCode == noErr) {
                long    downloadSize = GetHandleSize(h);
                SetHandleSize(h, (downloadSize+1));
                HLock (h);
                (*h)[downloadSize] = 0;
                urls = (char*)(*h);
                URLDisposeReference (url);
                DisposeHandle (h);
                HUnlock (h);
                //DisposeURLSystemEventUPP(gMySystemEventUPP);
                return true;
            }
        } else {
            long    f = fileName->FindBackwards (":",0,-1);

            _String dirName  = fileName->Cut(0,f-1),
                    fName    = fileName->Cut(f+1,-1);


            if (f<0 && dirName.sLength==0 || fName.sLength==0) {
                urls = "Invalid file specification";
                //DisposeURLSystemEventUPP(gMySystemEventUPP);
                return false;
            }

            Str255          buffer;
            StringToStr255 (dirName,buffer);
            CInfoPBRec      hfp;

            hfp.dirInfo.ioNamePtr       = buffer;
            hfp.dirInfo.ioVRefNum       = 0;
            hfp.dirInfo.ioFDirIndex     = 0;
            hfp.dirInfo.ioDrDirID       = 0;

            if ((errCode=PBGetCatInfo (&hfp,false))==noErr) {
                FSSpec      fspec;
                StringToStr63 (fName,fspec.name);
                fspec.vRefNum = hfp.dirInfo.ioVRefNum;
                fspec.parID   = hfp.dirInfo.ioDrDirID;

                if (errCode == noErr) {
                    errCode = URLDownload (url, &fspec, nil, kURLReplaceExistingFlag |kURLDisplayProgressFlag | kURLDisplayAuthFlag, nil, nil);
                    if (errCode == noErr) {
                        hfp.hFileInfo.ioFlFndrInfo.fdCreator = (OSType)'????';
                        hfp.hFileInfo.ioFlFndrInfo.fdType    = (OSType)'????';
                        errCode = PBSetCatInfo (&hfp,false);
                        if (errCode == noErr)
                            //DisposeURLSystemEventUPP(gMySystemEventUPP);
                        {
                            return true;
                        }
                    }
                }
            }
        }
        //DisposeURLSystemEventUPP(gMySystemEventUPP);
    }
    switch (errCode) {
    case kETIMEDOUTErr:
        urls = "Connection timed out";
        break;
    case kECONNREFUSEDErr:
        urls = "Connection refused";
        break;
    case kEHOSTDOWNErr:
        urls = "Host down";
        break;
    case kEHOSTUNREACHErr:
        urls = "No route to host";
        break;
    case kURLInvalidURLError:
        urls = "The format of the URL is invalid";
        break;
    case kURLUnsupportedSchemeError:
        urls = "The transfer protocol is not supported";
        break;
    case kURLServerBusyError :
        urls = "Failed data transfer operation";
        break;
    default:
        urls = _String ("System error:") & (long)errCode;
    }

    return false;
}

//________________________________________________________

void    StartBarTimer(void)
{
    lastTimer   = time (&timerStart);
    updateTimer = true;
}

//________________________________________________________

void    StopBarTimer(void)
{
    updateTimer = false;
}

//_________________________________________________________________________

void    MoveConsoleWindow (_HYRect& newLoc)
{
    if (newLoc.right&&newLoc.bottom) {
        hyphyConsoleWindow->SetPosition         (newLoc.left, newLoc.top);
        hyphyConsoleWindow->SetWindowRectangle  (newLoc.top,newLoc.left,newLoc.bottom,newLoc.right,true);
    }
}


//__________________________________________________________________________________
void    PlaceStringInClipboard (_String& res,Ptr )
{
#ifdef TARGET_API_MAC_CARBON
    ClearCurrentScrap();
    ScrapRef         theScrapRef;
    GetCurrentScrap(&theScrapRef);
    PutScrapFlavor(theScrapRef, 'TEXT', kScrapFlavorMaskNone,res.sLength,res.sData);
#else
    ZeroScrap();
    PutScrap (res.sLength,'TEXT',res.sData);
#endif
}