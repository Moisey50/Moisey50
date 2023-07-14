#pragma once
#include <Windows.h>

enum MH_MsgType
{
  MH_Error = -1 ,
  MH_Unknown = 0 ,
  MH_MouseMsg = 1 , 
  MH_Info = 3
};


class MyHook
{
public:
    //single ton
    static MyHook myHook;
 
    // function to install our mouseHook
    void InstallHook();

    // function to uninstall our mouseHook
    void UninstallHook();

    // function to "deal" with our messages
    int Messsages();

    void SetOwner( HWND hOwnerWnd , DWORD dwMsgId )
    {
      m_hOwner = hOwnerWnd ;
      m_dwMsgId = dwMsgId ;
    }

public:
    HHOOK mouseHook; // handle to the mouse hook
    HHOOK windowHook; // handle to the window hook
    MSG msg; // struct with information about all messages in our queue

    HWND m_hOwner = NULL ;
    DWORD m_dwMsgId = 0 ;
    CString m_LastMouseMsg ;
};

LRESULT CALLBACK MyMouseCallback(int nCode, WPARAM wParam, LPARAM lParam);