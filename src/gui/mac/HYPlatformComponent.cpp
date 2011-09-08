/*
    A general composite window component object, MacOS specifics

    Sergei L. Kosakovsky Pond, May 2000.
*/

#include "HYComponent.h"
#include "HYPlatformComponent.h"

#ifndef  TARGET_API_MAC_CARBON
#include "Windows.h"
#endif

#include "HYWindow.h"
#include "Controls.h"
#include "ControlDefinitions.h"
#include "Palettes.h"
#include "HYEventTypes.h"
#include "HYCanvas.h"
#include "HYUtils.h"

#ifdef   __MACHACKMP__
#include "mypthread.h"
#endif

#include "QDoffscreen.h"
#include "Quicktime.h"


//__________________________________________________________________________________

extern  long        smallScrollStep,
        scrollStepCounter,
        lastScrollControlValue;

extern  _HYGuiObject *
scrollingWindow;

extern  bool        hScrollingAction;
extern  Point       lastScrollPoint;

bool    forceUpdateForScrolling = false;

extern  RGBColor    menuLine1,
        menuLine2;

//__________________________________________________________________________________

void            AlignRectangle (_HYRect& rel , Rect& target , unsigned char alFlags)
{
    long     temp;
    if (alFlags&HY_ALIGN_RIGHT) {
        temp = target.right-target.left;
        target.right = rel.right;
        target.left = target.right - temp;
    } else if (!(alFlags&HY_ALIGN_LEFT)) {
        temp = (rel.right-rel.left-target.right+target.left)/2;
        target.left+=temp;
        target.right+=temp;
    }

    if (alFlags&HY_ALIGN_BOTTOM) {
        temp = target.bottom-target.top;
        target.bottom = rel.bottom;
        target.top = target.bottom - temp;
    } else if (!(alFlags&HY_ALIGN_TOP)) {
        temp = (rel.bottom-rel.top-target.bottom+target.top)/2;
        target.top+=temp;
        target.bottom+=temp;
    }
}

//__________________________________________________________________

pascal void scrollAction (ControlHandle,ControlPartCode);

/*//__________________________________________________________________

pascal void  controlScrollAction (ControlHandle theControl,ControlPartCode ctlPart)
{
    long   cv  = GetControl32BitValue (theControl),
           cv2 = cv;

    long    smStep,
            invisPixels;

    _HYPlatformComponent*    thisC      = (_HYPlatformComponent*)GetControlReference (theControl);
    _HYComponent*            theParent  = (_HYComponent*)thisC;
    bool                     hScrAction = (thisC->hScroll==theControl);

    if (hScrAction)
        invisPixels = theParent->GetMaxW()-(theParent->GetHSize());
    else
        invisPixels = theParent->GetMaxH()-(theParent->GetVSize());

    smStep = (double)MAX_CONTROL_VALUE/invisPixels;
    if (!smStep)
        smStep = 1;


    switch (ctlPart)
    {
        case kControlDownButtonPart:
            HiliteControl (theControl,kControlDownButtonPart);
            cv2 = cv+smStep;
            if (cv2>MAX_CONTROL_VALUE)
                cv2 = MAX_CONTROL_VALUE;
            break;
        case kControlUpButtonPart:
            HiliteControl (theControl,kControlUpButtonPart);
            cv2 = cv-smStep;
            if (cv2<0)
                cv2 = 0;
            break;
        case kControlPageUpPart:
            HiliteControl (theControl,kControlPageUpPart);
            cv2 = cv-100*smStep;
            if (cv2<0)
                cv2 = 0;
            break;
        case kControlPageDownPart:
            HiliteControl (theControl,kControlPageDownPart);
            cv2 = cv+100*smStep;
            if (cv2>MAX_CONTROL_VALUE)
                cv2 = MAX_CONTROL_VALUE;
            break;
        case kControlIndicatorPart:
            cv2 = cv;
            cv  = hScrAction?thisC->lastHScroll:thisC->lastVScroll;
            printf ("%d %d\n", cv, cv2);
            break;
    }

    if (cv!=cv2)
    {
        SetControl32BitValue (theControl,cv2);
        forceUpdateForScrolling = true;
        if (hScrAction)
        {
            ((_HYComponent*)thisC)->ProcessEvent (generateScrollEvent (cv2-cv,0));
            thisC->lastHScroll = cv2;
        }
        else
        {
            ((_HYComponent*)thisC)->ProcessEvent (generateScrollEvent (0,cv2-cv));
            thisC->lastVScroll = cv2;
        }
        forceUpdateForScrolling = false;
    }
}

//__________________________________________________________________

ControlActionUPP      ctlActionUPP = NewControlActionUPP (controlScrollAction);*/
//__________________________________________________________________

_HYPlatformComponent::_HYPlatformComponent(void)
{
    vScroll = hScroll = nil;
}

//__________________________________________________________________

_HYPlatformComponent::_HYPlatformComponent(_HYRect s,Ptr w)
{
    bool    memError = false;
    vScroll = hScroll = nil;
    WindowPtr theWindow = (WindowPtr)w;
    Rect    cSize;
    if (s.width&HY_COMPONENT_H_SCROLL) {
        cSize.left = cSize.top = 0;
        cSize.right = 100;
        cSize.bottom = 15;
        hScroll = NewControl (theWindow,&cSize,"\p",false,0,0,MAX_CONTROL_VALUE,kControlScrollBarLiveProc,(SInt32)this);
        if (!hScroll) {
            memError = true;
        } else {
            SetControl32BitMinimum (hScroll,0);
            SetControl32BitMaximum (hScroll,MAX_CONTROL_VALUE);
            //SetControlAction     (hScroll, ctlActionUPP);
        }
    }
    if (s.width&HY_COMPONENT_V_SCROLL) {
        cSize.left = cSize.top = 0;
        cSize.bottom = 100;
        cSize.right = 15;
        vScroll = NewControl (theWindow,&cSize,"\p",false,0,0,MAX_CONTROL_VALUE,kControlScrollBarLiveProc,(SInt32)this);
        if (!vScroll) {
            memError = true;
        } else {
            SetControl32BitMinimum (vScroll,0);
            SetControl32BitMaximum (vScroll,MAX_CONTROL_VALUE);
            //SetControlAction     (vScroll, ctlActionUPP);
        }
    }
    if (memError) {
        _String errMsg = "Could not allocate memory for a window component structure.";
        FlagError (errMsg);
    }
    parentWindow = theWindow;
    lastHScroll = 0;
    lastVScroll = 0;
}

//__________________________________________________________________

void _HYPlatformComponent::_CleanUp(void)
{
    if (hScroll) {
        DisposeControl (hScroll);
        hScroll = nil;
    }
    if (vScroll) {
        DisposeControl (vScroll);
        vScroll = nil;
    }
}

//__________________________________________________________________
long        _HYPlatformComponent::_GetHScrollerPos (void)
{
    if (hScroll) {
        return GetControl32BitValue (hScroll);
    } else {
        return 0;
    }
}
//__________________________________________________________________
long        _HYPlatformComponent::_GetVScrollerPos (void)
{
    if (vScroll) {
        return GetControl32BitValue (vScroll);
    } else {
        return 0;
    }
}

//__________________________________________________________________
void        _HYPlatformComponent::_SetHScrollerPos (long nv)
{
    if (hScroll) {
        if (nv<0) {
            nv = 0;
        } else if (nv>MAX_CONTROL_VALUE) {
            nv = MAX_CONTROL_VALUE;
        }
        SetControl32BitValue (hScroll,nv);
    }
}

//__________________________________________________________________
void        _HYPlatformComponent::_SetVScrollerPos (long nv)
{
    if (vScroll) {
        if (nv<0) {
            nv = 0;
        } else if (nv>MAX_CONTROL_VALUE) {
            nv = MAX_CONTROL_VALUE;
        }
        SetControl32BitValue (vScroll,nv);
    }
}


//__________________________________________________________________
void _HYPlatformComponent::Duplicate (BaseRef s)
{
    _HYComponent* theS = (_HYComponent*)s;
    _CleanUp();
    hScroll = theS->hScroll;
    vScroll = theS->vScroll;
}

//__________________________________________________________________
void _HYPlatformComponent::_SetDimensions (_HYRect,_HYRect r)
{
    _SetVisibleSize (r);
}

//__________________________________________________________________
void _HYPlatformComponent::_MarkForUpdate (void)
{
    Rect inv;
    inv.left = rel.left;
    inv.right = rel.right;
    inv.bottom = rel.bottom;
    inv.top = rel.top;
    GrafPtr   savePort;
    GetPort   (&savePort);
#ifdef OPAQUE_TOOLBOX_STRUCTS
    SetPort (GetWindowPort(parentWindow));
#else
    SetPort (parentWindow);
#endif

    if (forceUpdateForScrolling) {
        //BeginUpdate(parentWindow);
        _Paint((Ptr)&rel);
        //EndUpdate(parentWindow);
    } else
#ifdef TARGET_API_MAC_CARBON
        InvalWindowRect (parentWindow,&inv);
#else
        InvalRect (&inv);
#endif

    SetPort   (savePort);
}

//__________________________________________________________________
void _HYPlatformComponent::_MarkContentsForUpdate (void)
{
    Rect inv;
    inv.left = rel.left;
    inv.right = rel.right;
    inv.bottom = rel.bottom;
    inv.top = rel.top;
    if (hScroll) {
        inv.bottom -= HY_SCROLLER_WIDTH;
    }
    if (vScroll) {
        inv.right -= HY_SCROLLER_WIDTH;
    }
    GrafPtr   savePort;
    GetPort   (&savePort);
#ifdef OPAQUE_TOOLBOX_STRUCTS
    SetPort (GetWindowPort(parentWindow));
#else
    SetPort (parentWindow);
#endif
    if (forceUpdateForScrolling) {
        _Paint((Ptr)&rel);
    } else
#ifdef TARGET_API_MAC_CARBON
        InvalWindowRect (parentWindow,&inv);
#else
        InvalRect (&inv);
#endif

    SetPort   (savePort);
}

//__________________________________________________________________

void        _HYPlatformComponent::_SetVisibleSize (_HYRect r)
{
    _HYComponent * theParent = (_HYComponent*)this;
    long        t, v;

    _Parameter  newSize;

    if (hScroll&&!vScroll) { // only horizontal scroll bar
        MoveControl (hScroll,r.left-1,r.bottom-16);
        SizeControl (hScroll,r.right-r.left+1,16);
        t = theParent->GetMaxW();
        v = r.right-r.left+1;
        //char buf[256];
        //sprintf (buf,"%d %d\n",t,v);
        //BufferToConsole (buf);
        if (t>v) {
            if (GetControl32BitMaximum (hScroll)!=MAX_CONTROL_VALUE) {
                lastHScroll = 0;
                SetControl32BitMaximum (hScroll,MAX_CONTROL_VALUE);
            }

            HiliteControl (hScroll,0);
            newSize = MAX_CONTROL_VALUE*(_Parameter)v/(t-v);
            if (newSize>0x6fffffff) {
                newSize = 0x6fffffff;
            }
            SetControlViewSize (hScroll,(long)newSize);
        } else {
            SetControl32BitMaximum (hScroll,0);
            SetControl32BitValue   (hScroll,0);
            HiliteControl (hScroll, kControlInactivePart);
        }
    }
    if (vScroll&&!hScroll) { // only vertical scroll bar
        MoveControl (vScroll,r.right-16,r.top-1);
        SizeControl (vScroll,16,r.bottom-r.top+1);
        t = theParent->GetMaxH();
        v = r.bottom-r.top+1;
        if (t>v) {
            if (GetControl32BitMaximum (vScroll)!=MAX_CONTROL_VALUE) {
                lastVScroll = 0;
                SetControl32BitMaximum (vScroll,MAX_CONTROL_VALUE);
            }

            HiliteControl (vScroll,0);
            newSize = MAX_CONTROL_VALUE*(_Parameter)v/(t-v);
            if (newSize>0x6fffffff) {
                newSize = 0x6fffffff;
            }
            SetControlViewSize (vScroll,(long)newSize);
        } else {
            SetControl32BitMaximum (vScroll,0);
            SetControl32BitValue   (vScroll,0);
            HiliteControl (vScroll, kControlInactivePart);
        }
    }
    if (vScroll&&hScroll) {
        MoveControl (hScroll,r.left-1,r.bottom-16);
        SizeControl (hScroll,r.right-r.left-13,16);
        t = theParent->GetMaxW();
        v = r.right-r.left+1+HY_SCROLLER_WIDTH;
        if (t>v) {
            if (GetControl32BitMaximum (hScroll)!=MAX_CONTROL_VALUE) {
                lastHScroll = 0;
                SetControl32BitMaximum (hScroll,MAX_CONTROL_VALUE);
            }
            HiliteControl (hScroll,0);
            newSize = MAX_CONTROL_VALUE*(_Parameter)v/(t-v);
            if (newSize>0x6fffffff) {
                newSize = 0x6fffffff;
            }
            SetControlViewSize (hScroll,(long)newSize);
        } else {
            SetControl32BitMaximum (hScroll,0);
            SetControl32BitValue   (hScroll,0);
            HiliteControl (hScroll, kControlInactivePart);
        }
        MoveControl (vScroll,r.right-16,r.top-1);
        SizeControl (vScroll,16,r.bottom-r.top+2);
        t = theParent->GetMaxH();
        v = r.bottom-r.top+1+HY_SCROLLER_WIDTH;
        if (t>v) {
            if (GetControl32BitMaximum (vScroll)!=MAX_CONTROL_VALUE) {
                lastVScroll = 0;
                SetControl32BitMaximum (vScroll,MAX_CONTROL_VALUE);
            }
            HiliteControl (vScroll,0);
            newSize = MAX_CONTROL_VALUE*(_Parameter)v/(t-v);
            if (newSize>0x6fffffff) {
                newSize = 0x6fffffff;
            }
            SetControlViewSize (vScroll,(long)newSize);
        } else {
            SetControl32BitMaximum (vScroll,0);
            SetControl32BitValue   (vScroll,0);
            HiliteControl (vScroll, kControlInactivePart);
        }
    }
    rel = r;
}
//__________________________________________________________________

void        _HYPlatformComponent::_Paint (Ptr)
{
    _HYComponent* parent = (_HYComponent*)this;
    if (parent->settings.width&HY_COMPONENT_BORDER) {
        RGBColor saveColor;
        PenState sp;
        GetPenState (&sp);
        GetForeColor (&saveColor);
        RGBForeColor (&menuLine2);
        PenSize (1,1);
        MoveTo (rel.left,rel.top);
        if (parent->settings.width&HY_COMPONENT_BORDER_T) {
            LineTo (rel.right-1,rel.top);
        } else {
            MoveTo (rel.right-1,rel.top);
        }
        if (parent->settings.width&HY_COMPONENT_BORDER_R) {
            LineTo (rel.right-1,rel.bottom-1);
        } else {
            MoveTo (rel.right-1,rel.bottom-1);
        }
        if (parent->settings.width&HY_COMPONENT_BORDER_B) {
            LineTo (rel.left,rel.bottom-1);
        } else {
            MoveTo (rel.left,rel.bottom-1);
        }
        if (parent->settings.width&HY_COMPONENT_BORDER_L) {
            LineTo (rel.left,rel.top);
        }
        SetPenState (&sp);
        RGBForeColor (&saveColor);
    }
    if (parent->settings.width&HY_COMPONENT_WELL) {
        Rect rr = HYRect2Rect (rel);
        InsetRect (&rr,2,2);
        DrawThemeGenericWell (&rr,activationFlag?kThemeStateActive:kThemeStateInactive,false);
    }
    if (hScroll) {
        Draw1Control (hScroll);
    }
    if (vScroll) {
        Draw1Control (vScroll);
    }
}

//__________________________________________________________________

void      _HYPlatformComponent::_Update (Ptr p)
{
    _Paint (p);
}

//__________________________________________________________________

void        _HYPlatformComponent::_Activate (void)
{
    _HYComponent*   theParent = (_HYComponent*)this;
    if (hScroll) {
        ShowControl (hScroll);
        if (theParent->GetMaxW()>theParent->rel.right-theParent->rel.left+1+HY_SCROLLER_WIDTH) {
            HiliteControl (hScroll,kControlNoPart);
        }
    }
    if (vScroll) {
        ShowControl (vScroll);
        if (theParent->GetMaxH()>theParent->rel.bottom-theParent->rel.top+1+HY_SCROLLER_WIDTH) {
            HiliteControl (vScroll, kControlNoPart );
        }
    }
    activationFlag = true;
}

//__________________________________________________________________

void        _HYPlatformComponent::_Deactivate (void)
{
    if (hScroll) {
        HiliteControl (hScroll, kControlInactivePart);
    }
    if (vScroll) {
        HiliteControl (vScroll, kControlInactivePart);
    }

    activationFlag = false;
}

//__________________________________________________________________

bool _HYPlatformComponent::_ProcessOSEvent (Ptr vEvent)
{
    EventRecord*    theEvent = (EventRecord*)vEvent;
    WindowPtr       dummy;
    _HYComponent*   theParent = (_HYComponent*)this;
    switch (theEvent->what) {
    case mouseDown: {
        long evtType = FindWindow (theEvent->where,&dummy);
        switch (evtType) {
        case inContent: {
            Point localClick = theEvent->where;
            GlobalToLocal (&localClick);
            ControlHandle whichC;
            short f = FindControl (localClick,dummy,&whichC);
            scrollingWindow = theParent;
            if (f) {
                // set scroll step
                long    invisPixels;
                if (whichC==hScroll) {
                    invisPixels = theParent->GetMaxW()-(theParent->GetHSize());
                    hScrollingAction = true;
                } else {
                    if (whichC==vScroll) {
                        invisPixels = theParent->GetMaxH()-(theParent->GetVSize());
                        hScrollingAction = false;
                    } else {
                        return false;
                    }
                }
                scrollStepCounter = 0;
                smallScrollStep = (double)MAX_CONTROL_VALUE/invisPixels;
                if (!smallScrollStep) {
                    smallScrollStep = 1;
                }
#ifdef TARGET_API_MAC_CARBON
                ControlActionUPP myActionProc;
                myActionProc =   NewControlActionUPP(scrollAction);
#else
                UniversalProcPtr myActionProc;
                myActionProc = NewRoutineDescriptor((ProcPtr)scrollAction,
                                                    uppControlActionProcInfo,
                                                    GetCurrentISA());
#endif

                lastScrollControlValue = GetControl32BitValue (whichC);

                switch (f) {
                    /*case kControlIndicatorPart:
                    {
                        long    cv = GetControl32BitValue(whichC),cv2;
                        TrackControl (whichC,localClick,nil);
                        cv2 = GetControl32BitValue(whichC);
                        if (cv!=cv2)
                            if (hScrollingAction)
                                scrollingWindow->ProcessEvent (generateScrollEvent (cv2-cv,0));
                            else
                                scrollingWindow->ProcessEvent (generateScrollEvent (0,cv2-cv));

                        break;
                    }*/
                default: {
                    forceUpdateForScrolling = true;
#ifndef __OLDMAC__
                    TrackControl (whichC,localClick,myActionProc);
#else
#ifdef TARGET_API_MAC_CARBON
                    HandleControlClick (whichC,localClick,theEvent->modifiers,scrollAction);
#else
                    TrackControl (whichC,localClick,scrollAction);
#endif
#endif
                    forceUpdateForScrolling = false;
                }
#ifdef TARGET_API_MAC_CARBON
                DisposeControlActionUPP(myActionProc);
#endif

                }
                return true;
            }
        }
        }
    }
    }
    return false;
}

//__________________________________________________________________


_HYRect _HYPlatformComponent::_VisibleContents (Ptr p)
{
    _HYComponent* theParent = (_HYComponent*)this;
    _HYRect     * r = (_HYRect*)p,
                  res;

    short v;
    short windowW=r->right-r->left,
          windowH=r->bottom-r->top;
    if ((!hScroll)&&(!vScroll)) {
        res.left = theParent->hOrigin;
        res.right = res.left+windowW;
        res.top = theParent->vOrigin;
        res.bottom = res.top+windowH;
        return res;
    }
    if (hScroll) {
        windowH-=HY_SCROLLER_WIDTH;
    }
    if (vScroll) {
        windowW-=HY_SCROLLER_WIDTH;
    }
    if (hScroll) {
        if (windowW>theParent->GetMaxW()) {
            res.left = 0;
            res.right = theParent->GetMaxW();
        } else {
            v = GetControl32BitValue (hScroll);
            res.left = (theParent->GetMaxW()-windowW)*v/(double)MAX_CONTROL_VALUE;
            res.right = res.left+windowW;
        }
    } else {
        res.left = 0;
        res.right = windowW;
    }
    if (vScroll) {
        if (windowH>theParent->GetMaxH()) {
            res.top = 0;
            res.bottom = theParent->GetMaxH();
        } else {
            v = GetControl32BitValue (vScroll);
            res.top = (theParent->GetMaxH()-windowH)*v/(double)MAX_CONTROL_VALUE;
            res.bottom = res.top+windowH;
        }
    } else {
        res.top = 0;
        res.bottom = windowH;
    }
    return res;
}

//__________________________________________________________________

void    _HYCanvas::_Paint (Ptr p)
{
    _HYRect * destR = (_HYRect*)p;
    Rect srcRect,destRect;
    destRect.right = destR->right;
    destRect.bottom = destR->bottom;
    if (HasHScroll()) {
        destRect.bottom-=HY_SCROLLER_WIDTH;
    }
    if (HasVScroll()) {
        destRect.right-=HY_SCROLLER_WIDTH;
    }
//  if (RectInRgn (&destRect,((WindowPeek)parentWindow)->updateRgn))
    {
        _HYPlatformComponent::_Paint(p);
        destRect.left = destR->left;
        destRect.top = destR->top;
        _HYRect srcR = _VisibleContents (p);
        srcRect.right = srcR.right;
        srcRect.left = srcR.left;
        srcRect.top = srcR.top;
        srcRect.bottom = srcR.bottom;
        RGBColor         foreC,
                         backC;
        GetForeColor     (&foreC);
        GetBackColor     (&backC);
        RGBColor         white = {0xffff,0xffff,0xffff};
        RGBBackColor (&white);
        white.red = white.green = white.blue = 0;
        RGBForeColor (&white);
        LockPixels (GetGWorldPixMap(thePane));
#ifdef OPAQUE_TOOLBOX_STRUCTS
        CopyBits (GetPortBitMapForCopyBits(thePane),GetPortBitMapForCopyBits(GetWindowPort(parentWindow)),&srcRect,&destRect,srcCopy,(RgnHandle)nil);
#else
        CopyBits (&(GrafPtr(thePane)->portBits),&(parentWindow->portBits),&srcRect,&destRect,srcCopy,(RgnHandle)nil);
#endif
        UnlockPixels (GetGWorldPixMap(thePane));
        RGBForeColor     (&foreC);
        RGBBackColor     (&backC);
    }
}

//__________________________________________________________________

void    _HYCanvas::_Update (Ptr p)
{
    _Paint(p);
}

//__________________________________________________________________

_List       exportFormats;
_SimpleList exportOptions;

void        findGraphicsExporterComponents (_List&, _SimpleList&);

//__________________________________________________________________

bool _HYCanvas::_ProcessOSEvent (Ptr vEvent)
{
    if (_HYPlatformComponent::_ProcessOSEvent (vEvent)) {
        return true;
    }

    EventRecord*    theEvent = (EventRecord*)vEvent;
    WindowPtr       dummy;
    _HYComponent*   theParent = (_HYComponent*)this;
    switch (theEvent->what) {
    case mouseDown: {
        long evtType = FindWindow (theEvent->where,&dummy);
        switch (evtType) {
        case inContent: {
            if (theEvent->modifiers&controlKey) {
                if (exportFormats.lLength==0) {
                    findGraphicsExporterComponents (exportFormats,exportOptions);
                }
                _String s1 ("Save as a picture"),
                        s2 ("Save canvas as:"),
                        filePath;
                _List   menuOptions;
                menuOptions && & s1;
                long    menuChoice;
                s1 = HandlePullDown (menuOptions,theEvent->where.h,theEvent->where.v,0);
                menuChoice  = menuOptions.Find (&s1);
                if (menuChoice==0) {
                    s1 = "snapshot";
                    menuChoice = SaveFileWithPopUp (filePath, s2,s1, empty,exportFormats);
                    if (menuChoice>=0) {
                        Str255 buff;
                        ComponentInstance grexc = OpenComponent ((Component)exportOptions(menuChoice));
                        GraphicsExportSetInputGWorld (grexc,thePane);
                        FSSpec  fs;
                        StringToStr255 (filePath,buff);
                        FSMakeFSSpec(0,0,buff,&fs);
                        GraphicsExportSetOutputFile (grexc,&fs);
                        GraphicsExportRequestSettings (grexc,nil,nil);
                        unsigned long dummy;
                        OSType t,c;
                        GraphicsExportGetDefaultFileTypeAndCreator (grexc,&t,&c);
                        GraphicsExportSetOutputFileTypeAndCreator (grexc,t,c);
                        GraphicsExportDoExport (grexc,&dummy);
                        CloseComponent (grexc);
                    }

                }
                return true;
            } else {
                if ((messageRecipient)&&(doMouseClicks)) {
                    GrafPtr curPort;
                    GetPort (&curPort);
#ifdef TARGET_API_MAC_CARBON
                    SetPort (GetWindowPort (theParent->parentWindow));
#else
                    SetPort (theParent->parentWindow);
#endif
                    Point localClick = theEvent->where;
                    GlobalToLocal (&localClick);
                    SetPort (curPort);
                    messageRecipient->ProcessEvent(generateContextPopUpEvent (GetID(),localClick.h-rel.left,localClick.v-rel.top));
                    return true;
                }
            }
        }
        }
    }
    }
    return false;
}



//EOF