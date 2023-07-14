// SerialPort.h: interface for the CSerialPort class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERIALPORT_H__93D49ED1_DB42_11D1_AF52_000001359766__INCLUDED_)
#define AFX_SERIALPORT_H__93D49ED1_DB42_11D1_AF52_000001359766__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

typedef BOOL (COM_RxProc)(LPVOID pParam, char *Data , int iLen );

// #include "Portsettings.h"
#include "math/Intf_sup.h"

#include <helpers\InternalWatcher.h>

#define CPCHANGE_Port   0x0001
#define CPCHANGE_Baud   0x0002
#define CPCHANGE_Size   0x0004
#define CPCHANGE_Parity 0x0008
#define CPCHANGE_Stop   0x0010
#define CPCHANGE_XOnOff 0x0020
#define CPCHANGE_DtrDsr 0x0040
#define CPCHANGE_RtsCts 0x0080
#define CPCHANGE_FlagCR 0x0100
#define CPCHANGE_AppLF  0x0200
#define CPCHANGE_HexMod 0x0400
#define CPCHANGE_BlkSiz 0x0800
#define CPCHANGE_LDelay 0x1000
#define CPCHANGE_BlkWait 0x2000

DWORD GetBaudRate(int No); 
DWORD GetIndex( DWORD iBaud );
BYTE GetParity(int No) ;
const char * GetParityAsText( int No ) ;
const char * GetPortName( int iPortNum ) ;
#define      GetByteSize(a) ((a)+5)
#define      GetStopBits(a) (a)

class ComPortParam
{
public :
  int  Port = 0 ;
  int  BaudRate = 9600 ;
  int  DataBits = 3 ;
  int  Parity = 0 ;
  int  StopBits = 0 ;
  BOOL XonXoff = FALSE ;
  BOOL RtsCts = FALSE ;
  BOOL DtrDsr = FALSE ;
} ;


class CSerialPort  
{
  friend class CComPortSettings;
protected:
  COM_RxProc*  m_HostRxProc;
  LPVOID       m_HostParam;
  ComPortParam m_ComParam;
  BOOL         m_PortOpen;
  HANDLE       m_hComDev;
  CRITICAL_SECTION  m_CS ;
//   CInputListen * m_pInputListen;
  DWORD        m_EvtMask ;
  int          m_iSelectedPort;
  BOOL         m_WantsReturn;
  BOOL         m_bAppendLF;
  double       m_dLineDelay_ms;
  double       m_dLastSentTime_ms ;
  double       m_dBlockWaitTime_ms;
  int          m_iBlockSize;
  DWORD        m_dwLastChanged;
  OVERLAPPED   OlsWrite, OlsRead ;
  char         m_szPortName[30] ;
  FXStringArray       m_AvailablePorts;
  char         m_SavedData[ 10000 ] ;
  int          m_iSavedLen = 0 ;
public:
  int          m_FlagChar ;
  DWORD        m_dwAdditionalFlags = FILE_FLAG_OVERLAPPED ;
  double       m_dLastReceivingSpent_ms ;
  double       m_dLastSendingSpent_ms ;
  double       m_dInitTime_ms ;

public:
  CWinThread*  m_pWatchThread;
  CSerialPort( int FlagChar = '\n ');
  ~CSerialPort();
  void    PortAbort() {  CloseConnection();};
  BOOL    Create(COM_RxProc* RxProc, LPVOID pParam);
  BOOL    SendChar(char a);
  BOOL    SendBlock( LPBYTE data , int len );
  BOOL    SendBlockDirect( LPBYTE data , int len );
  virtual const char * GetPortName( int iPortNum ) ;
  virtual const char * GetSelectedPortName( )
  {
    return GetPortName( m_iSelectedPort ) ;
  }
  friend UINT CommWatchProc(LPVOID);
  friend int ReadCommBlock(CSerialPort*Port,LPSTR DataBlock,int MaxLength);

  bool GetComPortNames( FXStringArray& Ports ) ;
  bool ScanSettings( FXString& text , LPCTSTR pKnownBefore = NULL ) ;
  bool PrintProperties( FXString& Text ) ;
  bool ScanProperties( LPCTSTR text , bool& Invalidate ) ;
  FXString GetChanged( DWORD dwMask ) ;

  bool IsConnectionOpen() { return m_PortOpen ;}

protected:
  BOOL    TxString(char *a, int len=0);
  void    RxString(char* a , int iLen);
  void    CloseConnection();
  void    OpenConnection( LPCTSTR pDiagnostics = NULL , DWORD dwAdditionalFlag = FILE_FLAG_OVERLAPPED );
};

#endif // !defined(AFX_SERIALPORT_H__93D49ED1_DB42_11D1_AF52_000001359766__INCLUDED_)
