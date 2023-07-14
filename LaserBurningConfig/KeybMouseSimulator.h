#pragma once
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
#include <utility>

using namespace std ;


__forceinline bool ConvToBinary( LPCTSTR strIn , int& iResult )
{
  CString s( strIn ); s.Trim() ;
  if ( s[ 0 ] == _T( '(' )
    || s[ 0 ] == _T( '[' )
    || s[ 0 ] == _T( '{' ) )
    s.Delete( 0 ) ;
  if ( s[ 0 ] == _T( '0' ) )
  {
    if ( _totlower( s[ 1 ] ) == _T( 'x' ) )
    {
      int dwRes ;
      s = s.Mid( 2 ) ;
#ifdef _WIN64
      if ( _stscanf_s( s , _T( "%llx" ) , &dwRes ) )
#else
      if ( _stscanf_s( s , _T( "%x" ) , &dwRes ) )
#endif
      {
        iResult = dwRes ;
        return true ;
      }
      return false ;
    }
    else if ( _totlower( s[ 1 ] ) == _T( 'b' ) )
    {
      int dwRes = 0 ;
      s = s.Mid( 2 ) ;
      while ( !s.IsEmpty() )
      {
        switch ( s[ 0 ] )
        {
        case _T( '1' ): dwRes++ ;
        case _T( '0' ): dwRes <<= 1 ;
          s.Delete( 0 ) ;
          break ;
        default: s.Empty() ; break ;
        }
      }
      iResult = dwRes ;
      return true ;
    }
    else if ( isdigit( s[ 1 ] ) )
    {
      int dwRes ;
      s = s.Mid( 1 ) ;
      if ( _stscanf_s( s ,
#ifdef _WIN64
        _T( "%llo" ) ,
#else
        _T( "%o" ) ,
#endif
        &dwRes ) )
      {
        iResult = dwRes ;
        return true ;
      }
      return false ;
    }
  }
  if ( isdigit( s[ 0 ] ) )
  {
    iResult = _tstoi( s ) ;
    return true ;
  }
  return false ;
}

__forceinline int ConvToBinary( LPCTSTR strIn )
{
  int iResult = 0 ;
  ConvToBinary( strIn , iResult ) ;
  return iResult ;
}


enum KMS_MsgType
{
  KMS_Error = -1 ,
  KMS_Unknown = 0 , 
  KMS_ReadTextResult = 1 , // WPARAM - result text, several words (id,value)
  KMS_ReadNumberResult = 2 , // -----"-------          (id, value as string)
  KMS_Info = 3 ,
  KMS_DoStep = 10 ,
  KMS_ViewLog = 20 ,
  KMS_ViewDoConfigLog 
};


class KeybMouseSimulator
{
  public:
  HWND    m_hOwner ;
  DWORD   m_dwMsgId ;
  HANDLE  m_hSyncHandle ;

  std::thread * pRunnerThread = NULL ;
  bool m_bRunnerExit = false ;
  bool m_bAbortScript = false ;
  bool    m_bRunNewScript = false ;
  bool    m_bScriptIsRunning = false ;
  CString m_ScriptForLoad ;
  HWND    m_hSelectedWindow ;
  CRect   m_CurrentWindowRect ;
  CString m_LastStatusMsg ;
  CListBox m_Log ;

  KeybMouseSimulator( HWND hOwner = NULL , DWORD dwMsgId = 0 ) ;
  ~KeybMouseSimulator() ;

  static LONG ScriptRunner( KeybMouseSimulator * pSimulator ) ;

  int ExecuteStep( CString Step );
  int SetOwner( HWND hOwnerWnd , DWORD dwMsgId );
  int StartScript( LPCTSTR pScript );
  int AbortScript();
  int PostMsg( KMS_MsgType Type , LPCTSTR pMsg ) ;
  bool IsScriptRunning()
  {
    return m_bScriptIsRunning;
  }
};

