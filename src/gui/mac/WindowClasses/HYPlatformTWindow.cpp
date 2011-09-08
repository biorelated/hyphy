/*
    Mac OS Portions of the container window

    Sergei L. Kosakovsky Pond, Spring 2000 - December 2002.
*/

#include "HYTableWindow.h"
#include "Scrap.h"
#include "HYEventTypes.h"
#include "HYUtils.h"
#include "errorfns.h"


//__________________________________________________________________
extern      _SimpleList windowPtrs,
            windowObjects;

RGBColor    _BLACK_ = {0,0,0};

extern      PixPatHandle
statusBarFill;

//__________________________________________________________________


#ifdef TARGET_API_MAC_CARBON
extern  PMPageFormat     gPageFormat;
extern  Handle           gFlattenedFormat;
pascal OSStatus scrollWheelHandler (EventHandlerCallRef , EventRef, void*);
pascal void IdleWindowTimer        (EventLoopTimerRef ,void*);


pascal void IdleWindowTimer (EventLoopTimerRef ,void* userData)
{
    _HYTWindow * myTW = (_HYTWindow*)userData;
    for (long k=0; k<myTW->components.lLength; k++)
        if (myTW->cells.Find(k)>=0) {
            _HYComponent* tC = (_HYComponent*)myTW->components(k);
            tC->IdleHandler();
        }
    myTW->_HandleIdleEvent ();
}

//__________________________________________________________________

pascal OSStatus scrollWheelHandler (EventHandlerCallRef , EventRef theEvent, void* userData)
{
    EventParamType                  actType;
    EventMouseWheelAxis             axis;
    GetEventParameter (theEvent,  kEventParamMouseWheelAxis, typeMouseWheelAxis, &actType,sizeof(EventMouseWheelAxis),nil,&axis);

    if (axis == kEventMouseWheelAxisY) {
        Point              mouseLocation;
        GetEventParameter (theEvent, kEventParamMouseLocation, typeQDPoint, &actType,sizeof(Point),nil,&mouseLocation);

        long               mouseWheelDelta;
        GetEventParameter (theEvent, kEventParamMouseWheelDelta, typeLongInteger, &actType,sizeof(Point),nil,&mouseWheelDelta);

        UInt32 modifiers;
        GetEventParameter (theEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(modifiers), NULL, &modifiers);

        if (modifiers & optionKey) {
            mouseWheelDelta *= 10;
        }

        /*_HYWindow* thisWindow = (_HYWindow*)windowObjectRefs(windowPtrs.Find((long)userData));
        forceUpdateForScrolling = true;
        thisWindow->SetWindowRectangle (newSize.top,newSize.left,newSize.bottom+1,newSize.right+1,false);
        thisWindow->Update(nil);
        forceUpdateForScrolling = false;*/

        _HYTWindow* thisWindow = (_HYTWindow*)windowObjectRefs(windowPtrs.Find((long)userData));
        GrafPtr savedPort;
        GetPort(&savedPort);
#ifdef OPAQUE_TOOLBOX_STRUCTS
        SetPort(GetWindowPort(thisWindow->theWindow));
#else
        SetPort(thisWindow->theWindow);
#endif
        GlobalToLocal (&mouseLocation);
        int c = thisWindow->FindClickedCell(mouseLocation.h,mouseLocation.v);
        thisWindow->DoMouseWheel (c, mouseWheelDelta);
        return noErr;
    }

    return eventNotHandledErr;
}

#endif

//__________________________________________________________________


bool        _HYTWindow::_ProcessMenuSelection (long msel)
{
    if (_HYWindow::_ProcessMenuSelection(msel)) {
        return true;
    }

    long        menuChoice = msel&0x0000ffff;
    bool        done = false;

    switch (msel/0xffff) {
    case 129:
        if (menuChoice == 7) { // print setup
            OSStatus theStatus;
            Boolean isAccepted;

            PMPrintSession hyPC;

            theStatus = PMCreateSession (&hyPC);
            if (theStatus != noErr) {
                return false;
            }

            if (InitPrint(hyPC)) {
                theStatus = PMSessionPageSetupDialog(hyPC,gPageFormat, &isAccepted);
            }

            if (theStatus == noErr) {
                if (gFlattenedFormat != NULL) {
                    DisposeHandle(gFlattenedFormat);
                    gFlattenedFormat = NULL;
                }

                theStatus = PMFlattenPageFormat(gPageFormat, &gFlattenedFormat);
            }

            if (gPageFormat != kPMNoPageFormat) {
                theStatus = PMRelease(gPageFormat);
                gPageFormat = kPMNoPageFormat;
            }

            theStatus = PMRelease(hyPC);
            return true;
        }

    case 130: {
        if ((menuChoice == 4)||(menuChoice==5)) {
            done = true;
            HandleCopyPaste(menuChoice-4);
            break;
        }
    }
    }

    HiliteMenu(0);
    InvalMenuBar();

    return done;
}

//__________________________________________________________________

long _HYTWindow::_Grow(Ptr theData)
{
    EventRecord* theEvent = (EventRecord*)theData;
    //_HYRect        dynDim = MinMaxWindowDimensions();
    Rect         sizeRect;
    sizeRect.top    = dim.top;
    sizeRect.left   = dim.left;
    sizeRect.bottom = dim.bottom-1;
    sizeRect.right  = dim.right-1;

    return GrowWindow  (theWindow,theEvent->where,&sizeRect);
}

//__________________________________________________________________

void _HYTWindow::_PaintStatusBar(Ptr,bool)
{
    Rect clearRect = newHRect();
    //EraseRect (&clearRect);
    FillCRect (&clearRect, statusBarFill);
    if (hScroll) {
        clearRect = newVRect();
        EraseRect (&clearRect);
    } else {
        RGBColor saveColor;
        GetForeColor (&saveColor);
        RGBForeColor (&_BLACK_);
        MoveTo (clearRect.left,clearRect.top);
        LineTo (clearRect.right,clearRect.top);
        RGBForeColor (&saveColor);
        if (statusBar.sLength) {
#ifdef OPAQUE_TOOLBOX_STRUCTS
            Rect    destRect;
            GetWindowBounds (theWindow,kWindowGlobalPortRgn,&destRect);
            OffsetRect (&destRect,-destRect.left,-destRect.top);
            if (flags&HY_WINDOW_STATUS_BAR_LIGHT_LEFT) {
                MoveTo (20,destRect.bottom-4);
            } else {
                MoveTo (5,destRect.bottom-4);
            }
#else
            if (flags&HY_WINDOW_STATUS_BAR_LIGHT_LEFT) {
                MoveTo (20,theWindow->portRect.bottom-4);
            } else {
                MoveTo (5,theWindow->portRect.bottom-4);
            }
#endif
            TextFont(0);
            TextFace(0);
            TextSize(9);
            DrawText(statusBar.sData,0,statusBar.sLength);
        }
    }
}

//__________________________________________________________________

void _HYTWindow::_Paint(Ptr)
{
    /*if (flags&HY_WINDOW_SIZE)
        DrawGrowIcon (theWindow);*/
    _HYRect  relRect;

    _SimpleList alreadyDone (cells.lLength);

    for (int k=0; k<cells.lLength; k++) {
        long i = cells.lData[k];

        if (alreadyDone.lData[i] == 0) {
            relRect.left = componentL.lData[i];
            relRect.right = componentR.lData[i];
            relRect.top = componentT.lData[i];
            relRect.bottom = componentB.lData[i];
            ((_HYComponent*)components(i))->Update((Ptr)&relRect);
            alreadyDone.lData[i] = 1;
        }
    }
    if (hScroll) {
        ShowControl (hScroll);
        ShowControl (vScroll);
    } else {
        if(flags&HY_WINDOW_SIZE) {
            _PaintStatusBar();
        }
    }

#ifdef TARGET_API_MAC_CARBON
    if (!aquaInterfaceOn) {
        DrawControls(theWindow);
    }
#else
    DrawControls(theWindow);
#endif
}

//__________________________________________________________________

void _HYTWindow::_Update(Ptr)
{
    GrafPtr savedPort;
    GetPort(&savedPort);
#ifdef OPAQUE_TOOLBOX_STRUCTS
    SetPort(GetWindowPort(theWindow));
#else
    SetPort(theWindow);
#endif
    BeginUpdate(theWindow);
//  PixMapHandle osW,saveMap;
//  SetPort(theWindow);
//  GDHandle     junk;
//  if (NewScreenBuffer (&theWindow->portRect,false,&junk,&osW) != noErr)
//  {
    _Paint(nil);
    /*  }
        else
        {
            HLock ((Handle)osW);
            HLock ((Handle)((CGrafPtr)theWindow)->portPixMap);
            CopyBits ((BitMap*)*(((CGrafPtr)theWindow)->portPixMap),(BitMap*)*osW,&theWindow->portRect,&theWindow->portRect,srcCopy,nil);
            saveMap = ((CGrafPtr)theWindow)->portPixMap;
            SetPortPix (osW);
            _Paint(nil);
            SetPortPix (saveMap);
            CopyBits ((BitMap*)*osW,(BitMap*)*(((CGrafPtr)theWindow)->portPixMap),&theWindow->portRect,&theWindow->portRect,srcCopy,nil);
            HUnlock ((Handle)((CGrafPtr)theWindow)->portPixMap);
            DisposePixMap (osW);
        }*/
    EndUpdate(theWindow);
    SetPort(savedPort);
}

//__________________________________________________________________

void _HYTWindow::_Activate(void)
{
    for (int i=0; i<components.lLength; i++)
        if (cells.Find(i)>=0) {
            ((_HYComponent*)components(i))->Activate();
        }


#ifdef TARGET_API_MAC_CARBON
    if (!theTimer) {
        EventLoopRef      mainLoop;
        mainLoop = GetMainEventLoop();
        InstallEventLoopTimer (mainLoop,0,.5*kEventDurationSecond,timerUPP,this,&theTimer);
    }
#endif

    _HYPlatformWindow::_Activate();

}

//__________________________________________________________________

void _HYTWindow::_Activate2(void)
{
}

//__________________________________________________________________

void _HYTWindow::_Deactivate2(void)
{
}

//__________________________________________________________________

void _HYTWindow::_Zoom(bool inOut)
{
    if ((((savedLoc.right-savedLoc.left!=right-left)||(savedLoc.bottom-savedLoc.top!=bottom-left)))&&(!inOut)) {
        SetPosition (savedLoc.left,savedLoc.top);
        SetWindowRectangle (0,0,savedLoc.bottom-savedLoc.top, savedLoc.right-savedLoc.left);
    } else {
        Rect    sl = savedLoc;
        _HYRect sd = GetScreenDimensions();
        SetPosition (5,42);
        SetWindowRectangle (0,0, sd.bottom-25, sd.right-5);
        savedLoc = sl;
    }
}

//__________________________________________________________________

void _HYTWindow::_Deactivate(void)
{
    for (int i=0; i<components.lLength; i++)
        if (cells.Find(i)>=0) {
            ((_HYComponent*)components(i))->Deactivate();
        }

#ifdef TARGET_API_MAC_CARBON
    if (theTimer) {
        RemoveEventLoopTimer (theTimer);
        theTimer = nil;
    }
#endif
    _HYPlatformWindow::_Deactivate();
}

//__________________________________________________________________
void        _HYTWindow::_SetStatusBar(_String& text)
{
    statusBar = text;
    Rect statusRect = newHRect();
    GrafPtr savePort;
    GetPort (&savePort);
#ifdef OPAQUE_TOOLBOX_STRUCTS
    SetPort(GetWindowPort(theWindow));
    InvalWindowRect (theWindow,&statusRect);
#else
    SetPort(theWindow);
    InvalRect (&statusRect);
#endif
    SetPort (savePort);
}


//__________________________________________________________________
void        _HYPlatformTWindow::_SetWindowRectangle     (int,int,int,int,bool)
{

}


//__________________________________________________________________

bool _HYTWindow::_ProcessOSEvent (Ptr vEvent)
{
    EventRecord* theEvent = (EventRecord*)vEvent;
    if (!_HYPlatformWindow::_ProcessOSEvent (vEvent)) {
        if ((theEvent->what==keyDown)||(theEvent->what==autoKey)) {
            if (keyboardFocusChain.lLength) {
                int keyCode = (theEvent->message&keyCodeMask)>>8;
                if (keyCode == 0x30) { // tab
                    bool    backwards = theEvent->modifiers & shiftKey;

                    if (keyboardFocus==-1) {
                        keyCode = keyboardFocusChain.lData[backwards?keyboardFocusChain.lLength-1:0];
                    } else if (keyboardFocusChain.lLength>1) {
                        keyCode = keyboardFocusChain.Find (keyboardFocus);
                        keyCode += backwards?(-1):1;
                        if (keyCode<0) {
                            keyCode = keyboardFocusChain.lLength-1;
                        } else if (keyCode >= keyboardFocusChain.lLength) {
                            keyCode = 0;
                        }
                        keyCode = keyboardFocusChain.lData[keyCode];
                    } else {
                        keyCode = -1;
                    }

                    if (keyCode>=0) {
                        ProcessEvent (generateKeyboardFocusEvent (((_HYComponent*)components(keyCode))->GetID()));
                    }
                    return true;
                }
            }

            for (long k=0; k<components.lLength; k++)
                if (cells.Find(k)>=0) {
                    _HYComponent* tC = (_HYComponent*)components(k);
                    if (tC->UnfocusedKeyboardInput())
                        if (tC->_ProcessOSEvent (vEvent)) {
                            return true;
                        }
                }

            if ((keyboardFocus>=0)&&(keyboardFocus<components.lLength))
                if (((_HYComponent*)components(keyboardFocus))->_ProcessOSEvent (vEvent)) {
                    return true;
                }

            return false;
        }
        GrafPtr savedPort;
        GetPort(&savedPort);
#ifdef OPAQUE_TOOLBOX_STRUCTS
        SetPort(GetWindowPort(theWindow));
#else
        SetPort(theWindow);
#endif
        Point localClick = theEvent->where;
        GlobalToLocal (&localClick);
        int c = FindClickedCell(localClick.h,localClick.v);
        if (c<0) {
            if (lastMouseComponent>=0) {
                ((_HYComponent*)components(lastMouseComponent))->_ComponentMouseExit();
            }
            lastMouseComponent = -1;
            return false;
        } else {
            if ((lastMouseComponent>=0)&&(c!=lastMouseComponent)) {
                ((_HYComponent*)components(lastMouseComponent))->_ComponentMouseExit();
            }
            lastMouseComponent = c;
        }
        bool res = ((_HYComponent*)components(c))->_ProcessOSEvent (vEvent);
        if ((theEvent->what == osEvt)||(theEvent->what == nullEvent))
            for (long k=0; k<components.lLength; k++)
                if (cells.Find(k)>=0) {
                    _HYComponent* tC = (_HYComponent*)components(k);
                    tC->IdleHandler();
                }
        SetPort(savedPort);
        return res;
    } else {
        return true;
    }
    return false;
}

//__________________________________________________________________

void        _HYTWindow::_SetCopyString (_String* str)
{
#ifdef TARGET_API_MAC_CARBON
    ClearCurrentScrap();
    ScrapRef         theScrapRef;
    GetCurrentScrap(&theScrapRef);
    PutScrapFlavor(theScrapRef, 'TEXT', kScrapFlavorMaskNone,str->sLength,str->sData);
#else
    ZeroScrap();
    PutScrap (str->sLength,'TEXT',str->sData);
#endif
}

//__________________________________________________________________

_String*        _HYTWindow::_GetPasteString (void)
{
    Handle  scrapHandle = NewHandle (0);
    long    rc;
    _String *res = nil;

#ifdef TARGET_API_MAC_CARBON
    ScrapRef theScrapRef;
    if (GetCurrentScrap(&theScrapRef) != noErr) {
        return new _String;
    }

    if (GetScrapFlavorSize(theScrapRef, 'TEXT', &rc) != noErr) {
        return new _String;
    }
#else
    long    scrapOffset;
    rc = GetScrap( scrapHandle, 'TEXT', &scrapOffset );
#endif
    if ( rc >= 0 ) {
        SetHandleSize( scrapHandle, rc+1 );
        HLock  (scrapHandle);
#ifdef TARGET_API_MAC_CARBON
        long err = GetScrapFlavorData(theScrapRef, 'TEXT', &rc, *scrapHandle);
#endif
        (*scrapHandle)[rc] = 0;
        HUnlock (scrapHandle);
        if (err == noErr) {
            res = new _String (*scrapHandle);
        } else {
            res = new _String;
        }
    } else {
        res = new _String;
    }

    DisposeHandle (scrapHandle);
    return res;
}

//__________________________________________________________________

_HYPlatformTWindow::_HYPlatformTWindow(Ptr)
{
#ifdef TARGET_API_MAC_CARBON
    theTimer = nil;
    timerUPP = NewEventLoopTimerUPP(IdleWindowTimer);

    scrollWheelH    = NewEventHandlerUPP (scrollWheelHandler);
    checkPointer  ((Ptr)scrollWheelH);
    EventTypeSpec sw;
    sw.eventClass  = kEventClassMouse;
    sw.eventKind   = kEventMouseWheelMoved;
    InstallWindowEventHandler (((_HYTWindow*)this)->theWindow,scrollWheelH,1,&sw,(Ptr)(((_HYTWindow*)this)->theWindow),NULL);
#endif
}

//__________________________________________________________________

_HYPlatformTWindow::~_HYPlatformTWindow(void)
{
#ifdef TARGET_API_MAC_CARBON
    if (theTimer) {
        RemoveEventLoopTimer (theTimer);
    }
    DisposeEventLoopTimerUPP (timerUPP);
    if (scrollWheelH) {
        DisposeEventHandlerUPP (scrollWheelH);
    }
#endif
}




//EOF