/*
    Toolbar component for Mac OS API

    Sergei L. Kosakovsky Pond, May 2000-December 2002
*/

#include "HYPlatformGraphicPane.h"

#include "errorfns.h"
#include "HYButtonBar.h"
#include "HYUtils.h"
#include "HYEventTypes.h"

#include "ToolUtils.h"
#include "Appearance.h"
#include "string.h"


//__________________________________________________________________

RGBColor
buttonBorder1 = {0,0,0},
buttonBorder2 = {0x3fff,0x3fff,0x3fff};

//__________________________________________________________________

_HYPlatformButtonBar::_HYPlatformButtonBar(void)
{
    backFill = NewPixPat();
    if (!backFill) {
        warnError (-108);
    }
    RGBColor  wht = {0xffff,0xffff,0xffff};
    MakeRGBPat (backFill,&wht);
    pushed = -1;
    saveMousePosH = -1;
    saveMousePosV = -1;
    lastSave = 0;
    toolTipBounds.left = 0;
#ifdef TARGET_API_MAC_CARBON
    //EventLoopRef    mainLoop;
    //mainLoop = GetMainEventLoop();
    timerUPP = NewEventLoopTimerUPP(ButtonBarTimer);
    //InstallEventLoopTimer (mainLoop,0,1.5*kEventDurationSecond,timerUPP,this,&theTimer);
    theTimer = nil;
#endif
}

//__________________________________________________________________

_HYPlatformButtonBar::~_HYPlatformButtonBar(void)
{
    if (backFill) {
        DisposePixPat (backFill);
    }
#ifdef TARGET_API_MAC_CARBON
    if (theTimer) {
        RemoveEventLoopTimer (theTimer);
    }
    DisposeEventLoopTimerUPP (timerUPP);
#endif
}

//__________________________________________________________________

void    _HYPlatformButtonBar::_DisposeButtons(void)
{
    _HYButtonBar* theParent = (_HYButtonBar*)this;
    for (long i=0; i<theParent->ButtonCount(); i++) {
        CIconHandle thisIcon = (CIconHandle)theParent->buttons.lData[i];
        DisposeCIcon (thisIcon);
    }
}

//__________________________________________________________________

void    _HYPlatformButtonBar::_DisposeButton(long k)
{
    _HYButtonBar* theParent = (_HYButtonBar*)this;
    if ((k<theParent->ButtonCount())&&(k>=0)) {
        CIconHandle thisIcon = (CIconHandle)theParent->buttons.lData[k];
        DisposeCIcon (thisIcon);
    }
}




//__________________________________________________________________

void        _HYPlatformButtonBar::_SetBackColor (_HYColor& c)
{
    RGBColor newBG;
    newBG.red = c.R*256;
    newBG.blue = c.B*256;
    newBG.green = c.G*256;
    MakeRGBPat (backFill,&newBG);

}

//__________________________________________________________________
void        _HYPlatformButtonBar::_SetVisibleSize (_HYRect rel)
{
    _HYButtonBar* theParent = (_HYButtonBar*) this;
    buttonRect.left=rel.left;
    buttonRect.top = rel.top;
    _HYRect s = theParent->_SuggestDimensions();
    buttonRect.right = buttonRect.left+s.right;
    buttonRect.bottom = buttonRect.top+s.bottom;
    AlignRectangle (rel, buttonRect, theParent->GetAlignFlags());
}

//__________________________________________________________________

void        _HYButtonBar::_Activate (void)
{
    if (!activationFlag)
        for (long k=0; k<enabledButtons.lLength; k++) {
            _MarkButtonForUpdate (enabledButtons.lData[k]);
        }
    if (!theTimer) {
        EventLoopRef      mainLoop;
        mainLoop = GetMainEventLoop();
        InstallEventLoopTimer (mainLoop,0,.5*kEventDurationSecond,timerUPP,this,&theTimer);
    }
    _HYPlatformComponent::_Activate();
}

//__________________________________________________________________

void        _HYButtonBar::_Deactivate (void)
{
    if (activationFlag) {
        for (long k=0; k<enabledButtons.lLength; k++) {
            _MarkButtonForUpdate (enabledButtons.lData[k]);
        }
        if (toolTipBounds.left) {
#ifdef TARGET_API_MAC_CARBON
            InvalWindowRect (parentWindow,&toolTipBounds);
            //HMHideTag ();
#else
            InvalRect (&toolTipBounds);
#endif
            toolTipBounds.left = 0;

        }
    }

#ifdef TARGET_API_MAC_CARBON
    if (theTimer) {
        RemoveEventLoopTimer(theTimer);
        theTimer = nil;
    }
#endif
    _HYPlatformComponent::_Deactivate();
}

//__________________________________________________________________

void        _HYButtonBar::_ComponentMouseExit (void)
{
    if (toolTipBounds.left) {
#ifdef TARGET_API_MAC_CARBON
        InvalWindowRect (parentWindow,&toolTipBounds);
        //HMHideTag ();
#else
        InvalRect (&toolTipBounds);
#endif
        toolTipBounds.left = 0;
    }
#ifdef TARGET_API_MAC_CARBON
    if (theTimer) {
        RemoveEventLoopTimer(theTimer);
        theTimer = nil;
    }
#endif
}


//__________________________________________________________________
void        _HYPlatformButtonBar::_Paint (Ptr p)
{

    _HYButtonBar * theParent = (_HYButtonBar*)this;
    _HYRect * relRect = (_HYRect*)p;
    Rect    cRect,iRect;
    cRect.left = relRect->left;
    cRect.right = relRect->right;
    cRect.top = relRect->top;
    cRect.bottom = relRect->bottom;
    if (!(theParent->settings.width&HY_COMPONENT_TRANSP_BG)) {
        FillCRect (&cRect,backFill);
    } else {
        EraseRect (&cRect);
    }
    RgnHandle saveRgn = NewRgn();

    if (!saveRgn) {
        warnError(-108);
    }

    GetClip (saveRgn);
    ClipRect (&cRect);
    cRect.left = buttonRect.left;
    cRect.top = buttonRect.top;
    int   step = theParent->GetButtonDim()+2*HY_BUTTONBAR_BORDER;
    cRect.right = cRect.left+step;
    cRect.bottom = cRect.top+step;
    RGBColor saveColor;
    GetForeColor (&saveColor);
    RGBForeColor (&buttonBorder1);
    PenSize (1,1);
    for (long i=0; i<theParent->ButtonCount(); i++) {
        if (i&&(i%theParent->BarWidth()==0)) {
            cRect.left = buttonRect.left;
            cRect.top +=step;
            cRect.bottom +=step;
            cRect.right = cRect.left+step;
        }
        iRect = cRect;
        //ThemeButtonDrawInfo binfo = {theParent->activationFlag?kThemeStateActive:kThemeStateInactive,kThemeButtonOff,kThemeAdornmentNone};
        //DrawThemeButton (&iRect,kThemeBevelButton,&binfo,nil,nil,nil,0);
        InsetRect (&iRect,HY_BUTTONBAR_BORDER,HY_BUTTONBAR_BORDER);
        if (theParent->activationFlag)
            if (i==pushed) {
                PlotCIconHandle (&iRect,atNone,ttSelected,(CIconHandle)theParent->buttons.lData[i]);
            } else {
                if (theParent->enabledButtons.Find(i)>=0) {
                    PlotCIcon (&iRect,(CIconHandle)theParent->buttons.lData[i]);
                } else {
                    PlotCIconHandle (&iRect,atNone,ttDisabled,(CIconHandle)theParent->buttons.lData[i]);
                }
            }
        else {
            PlotCIconHandle (&iRect,atNone,ttDisabled,(CIconHandle)theParent->buttons.lData[i]);
        }

        MoveTo (iRect.left-1,iRect.top-1);
        LineTo (iRect.right+1,iRect.top-1);
        LineTo (iRect.right+1,iRect.bottom+1);
        LineTo (iRect.left-1,iRect.bottom+1);
        LineTo (iRect.left-1,iRect.top-1);

        cRect.left +=step;
        cRect.right +=step;
    }

    RGBForeColor (&saveColor);

    SetClip    (saveRgn);
    DisposeRgn (saveRgn);

    (*theParent)._HYPlatformComponent::_Paint(p);
}
//__________________________________________________________________
_HYRect _HYPlatformButtonBar::_GetButtonRect (bool conv)
{
    _HYRect res;
    res.left   = buttonRect.left;
    res.right  = buttonRect.right;
    res.top    = buttonRect.top;
    res.bottom = buttonRect.bottom;
    if (conv) {
        GrafPtr thisPort;
        GetPort (&thisPort);
#ifdef OPAQUE_TOOLBOX_STRUCTS
        SetPort (GetWindowPort(((_HYButtonBar*)this)->parentWindow));
#else
        SetPort (((_HYButtonBar*)this)->parentWindow);
#endif
        Point   c;
        c.v = res.top;
        c.h = res.left;
        LocalToGlobal(&c);
        res.top = c.v;
        res.left = c.h;
        c.v = res.bottom;
        c.h = res.right;
        LocalToGlobal(&c);
        res.bottom = c.v;
        res.right = c.h;
        SetPort (thisPort);
    }
    return res;
}


//__________________________________________________________________
void        _HYPlatformButtonBar::_Update (Ptr p)
{
    _Paint (p);
}

//__________________________________________________________________
void        _HYPlatformButtonBar::_MarkButtonForUpdate (int i)
{
    _HYButtonBar* theParent = (_HYButtonBar*)this;
    if ((i>=0)&&(i<theParent->ButtonCount())) {
        int hR = i%theParent->BarWidth(),
            vR = i/theParent->BarWidth(),
            step = 2*HY_BUTTONBAR_BORDER+theParent->GetButtonDim();

        Rect invRect;
        invRect.left  = buttonRect.left+hR*step;
        invRect.right = invRect.left+step;
        invRect.top   = buttonRect.top+vR*step;
        invRect.bottom= invRect.top+step;
        GrafPtr savePort;
        GetPort (&savePort);
#ifdef OPAQUE_TOOLBOX_STRUCTS
        SetPort (GetWindowPort(theParent->parentWindow));
#else
        SetPort (theParent->parentWindow);
#endif
#ifdef TARGET_API_MAC_CARBON
        InvalWindowRect (theParent->parentWindow,&invRect);
#else
        InvalRect (&invRect);
#endif

        if (forceUpdateForScrolling) {
            Rect    rel = HYRect2Rect (theParent->rel);
            SectRect (&rel,&invRect,&invRect);
            _HYRect br = {invRect.top,invRect.left,invRect.bottom,invRect.right,0};
            theParent->Paint((Ptr)&br);
        }
        SetPort (savePort);
    }
}

//__________________________________________________________________
void        _HYPlatformButtonBar::_UnpushButton (void)
{
    if (pushed>=0) {
        _MarkButtonForUpdate(pushed);
        pushed = -1;
    }
}


//__________________________________________________________________
void        _HYPlatformButtonBar::_SetDimensions (_HYRect r, _HYRect rel)
{
    _HYButtonBar* theParent = (_HYButtonBar*) this;
    theParent->_HYPlatformComponent::_SetDimensions (r,rel);
    _SetVisibleSize (rel);
}

//__________________________________________________________________
int         _HYPlatformButtonBar::_FindClickedButton (int h, int v)
{
    Point localClick = {v,h};
    _HYButtonBar * parent = (_HYButtonBar*)this;
    if (PtInRect (localClick,&buttonRect)) {
        int v = localClick.v-buttonRect.top,
            h = localClick.h-buttonRect.left,
            step = 2*HY_BUTTONBAR_BORDER+parent->buttonDim,
            hR = h/step,
            vR = v/step;

        v-=vR*step;
        h-=hR*step;
        if ((v>HY_BUTTONBAR_BORDER)&&(v<step-HY_BUTTONBAR_BORDER)&&
                (h>HY_BUTTONBAR_BORDER)&&(h<step-HY_BUTTONBAR_BORDER)) {
            return hR+vR*parent->barW;
        }
    }
    return -1;

}

//__________________________________________________________________

#ifdef TARGET_API_MAC_CARBON
pascal void ButtonBarTimer (EventLoopTimerRef ,void* userData)
{
    Point    curMouse;
    GetGlobalMouse (&curMouse);
    //LocalToGlobal (&curMouse);
    _HYButtonBar * myBB = (_HYButtonBar*)userData;
    unsigned long t;
    GetDateTime(&t);

    if ((curMouse.h==myBB->saveMousePosH)
            &&(curMouse.v==myBB->saveMousePosV)) {
        if (!myBB->toolTipBounds.left) {
            GrafPtr curPort;
            GetPort (&curPort);
            SetPort (GetWindowPort (myBB->parentWindow));
            myBB->_DisplayToolTip();
            SetPort (curPort);
        }
    }

    myBB->saveMousePosH = curMouse.h;
    myBB->saveMousePosV = curMouse.v;
    myBB->lastSave      = t;
}

#endif

//__________________________________________________________________
void        _HYButtonBar::_DisplayToolTip      (void)
{
    Point p = {saveMousePosV,saveMousePosH};
    GlobalToLocal (&p);
    int h = _FindClickedButton (p.h,p.v);

    /*#ifdef TARGET_API_MAC_CARBON
    if (h>-1)
    {
        _String* toolTip = (_String*)toolTips(h);
        if (toolTip->sLength)
        {
            int x,
                y;

            GetButtonLoc (h,x,y,true);
            HMHelpContentRec hmr;

            hmr.absHotRect.left  = x;
            hmr.absHotRect.top   = y;
            hmr.absHotRect.right = x+GetButtonDim();
            hmr.absHotRect.bottom= y+GetButtonDim();

            hmr.version = kMacHelpVersion;
            hmr.tagSide = kHMDefaultSide;

            hmr.content[0].contentType = kHMPascalStrContent;
            hmr.content[1].contentType = kHMPascalStrContent;
            StringToStr255 (*toolTip,hmr.content[0].u.tagString);
            StringToStr255 (*toolTip,hmr.content[1].u.tagString);

            HMDisplayTag (&hmr);
            toolTipBounds.left = 1;
        }
    }
    #else*/
    if ((h>-1)&&(h<toolTips.lLength)) {
        _String* toolTip = (_String*)toolTips(h);
        if (toolTip->sLength) {
            //RGBColor      toolTipColor = {0x0000,0x3A00,0x8A00};
            RGBColor      toolTipColor = {0xFFFF,0xCCFF,0x6600};
            PixPatHandle  toolTipPixPat = NewPixPat();
            MakeRGBPat (toolTipPixPat,&toolTipColor);
            int        bL,bT;
            GetButtonLoc (h,bL,bT,false);
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
            TextFont (kFontIDHelvetica);
            TextSize (12);
            TextFace (0);
            toolTipBounds.bottom = bT-1;
            toolTipBounds.top = toolTipBounds.bottom - 15;
            if (toolTipBounds.top<0) {
                toolTipBounds.top = bT+buttonDim+1;
                toolTipBounds.bottom = toolTipBounds.top+15;
            }
            h = GetVisibleStringWidth (*toolTip)+4;
            toolTipBounds.left = bL+(buttonDim-h-1)/2;
            if (toolTipBounds.left<=0) {
                toolTipBounds.left = 1;
            }
            toolTipBounds.right = toolTipBounds.left+h+2;
#ifdef OPAQUE_TOOLBOX_STRUCTS
            Rect portRect;
            GetPortBounds (thisPort,&portRect);
            h = toolTipBounds.right-portRect.right+portRect.left;
#else
            h = toolTipBounds.right-thisPort->portRect.right+thisPort->portRect.left;
#endif
            if (h>0) {
                if (h>=toolTipBounds.left) {
                    h = toolTipBounds.left-1;
                }
                toolTipBounds.left -= h;
                toolTipBounds.right-= h;
            }
            RGBColor oldColor;
            GetForeColor (&oldColor);
            toolTipColor.red = toolTipColor.blue = toolTipColor.green = 0x0000;
            RGBForeColor (&toolTipColor);
            FillCRect  (&toolTipBounds,toolTipPixPat);
            FrameRect  (&toolTipBounds);
            MoveTo (toolTipBounds.left+3,toolTipBounds.bottom-3);
            DrawText (toolTip->sData,0,toolTip->sLength);
            RGBForeColor (&oldColor);
            TextFont (savedFace);
            TextSize (savedSize);
            TextFace (savedStyle);
            DisposePixPat (toolTipPixPat);
        }
    }
    //#endif
}


//__________________________________________________________________

bool _HYButtonBar::_ProcessOSEvent (Ptr vEvent)
{
    EventRecord*    theEvent = (EventRecord*)vEvent;
    WindowPtr       dummy;
#ifdef          TARGET_API_MAC_CARBON
    if (!theTimer) {
        EventLoopRef      mainLoop;
        mainLoop = GetMainEventLoop();
        InstallEventLoopTimer (mainLoop,0,.5*kEventDurationSecond,timerUPP,this,&theTimer);
    }
#endif
    if (buttons.lLength)
        switch (theEvent->what) {
        case mouseDown: {
            if (toolTipBounds.left) {
#ifdef TARGET_API_MAC_CARBON
                InvalWindowRect (parentWindow,&toolTipBounds);
                //HMHideTag ();
#else
                InvalRect (&toolTipBounds);
#endif
                toolTipBounds.left = 0;
                GetDateTime (&lastSave);
            }
            long evtType = FindWindow (theEvent->where,&dummy);
            switch (evtType) {
            case inContent: {
                Point localClick = theEvent->where;
                GlobalToLocal (&localClick);
                int h = _FindClickedButton (localClick.h,localClick.v);
                if ((h>=0)&&(enabledButtons.Find(h)>=0)) {
                    if (pullDownButtons.Find(h)<0) {
                        long lastFound = h,
                             cf;
                        forceUpdateForScrolling = true;
                        pushed = h;
                        _MarkButtonForUpdate (h);
#ifdef TARGET_API_MAC_CARBON
                        MouseTrackingResult  trackingResult = kMouseTrackingMousePressed;
                        GetMouse (&localClick);
                        while (trackingResult != kMouseTrackingMouseReleased) {
                            TrackMouseLocation (NULL, &localClick, &trackingResult);
                            cf = _FindClickedButton(localClick.h,localClick.v);
                            if (cf!=lastFound) {
                                if (lastFound==h) {
                                    pushed = -1;
                                    _MarkButtonForUpdate (h);
                                } else if (cf==h) {
                                    pushed = h;
                                    _MarkButtonForUpdate (h);
                                }

                                lastFound = cf;
                            }
                        }
#else
                        while (WaitMouseUp()) {
                            GetMouse (&localClick);
                            cf = _FindClickedButton(localClick.h,localClick.v);
                            if (cf!=lastFound) {
                                if (lastFound==h) {
                                    pushed = -1;
                                    _MarkButtonForUpdate (h);
                                } else if (cf==h) {
                                    pushed = h;
                                    _MarkButtonForUpdate (h);
                                }

                                lastFound = cf;
                            }
                        }
#endif
                        pushed = -1;
                        forceUpdateForScrolling = false;
                    }
                    if (_FindClickedButton(localClick.h,localClick.v)==h) {
                        //pushed = h;
                        pushed = -1;
                        forceUpdateForScrolling = true;
                        _MarkButtonForUpdate (h);
                        forceUpdateForScrolling = false;
                        SendButtonPush(h);
                    }
                }
                return true;
            }
            }
            break;
        }
        default: {
            if (!StillDown()) {
                if (pushed>=0) {
                    _MarkButtonForUpdate (pushed);
                    pushed = -1;
                    return true;
                }
                if (theEvent->what == osEvt) {
                    unsigned long t;
                    GetDateTime(&t);
                    if ((theEvent->where.h==saveMousePosH)
                            &&(theEvent->where.v==saveMousePosV)) {
                        if ((t-lastSave>1)&&(!toolTipBounds.left)) {
                            _DisplayToolTip();
                            lastSave = t;
                        }
                    } else {
                        if (toolTipBounds.left) {
#ifdef TARGET_API_MAC_CARBON
                            InvalWindowRect (parentWindow,&toolTipBounds);
                            //HMHideTag ();
#else
                            InvalRect (&toolTipBounds);
#endif
                            toolTipBounds.left = 0;
                        }
                        saveMousePosH = theEvent->where.h;
                        saveMousePosV = theEvent->where.v;
                        lastSave = t;
                    }
                    return true;
                }
            }
        }
        }
    return _HYPlatformComponent::_ProcessOSEvent (vEvent);
}

//EOF