#include "pch.h"
#include "Hook.h"

#include <stdio.h>

MyHook g_MouseHook ;

void MyHook::InstallHook()
{
    /*
    SetWindowHookEx(
    WM_MOUSE_LL = mouse low level mouseHook type,
    MyMouseCallback = our callback function that will deal with system messages about mouse
    NULL, 0);

    c++ note: we can check the return SetWindowsHookEx like this because:
    If it return NULL, a NULL value is 0 and 0 is false.
    */
  if ( !( mouseHook = SetWindowsHookEx( WH_MOUSE_LL , MyMouseCallback , NULL , 0 ) ) )
  {
    m_LastMouseMsg = "Error on mouse hook install" ;
    PostMessage( m_hOwner , m_dwMsgId , MH_Error , ( LPARAM ) ( LPCTSTR ) m_LastMouseMsg ) ;
  }
}

// function to uninstall our mouseHook
void MyHook::UninstallHook()
{
  UnhookWindowsHookEx( mouseHook );
}

MSG msg; // struct with information about all messages in our queue

// function to "deal" with our messages
int MyHook::Messsages()
{
    // while we do not close our application
  while ( msg.message != WM_QUIT )
  {
    if ( GetMessage( &msg , NULL , 0 , 0/*, PM_REMOVE*/ ) )
    {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
    }
    Sleep( 1 );
  }
  UninstallHook(); // if we close, let's uninstall our mouseHook
  return ( int ) msg.wParam; // return the messages
}

LRESULT CALLBACK MyMouseCallback( int nCode , WPARAM wParam , LPARAM lParam )
{
  MSLLHOOKSTRUCT* pMouseStruct = ( MSLLHOOKSTRUCT* ) lParam; // WH_MOUSE_LL struct
  if ( nCode == 0 )
  {
    switch ( wParam )
    {
      case WM_LBUTTONDOWN:
      {
        HWND hWnd = WindowFromPoint( pMouseStruct->pt ) ;
        g_MouseHook.m_LastMouseMsg.Format( "MOUSE_LBD (%d,%d) h=%p" , 
          pMouseStruct->pt.x , pMouseStruct->pt.y , hWnd ) ;
        PostMessage( g_MouseHook.m_hOwner , g_MouseHook.m_dwMsgId ,
          MH_MouseMsg , ( LPARAM ) ( LPCTSTR ) g_MouseHook.m_LastMouseMsg ) ;
      }
      break;
      case WM_LBUTTONUP:
      {
        HWND hWnd = WindowFromPoint( pMouseStruct->pt ) ;
        g_MouseHook.m_LastMouseMsg.Format( "MOUSE_LBU (%d,%d) h=%p" ,
          pMouseStruct->pt.x , pMouseStruct->pt.y , hWnd ) ;
        PostMessage( g_MouseHook.m_hOwner , g_MouseHook.m_dwMsgId ,
          MH_MouseMsg , ( LPARAM ) ( LPCTSTR ) g_MouseHook.m_LastMouseMsg ) ;
      }
      break;
      case WM_RBUTTONDOWN:
      {
        HWND hWnd = WindowFromPoint( pMouseStruct->pt ) ;
        g_MouseHook.m_LastMouseMsg.Format( "MOUSE_RBD (%d,%d) h=%p" ,
          pMouseStruct->pt.x , pMouseStruct->pt.y , hWnd ) ;
        PostMessage( g_MouseHook.m_hOwner , g_MouseHook.m_dwMsgId ,
          MH_MouseMsg , ( LPARAM ) ( LPCTSTR ) g_MouseHook.m_LastMouseMsg ) ;
      }
      break;
      case WM_RBUTTONUP:
      {
        HWND hWnd = WindowFromPoint( pMouseStruct->pt ) ;
        g_MouseHook.m_LastMouseMsg.Format( "MOUSE_RBU (%d,%d) h=%p" ,
          pMouseStruct->pt.x , pMouseStruct->pt.y , hWnd ) ;
        PostMessage( g_MouseHook.m_hOwner , g_MouseHook.m_dwMsgId ,
          MH_MouseMsg , ( LPARAM ) ( LPCTSTR ) g_MouseHook.m_LastMouseMsg ) ;
      }
      break;
    }
  }
  return CallNextHookEx( g_MouseHook.mouseHook , nCode , wParam , lParam );
}
