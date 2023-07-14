#include "pch.h"
#include "KeybMouseSimulator.h"

static void CALLBACK fn_ReadValueCallBack ( LPCTSTR Item , LPCTSTR Content ,
  KeybMouseSimulator* pSimulator )
{
}


KeybMouseSimulator::KeybMouseSimulator( HWND hOwner , DWORD dwMsgId )
{
  m_hOwner = hOwner ;
  m_dwMsgId = dwMsgId ;

  m_hSyncHandle = CreateEvent( NULL , TRUE , FALSE , NULL ) ;
  pRunnerThread = new std::thread( ScriptRunner , this ) ;
}

KeybMouseSimulator::~KeybMouseSimulator()
{
  CloseHandle( m_hSyncHandle ) ; 
  m_bAbortScript = true ;
  m_bRunnerExit = true ;

  Sleep( 50 ) ;
  if ( pRunnerThread )
  {
    if ( pRunnerThread->joinable() )
    {
      pRunnerThread->join() ;
      TRACE( "\n   Joined" ) ;
    }
    else
      TRACE( "\n   Is not joinable" ) ;
  }
  else
    TRACE( "\n   No simlator or RunnerThread" ) ;

  delete pRunnerThread ;
} ;


LONG KeybMouseSimulator::ScriptRunner( KeybMouseSimulator * pSimulator )
{
  TRACE( "\n*********************ScriptRunner entry" ) ;
  CString Script ;
  CStringArray Steps ;

  while ( !pSimulator->m_bRunnerExit )
  {
    if ( pSimulator->m_bAbortScript )
    {
      Steps.RemoveAll() ;
      pSimulator->m_bAbortScript = false ;
      pSimulator->m_bScriptIsRunning = false ;
    }
    if ( Steps.GetCount() )
    {
      CString Step = Steps[0] ;
      Step.Trim() ;
      int iPos = 0 ;
      CString Command = Step.Tokenize( " ,;" , iPos ).MakeUpper() ;

      if ( Command == "WAIT" )
      {
          // Format WAIT <Time_ms>
        CString Time_ms = Step.Tokenize( " ,;" , iPos ) ;
        int iTime_ms = atoi( Time_ms ) ;
        Sleep( iTime_ms ) ;
        Step.Insert( 0 , "Step is Finished: " ) ;
        TRACE( " %s \n" , ( LPCTSTR ) Step ) ;
        pSimulator->PostMsg( KMS_Info , Step)  ;
        
      }
      else
      {
        int iRes = pSimulator->ExecuteStep( Steps[ 0 ] ) ;
        if ( Command == "MOUSECLICK" )
          Sleep( 200 ) ;
      }
      Steps.RemoveAt( 0 ) ;
      if ( !Steps.GetCount() )
        pSimulator->m_bScriptIsRunning = false ;
      Sleep( 40 ) ;
    }
    else if ( pSimulator->m_bRunNewScript )
    {
      if ( !pSimulator->m_ScriptForLoad.IsEmpty() )
      {
        Steps.RemoveAll() ;
        int iPos = 0 ;
        CString NextString = pSimulator->m_ScriptForLoad.Tokenize( "\n" , iPos ) ;
        while ( iPos > 0 )
        {
          Steps.Add( NextString ) ;
          NextString = pSimulator->m_ScriptForLoad.Tokenize( "\n" , iPos ) ;
        }
        pSimulator->m_bRunNewScript = false ;
        pSimulator->m_bScriptIsRunning = true ;
        Sleep( 500 ) ;
      }
    }
    else
      Sleep( 10 ) ;
  }
  TRACE( "\n*********************ScriptRunner exit" ) ;

  return 0 ;
}

int KeybMouseSimulator::PostMsg( KMS_MsgType Type , LPCTSTR pMsgText )
{
  LPCSTR p = _tcsdup( pMsgText ) ;
  PostMessage( m_hOwner , m_dwMsgId , Type , ( LPARAM ) p ) ;
  return 1 ;
}

int KeybMouseSimulator::ExecuteStep( CString Step )
{
  TRACE( "----------- Step: %s\n" , (LPCTSTR)Step ) ;

  Step.Trim() ;
  
  ResetEvent( m_hSyncHandle ) ;

  PostMsg( KMS_DoStep , Step ) ;

  DWORD dwRes = WaitForSingleObject( m_hSyncHandle , 10000 ) ;
  
  if ( dwRes != WAIT_OBJECT_0 )
  {
    return -1 ;  
  }
  return 1 ;
  
  if ( Step.Find("//") == 0 ) // comment
  {
    m_LastStatusMsg = Step ;
    PostMsg( KMS_Info , m_LastStatusMsg ) ;
    return 0 ;
  }

  int iPos = 0 ; 
  CString Command = Step.Tokenize( " ,;" , iPos ).MakeUpper() ;

  if ( Command == "START" )
  {
      // Format: START <full path to application>
    CString Application = Step.Mid( iPos ) ;
    system( Application ) ;
    Sleep( 800 ) ;
    m_LastStatusMsg.Format( "Application %s started" , (LPCTSTR) Application ) ;
    TRACE( " %s\n" , ( LPCTSTR ) m_LastStatusMsg ) ;
    PostMsg( KMS_Info , m_LastStatusMsg ) ;

    return 1 ;
  }
  else if ( Command == "FINDWINDOW" )
  {
      // Format: FINDWINDOW <Window Caption>
    CString WindowCaption = Step.Mid( iPos ).Trim( " ,;" ) ;
    m_hSelectedWindow = FindWindow( NULL , WindowCaption ) ;
    if ( m_hSelectedWindow )
    {
      GetWindowRect( m_hSelectedWindow , &m_CurrentWindowRect ) ;
      m_LastStatusMsg.Format( "Window %s found, h=%p, Rect=%d,%d,%d,%d" , ( LPCTSTR ) WindowCaption ,
        m_hSelectedWindow ,
        m_CurrentWindowRect.left , m_CurrentWindowRect.top ,
        m_CurrentWindowRect.right , m_CurrentWindowRect.bottom ) ;
      TRACE( " %s\n" , (LPCTSTR) m_LastStatusMsg ) ;
      PostMsg( KMS_Info , m_LastStatusMsg ) ;
      return 1 ;
    }
    else
    {
      m_LastStatusMsg.Format( "Can't find window '%s'" , (LPCTSTR)WindowCaption ) ;
      PostMsg( KMS_Error , m_LastStatusMsg ) ;
      return -1 ;
    }
  }
  else if ( Command == "MOUSECLICK" )
  {
    // Format: MOUSECLICK ABSOLUTE-or-RELATIVE (X,Y)
    // For Absolute - X and Y are integer
    // For Relative - X and Y are part of selected window size; range[0.0,1.0]
    CString Mode = Step.Tokenize( " ,;" , iPos ).MakeUpper() ;
    CString CoordinatesAsText = Step.Mid(iPos).Trim() ;
    CoordinatesAsText.Remove(' ') ;

    if ( CoordinatesAsText[0] == '(' )
      CoordinatesAsText.Delete( 0 ) ;
    double dX = atof( CoordinatesAsText ) ;
    int iCommaPos = CoordinatesAsText.Find(',') ;
    if ( iCommaPos > 0 )
    {
      double dY = atof( ((LPCTSTR)CoordinatesAsText) + iCommaPos + 1 ) ;
      ClipCursor( NULL ) ;
      Sleep(20) ;
      if ( Mode == "RELATIVE" )
      {
        CPoint CursorPos ;
        GetCursorPos( &CursorPos ) ;
        double dAbsX = m_CurrentWindowRect.left + /*m_CurrentWindowRect.Width() * */dX ;
        double dAbsY = m_CurrentWindowRect.top + /*m_CurrentWindowRect.Height() * */dY ;
        double fScreenWidth = ::GetSystemMetrics( SM_CXSCREEN ) - 1;
        double fScreenHeight = ::GetSystemMetrics( SM_CYSCREEN ) - 1;
        double fx = dAbsX * ( 65535.0 / fScreenWidth );
        double fy = dAbsY * ( 65535.0 / fScreenHeight );

        int iDx = ( int ) ( ( dAbsX - CursorPos.x ) / 10. ) ;
        int iDy = ( int ) ( ( dAbsY - CursorPos.y ) / 10. ) ;
        INPUT ForClick[ 3 ] ;
        ::ZeroMemory( ForClick , sizeof(ForClick) ) ;
//         ForClick[ 0 ].type = INPUT_MOUSE;
//         ForClick[ 0 ].mi.dx = ( long ) fx ; // desired X coordinate
//         ForClick[ 0 ].mi.dy = ( long ) fy ; // desired Y coordinate
//         ForClick[ 0 ].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
        ForClick[ 0 ].type = INPUT_MOUSE;
        ForClick[ 0 ].mi.dx = iDx ; // desired X coordinate
        ForClick[ 0 ].mi.dy = iDy ; // desired Y coordinate
        ForClick[ 0 ].mi.dwFlags = MOUSEEVENTF_MOVE;
        SendInput( 1 , ForClick , sizeof( INPUT ) );
        Sleep(100) ;
        ForClick[ 1 ].type = INPUT_MOUSE;
        ForClick[ 1 ].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput( 1 , ForClick + 1, sizeof( INPUT ) );
        Sleep( 100 ) ;

        ForClick[ 2 ].type = INPUT_MOUSE;
        ForClick[ 2 ].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput( 1 , ForClick + 2 , sizeof( INPUT ) );
        Sleep( 100 ) ;

//         SendInput( 3 , ForClick , sizeof( INPUT ) );
//         mouse_event( MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE , (DWORD)fx , (DWORD)fy , 0 , 0 ) ;
//         Sleep( 100 ) ;
//         mouse_event( MOUSEEVENTF_LEFTDOWN , ( DWORD ) fx , ( DWORD ) fy , 0 , 0 ) ;
//         Sleep( 100 ) ;
//         mouse_event( MOUSEEVENTF_LEFTUP , ( DWORD ) fx , ( DWORD ) fy , 0 , 0 ) ;

        CPoint PtReadCursor ;
        GetCursorPos( &PtReadCursor ) ;
        m_LastStatusMsg.Format( "  [%d,%d] h=%p Cur[%d,%d]" , ( int ) dAbsX , ( int ) dAbsY ,
          m_hSelectedWindow , PtReadCursor.x , PtReadCursor.y ) ;
//         CPoint Pt( (int)dX , (int)dY ) ;
// 
// 
// //         HWND hWnd = WindowFromPoint( Pt ) ;
// //         if ( hWnd )
//         if( m_hSelectedWindow )
//         {
//           ClipCursor( NULL ) ;
//           HWND hWnd = WindowFromPoint( Pt ) ;
//           if ( hWnd )
//             PostMessage( hWnd , WM_LBUTTONDOWN , MK_LBUTTON , ( Pt.x & 0xffff ) + ( ( Pt.y & 0xffff ) << 16 ) ) ;
//           Sleep( 500 ) ;
//           PostMessage( hWnd , WM_LBUTTONUP , 0 , ( Pt.x & 0xffff ) + ( ( Pt.y & 0xffff ) << 16 ) ) ;
//           m_LastStatusMsg.Format( "  [%d,%d] h=%p h2=%p" , ( int ) dAbsX , ( int ) dAbsY ,
//             m_hSelectedWindow , hWnd ) ;
//         }
//         else
//         {
//           m_LastStatusMsg.Format( "  [%d,%d] h=%p h2=%p" , ( int ) dAbsX , ( int ) dAbsY ,
//             m_hSelectedWindow , 0 ) ;
//         }
        m_LastStatusMsg.Insert( 0 , Step) ;
        TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
//         m_LastStatusMsg.Insert( 0 , "Finished Command " ) ;
        PostMsg( KMS_Info , m_LastStatusMsg ) ;
        return 1 ;
      }
      else if ( Mode == "ABSOLUTE" )
      {
//         INPUT ForClick[3] ;
//         ForClick[ 0 ].type = INPUT_MOUSE;
//         ForClick[ 0 ].mi.dx = ( long ) dX ; // desired X coordinate
//         ForClick[ 0 ].mi.dy = ( long ) dY ; // desired Y coordinate
//         ForClick[ 0 ].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
// 
//         ForClick[ 1 ].type = INPUT_MOUSE;
//         ForClick[ 1 ].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
//         ForClick[ 0 ].mi.dx = ( long ) dX ; // desired X coordinate
//         ForClick[ 0 ].mi.dy = ( long ) dY ; // desired Y coordinate
// 
//         ForClick[ 2 ].type = INPUT_MOUSE;
//         ForClick[ 2 ].mi.dwFlags = MOUSEEVENTF_LEFTUP;
//         ForClick[ 0 ].mi.dx = ( long ) dX ; // desired X coordinate
//         ForClick[ 0 ].mi.dy = ( long ) dY ; // desired Y coordinate
// 
//         SendInput( 3 , ForClick , sizeof( INPUT ) );

        ClipCursor( NULL ) ;
        CPoint Pt( ( int ) dX , ( int ) dY ) ;
        SetCursorPos( Pt.x , Pt.y ) ;
        HWND hWnd = WindowFromPoint( Pt ) ;
        if ( hWnd )
        {
          PostMessage( hWnd , WM_LBUTTONDOWN , MK_LBUTTON , ( Pt.x & 0xffff ) + ( ( Pt.y & 0xffff ) << 16 ) ) ;
          Sleep( 200 ) ;
          PostMessage( hWnd , WM_LBUTTONUP , 0 , ( Pt.x & 0xffff ) + ( ( Pt.y & 0xffff ) << 16 ) ) ;
        }
        m_LastStatusMsg.Format( "  [%d,%d] h=%p" , ( int ) dX , ( int ) dY , hWnd ) ;
        m_LastStatusMsg.Insert( 0 , Step ) ;
        TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
//         m_LastStatusMsg.Insert( 0 , "Finished Command " ) ;
        PostMsg( KMS_Info , m_LastStatusMsg ) ;
        return 1 ;
      }
      else
      {
        m_LastStatusMsg.Format( "Bad MouseClick command format: '%s'" , ( LPCTSTR ) Step ) ;
        PostMsg( KMS_Error , m_LastStatusMsg ) ;
        TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
      }
    }
    else
    {
      m_LastStatusMsg.Format( "Bad MouseClick coordinates: '%s'" , ( LPCTSTR ) Step ) ;
      PostMsg( KMS_Error , m_LastStatusMsg ) ;
      TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
    }
    return -1 ;
  }
  else if ( Command == "GETTEXT" )
  {
    // Format: GETTEXT (X,Y) - always coordinates inside selected window
    CString CoordinatesAsText = Step.Mid( iPos ).Trim() ;
    CoordinatesAsText.Remove( ' ' ) ;

    if ( CoordinatesAsText[ 0 ] == '(' )
      CoordinatesAsText.Delete( 0 ) ;
    double dX = atof( CoordinatesAsText ) ;
    int iCommaPos = CoordinatesAsText.Find( ',' ) ;
    if ( iCommaPos > 0 )
    {
      double dY = atof( ( ( LPCTSTR ) CoordinatesAsText ) + iCommaPos + 1 ) ;
      dX = m_CurrentWindowRect.left + m_CurrentWindowRect.Width() * dX ;
      dY = m_CurrentWindowRect.top + m_CurrentWindowRect.Height() * dY ;
      CPoint Pt( (int)dX , (int)dY ) ;
      HWND hSelectedControl = ChildWindowFromPoint( m_hSelectedWindow , Pt ) ;
      if (  hSelectedControl )
      {
        TCHAR String[500] ;
        int iLen = GetWindowText( hSelectedControl , String , 500 ) ;
        if ( iLen )
        {
          m_LastStatusMsg = String ;
          PostMessage( m_hOwner , m_dwMsgId , KMS_ReadTextResult , 
            ( WPARAM ) ( LPCTSTR ) m_LastStatusMsg ) ;
          TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
          return 1 ;
        }
        else
        {
          DWORD dwCode = GetLastError() ;
          if ( dwCode )
          {
            m_LastStatusMsg.Format( "Read Text Error %d on %s" , dwCode , (LPCTSTR)Step ) ;
            PostMessage( m_hOwner , m_dwMsgId , KMS_Error , ( LPARAM ) ( LPCTSTR ) m_LastStatusMsg ) ;
            TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
          }
          else
          {
            m_LastStatusMsg = "  ";
            PostMessage( m_hOwner , m_dwMsgId , KMS_Error , ( LPARAM ) ( LPCTSTR ) m_LastStatusMsg ) ;
            TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
            return 1 ;
          }
        }
      }
      else
      {
        m_LastStatusMsg.Format( "No Window for %s" , ( LPCTSTR ) Step ) ;
        PostMessage( m_hOwner , m_dwMsgId , KMS_Error , ( LPARAM ) ( LPCTSTR ) m_LastStatusMsg ) ;
        TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
      }
    }
    else
    {
      m_LastStatusMsg.Format( "Bad command format: '%s'" , ( LPCTSTR ) Step ) ;
      PostMsg( KMS_Error , m_LastStatusMsg ) ;
    }
    return -1 ;
  }
  else if ( Command == "SETTEXT" )
  {

  }
  else if ( Command == "WAIT" )
  {
      // Format WAIT <Time_ms>
    CString Time_ms = Step.Tokenize( " ,;" , iPos ) ;
    int iTime_ms = atoi( Time_ms ) ;
    Sleep( iTime_ms ) ;
    m_LastStatusMsg = "Step is Finished: " ;
    m_LastStatusMsg += Step ;
    TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
    return 1 ;
  }
  return 0;
}


int KeybMouseSimulator::SetOwner( HWND hOwnerWnd , DWORD dwMsgId )
{
  ASSERT ( pRunnerThread ) ;

  m_hOwner = hOwnerWnd ;
  m_dwMsgId = dwMsgId ;

  return 0;
}


int KeybMouseSimulator::StartScript( LPCTSTR pScript )
{
  if ( !m_bScriptIsRunning && !m_bRunNewScript )
  {
    m_ScriptForLoad = pScript ;
    m_bRunNewScript = true ;
  }
  return 0;
}


int KeybMouseSimulator::AbortScript()
{
  m_bAbortScript = true ;
  return 0;
}
