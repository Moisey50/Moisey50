// SerialPort.cpp: implementation of the CSerialPort class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SerialPort.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

#define MAXBLOCK        4096

#define THIS_MODULENAME "SerialPort"

const char PortNames[][6] = { "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "COM10" };
const char WrongName[] = "*";

#define ASCII_XON       0x11
#define ASCII_XOFF      0x13


const char * CSerialPort::GetPortName(int iPortNum)
{
  if ( iPortNum >= 0 )
  {
    ASSERT( (0 <= iPortNum) && ( iPortNum < m_AvailablePorts.size() ) ) ;
    sprintf_s( m_szPortName , "%s%s" , 
      (iPortNum > 9) ? "\\\\.\\" : "" , (LPCTSTR)m_AvailablePorts[ iPortNum ] ) ;
    return m_szPortName ;
  }
//   if ((iPortNum >= 0) && (iPortNum < 10))
//     return(PortNames[iPortNum]);
  return (WrongName);
}


const DWORD Bauds[] = { CBR_110, CBR_300, CBR_600, CBR_1200, CBR_2400,
CBR_4800, CBR_9600, CBR_14400, CBR_19200, CBR_38400,
CBR_56000, CBR_115200, CBR_128000, CBR_256000 };

const BYTE Parities[] = { NOPARITY, ODDPARITY, EVENPARITY, MARKPARITY, SPACEPARITY };
const char * ParityAsText[5] =
{
  "No", "Odd", "Even", "Mark", "Space"
};

const char * StopBitsAsText[] = { "1", "1.5", "2" };

DWORD GetBaudRate(int No)
{
  if ((No >= 0) && (No < (sizeof(Bauds) / sizeof(DWORD))))
    return(Bauds[No]);
  return(-1);
}
DWORD GetIndex(DWORD iBaud)
{
  for (int i = 0; i < ARRSZ(Bauds); i++)
  {
    if (Bauds[i] == iBaud)
      return i;
  }
  return 11;
}

BYTE GetParity(int No)
{
  if ((No >= 0) && (No < ARRSZ(Parities)))
    return(Parities[No]);
  return(-1);
}

const char * GetParityAsText(int No)
{
  if ((No >= 0) && (No < ARRSZ(Parities)))
    return (ParityAsText[No]);
  return(NULL);
}

static LPCTSTR ParName[ ] =
{
  "Port" , "Baud" , "NDataBits" , "Parity" ,
  "NStopBits" , "XOnXOff" , "DtrDsr" , "RtsCts" ,
  "Flag" , "AppLF" , "IOM_Hex1_CSV2_NoFlag3" , "BlockSize" ,
  "LineDelay" , "BlockWait"
};


FXString CSerialPort::GetChanged( DWORD dwMask )
{
  FXString RetVal;
  for (int i = 0; i < ARRSZ( ParName ); i++)
  {
    if (dwMask & ( 1 << i ))
      RetVal = RetVal + ' ' + ParName[ i ];
  }

  return RetVal;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSerialPort::CSerialPort(int FlagChar) :
  m_iSelectedPort( 0 ) ,
  m_WantsReturn( TRUE ) ,
  m_bAppendLF( TRUE ) ,
  m_dLineDelay_ms( 0. ) ,
  m_dBlockWaitTime_ms( 0. ) ,
  m_iBlockSize( 0 ) ,
  m_HostRxProc( NULL ) ,
  m_HostParam( NULL ) 
{
  m_PortOpen = FALSE;
  m_pWatchThread = NULL;
  m_hComDev = NULL;
  InitializeCriticalSection(&m_CS);
  m_FlagChar = FlagChar;
  m_dLastSendingSpent_ms = m_dLastReceivingSpent_ms = m_dInitTime_ms = 0. ;
}

CSerialPort::~CSerialPort()
{
  if (m_PortOpen)
  {
    CloseConnection();
    CloseHandle(OlsRead.hEvent);
    CloseHandle(OlsWrite.hEvent);
  }
  DeleteCriticalSection(&m_CS);
}

void CSerialPort::RxString(char * pData, int iLen)
{
  if ( !m_HostRxProc ) 
    return ; // nothing to do
  if (m_FlagChar) // working with flag char
  {          // all strings with flag char will be sent one by one
    int iForwSearch = 0 ;

    while ( iForwSearch < iLen )
    {
      if (pData[ iForwSearch ] == m_FlagChar)
      {
        if (m_iSavedLen)
        {
          ASSERT( m_iSavedLen + iForwSearch < sizeof( m_SavedData ) );
          memcpy( m_SavedData + m_iSavedLen , pData , iForwSearch + 1 );
          m_HostRxProc( m_HostParam , m_SavedData , m_iSavedLen + iForwSearch + 1 );
          m_iSavedLen = 0 ;
        }
        else
          m_HostRxProc( m_HostParam , pData , iForwSearch + 1 ) ;

        iLen -= iForwSearch + 1;
        pData += iForwSearch + 1;
        iForwSearch = 0;
        continue;
      }
      else if ( m_iBlockSize ) // is there block length restriction
      {
        if ( m_iSavedLen )
        {
          if (( m_iSavedLen + iForwSearch + 1 ) >= m_iBlockSize)
          {
            memcpy( m_SavedData + m_iSavedLen , pData , iForwSearch + 1 ) ;
            m_HostRxProc( m_HostParam , m_SavedData , m_iSavedLen + iForwSearch + 1 );
            m_iSavedLen = 0;
          }
        }
        else
          m_HostRxProc( m_HostParam , pData , iForwSearch + 1 ) ;

        iLen -= iForwSearch + 1;
        pData += iForwSearch + 1;
        iForwSearch = 0;
        continue;
      }
      else
        iForwSearch++ ;
    }
    if ( iLen )
    {
      memcpy( m_SavedData + m_iSavedLen , pData , iLen ) ;
      m_iSavedLen += iLen ;
      return ;
    }
  }
  else if ( m_iBlockSize ) // working without flag and with block size restriction
  {
    while ( iLen )
    {
      if ( m_iSavedLen )
      {
        if ( iLen < m_iBlockSize - m_iSavedLen )
        {
          memcpy( m_SavedData + m_iSavedLen , pData , iLen );
          m_iSavedLen += iLen ;
          iLen = 0 ;
        }
        else
        {
          memcpy( m_SavedData + m_iSavedLen , pData , m_iBlockSize - m_iSavedLen );
          m_HostRxProc( m_HostParam , m_SavedData , m_iBlockSize );
          iLen -= m_iBlockSize - m_iSavedLen ;
          pData += m_iBlockSize - m_iSavedLen ;
          m_iSavedLen = 0 ;
        }
      }
      else if ( iLen >= m_iBlockSize )
      {
        m_HostRxProc( m_HostParam , pData , m_iBlockSize );
        iLen -= m_iBlockSize ;
        pData += m_iBlockSize ;
      }
      else
      {
        memcpy( m_SavedData , pData , iLen );
        m_iSavedLen = iLen ;
        iLen = 0 ;
      }
    }
  }
  else // simple send data to receiver
  {
    m_HostRxProc( m_HostParam , pData , iLen ) ;
  }
}

BOOL CSerialPort::TxString(char * a, int len)
{
  BOOL res = FALSE;
 if ( m_dLineDelay_ms )
  {
    double dNow = GetHRTickCount() ;
    double dPassedAfterLastSent_ms  = dNow - m_dLastSentTime_ms ;
    double dWait_ms = m_dLineDelay_ms - dPassedAfterLastSent_ms ;
    if ( dWait_ms > 0. )
      Sleep( ROUND( ceil( dWait_ms ) ) ) ;
  }
  int iBytesToWrite = ( len == 0 ) ? (int)strlen( a ) : len ;

  DWORD BytesWritten;
  if ((m_PortOpen) && (m_hComDev) && (m_hComDev != (HANDLE)(-1)))
  {
    if (iBytesToWrite == 0)
    {
      res = WriteFile(m_hComDev, a, 1, &BytesWritten, &OlsWrite);
      if ( m_dwAdditionalFlags & FILE_FLAG_OVERLAPPED )
        res = GetOverlappedResult(m_hComDev, &OlsWrite, &BytesWritten, TRUE);
    }
    else
    {
      res = WriteFile(m_hComDev, a , iBytesToWrite , &BytesWritten, &OlsWrite);
    }
    if (m_bAppendLF)
    {
      char LF = '\n' ;
      res = WriteFile( m_hComDev , &LF , 1 , &BytesWritten , &OlsWrite );
    }
    if (m_dwAdditionalFlags & FILE_FLAG_OVERLAPPED)
      res = GetOverlappedResult( m_hComDev , &OlsWrite , &BytesWritten , TRUE );

    m_dLastSentTime_ms = GetHRTickCount() ;
  }
  return res;
}


BOOL CSerialPort::SendChar(char a)
{
  char b[2];
  b[1] = 0; b[0] = a;
  TxString(b);
  return(TRUE);
}

BOOL CSerialPort::SendBlock( LPBYTE data , int len )
{
  TxString( ( char* ) data , len );
  return TRUE;
}

BOOL CSerialPort::SendBlockDirect( LPBYTE data , int len )
{
  BOOL res = FALSE;
  if ( !data || (len == 0))
    return FALSE ;
  if (m_dLineDelay_ms)
  {
    double dNow = GetHRTickCount( );
    double dPassedAfterLastSent_ms = dNow - m_dLastSentTime_ms;
    double dWait_ms = m_dLineDelay_ms - dPassedAfterLastSent_ms;
    if (dWait_ms > 0.)
      Sleep( ROUND( ceil( dWait_ms ) ) );
  }
  DWORD BytesWritten;
  if (( m_PortOpen ) && ( m_hComDev ) && ( m_hComDev != ( HANDLE ) ( -1 ) ))
  {
    res = WriteFile( m_hComDev , data , len , &BytesWritten , &OlsWrite );
    m_dLastSentTime_ms = GetHRTickCount( );
  }
  return res;
}

BOOL CSerialPort::Create(COM_RxProc* RxProc, LPVOID pParam)
{
  m_HostRxProc = RxProc;
  m_HostParam = pParam;
  memset(&OlsWrite, 0, sizeof(OlsWrite));
  memset(&OlsRead, 0, sizeof(OlsRead));
  return(TRUE);
}

void CSerialPort::OpenConnection( LPCTSTR pDiagnostics , DWORD dwAdditionalFlags )
{
  FXString PortName = GetPortName(m_ComParam.Port);
  if ((m_PortOpen) || (m_pWatchThread) || PortName.IsEmpty())
    return;
  int iPortNum = m_ComParam.Port ; // atoi( (LPCTSTR) PortName + 3 );
  m_PortOpen = ((m_hComDev = CreateFile((LPCTSTR)PortName,
    GENERIC_READ | GENERIC_WRITE,
    0, NULL, OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL | dwAdditionalFlags , NULL)) != INVALID_HANDLE_VALUE);
  if (m_PortOpen)
  {
    m_dwAdditionalFlags = dwAdditionalFlags ;
    COMMTIMEOUTS  CommTimeOuts;
    DCB           dcb;

    m_EvtMask = (m_FlagChar > 0) ? EV_RXFLAG : EV_RXCHAR;
    SetCommMask(m_hComDev, m_EvtMask);
    SetupComm(m_hComDev, 4096, 4096);
    PurgeComm(m_hComDev, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

    CommTimeOuts.ReadIntervalTimeout = MAXWORD;
    CommTimeOuts.ReadTotalTimeoutMultiplier = MAXWORD;
    CommTimeOuts.ReadTotalTimeoutConstant = 50;
    CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
    CommTimeOuts.WriteTotalTimeoutConstant = 0;
    SetCommTimeouts(m_hComDev, &CommTimeOuts);

    dcb.DCBlength = sizeof(DCB);
    GetCommState(m_hComDev, &dcb);
    dcb.BaudRate = m_ComParam.BaudRate; //GetBaudRate(m_ComParam.BaudRate);
    dcb.ByteSize = GetByteSize(m_ComParam.DataBits);
    dcb.Parity = GetParity(m_ComParam.Parity);
    dcb.StopBits = GetStopBits(m_ComParam.StopBits);

    dcb.fOutxDsrFlow = m_ComParam.DtrDsr;
    switch (m_ComParam.DtrDsr)
    {
    case 0: dcb.fDtrControl = DTR_CONTROL_ENABLE; break;//No
    case 1: dcb.fDtrControl = DTR_CONTROL_HANDSHAKE; break;//Yes
    case 2: dcb.fDtrControl = DTR_CONTROL_DISABLE; break;//Set to zero
    }

    dcb.fOutxCtsFlow = m_ComParam.RtsCts;
    if (dcb.fOutxCtsFlow)
      dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    else
      dcb.fRtsControl = RTS_CONTROL_ENABLE;

    dcb.fInX = dcb.fOutX = m_ComParam.XonXoff;
    dcb.XonChar = ASCII_XON;
    dcb.XoffChar = ASCII_XOFF;
    dcb.XonLim = 100;
    dcb.XoffLim = 100;

    dcb.fBinary = TRUE;
    dcb.fParity = (dcb.Parity != NOPARITY);
    dcb.EvtChar = (char)m_FlagChar;

    if (!SetCommState(m_hComDev, &dcb))
    {
      CloseConnection();
      LPVOID lpMsgBuf;
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
      SENDERR_2("Can't SetCommState port %s: %s", GetPortName(m_ComParam.Port), lpMsgBuf);
      if ( pDiagnostics )
        SENDERR( pDiagnostics ) ;

      LocalFree(lpMsgBuf);
      return;
    }

    OlsRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    OlsWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_pWatchThread = AfxBeginThread(CommWatchProc, (LPVOID)this,
      THREAD_PRIORITY_ABOVE_NORMAL);
    m_pWatchThread->m_bAutoDelete = TRUE;

    if (m_pWatchThread == NULL)
    {
      CloseConnection();
      LPVOID lpMsgBuf;
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
      SENDERR_2("Can't create watch thread port %s: %s", GetPortName(m_ComParam.Port), lpMsgBuf);
      LocalFree(lpMsgBuf);
      return;
    }
    if (dcb.fOutxDsrFlow != DTR_CONTROL_HANDSHAKE )
    {
      if (!EscapeCommFunction(m_hComDev, (dcb.fOutxDsrFlow == DTR_CONTROL_ENABLE) ? SETDTR : CLRDTR ) )
      {
        LPVOID lpMsgBuf;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
          NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          (LPTSTR)&lpMsgBuf, 0, NULL);
        SENDERR_2("Can't EscapeComm port %s: %s", GetPortName(m_ComParam.Port), lpMsgBuf);
        LocalFree(lpMsgBuf);
      }
    }
    if (!EscapeCommFunction(m_hComDev, SETRTS))
    {
      LPVOID lpMsgBuf;
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
      SENDERR_2("Can't EscapeComm port %s: %s", GetPortName(m_ComParam.Port), lpMsgBuf);
      LocalFree(lpMsgBuf);
    }
    ClearCommBreak(m_hComDev);
    FXString Params;
    const int DataLen[] = { 5, 6, 7, 8 };
    Params.Format("%d,%d,%s,%s",
      m_ComParam.BaudRate > 40 ? m_ComParam.BaudRate : Bauds[m_ComParam.BaudRate],
      m_ComParam.DataBits <= 3 ? DataLen[m_ComParam.DataBits] : -1,
      ParityAsText[m_ComParam.Parity], StopBitsAsText[m_ComParam.StopBits]);
    if ( pDiagnostics )
      Params = (Params + "; Cause:  ") + pDiagnostics ;
    SENDINFO_2("Port %s is initialized with %s", GetPortName(m_ComParam.Port), (LPCTSTR)Params);
  }
  else
  {
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&lpMsgBuf, 0, NULL);
    SENDERR_2("Can't open port %s: %s", GetPortName(m_ComParam.Port), lpMsgBuf);
    LocalFree(lpMsgBuf);
  }
}

void CSerialPort::CloseConnection()
{
  if (m_PortOpen)
  {
    m_PortOpen = FALSE;
    if (!EscapeCommFunction(m_hComDev, CLRDTR))
      SENDERR("EscapeCommFunction CLRDTR Failed");
    if (!EscapeCommFunction(m_hComDev, CLRRTS))
      SENDERR("EscapeCommFunction CLRRTS Failed");
    Sleep(50);
    if (!SetCommMask(m_hComDev, 0))
      SENDERR("SetCommMask Failed");
    if (!PurgeComm(m_hComDev,
      PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR))
      SENDERR("PurgeComm Failed");
    if (!CloseHandle(OlsRead.hEvent))
      SENDERR("CloseHandle( OlsRead.hEvent ) Failed");
    if (!CloseHandle(OlsWrite.hEvent))
      SENDERR("CloseHandle( OlsWrite.hEvent ) Failed");
    if (!CloseHandle(m_hComDev))
      SENDERR("CloseHandle( m_hComDev ) Failed");
    Sleep( 30 ) ;
    if (m_pWatchThread && (m_pWatchThread->m_hThread))
    {
      DWORD Stat = WaitForSingleObject(m_pWatchThread->m_hThread, 2000);
      if (Stat != WAIT_OBJECT_0)
        SENDERR("Read Thread Closing ERROR = 0x%08H", Stat);
      m_pWatchThread = NULL;
    }

    OlsRead.hEvent = NULL;
    OlsWrite.hEvent = NULL;
    m_hComDev = 0;
  }
}

int ReadCommBlock(CSerialPort*Port, LPSTR DataBlock, int MaxLength)
{
  DWORD      ErrorFlags;
  COMSTAT    ComStat;
  DWORD      Length, iNRead = 0;
  DWORD      Error;
  BOOL       ReadStat;

  ClearCommError(Port->m_hComDev, &ErrorFlags, &ComStat);
  Length = min((DWORD)MaxLength, ComStat.cbInQue);
  if (Length > 0)
  {
    ReadStat = ReadFile(Port->m_hComDev, DataBlock, Length, &iNRead, &(Port->OlsRead));
    // 	if ( iNRead )
    //       DataBlock[iNRead]=0;
    if (!ReadStat)
    {
      if (GetLastError() == ERROR_IO_PENDING)
      {
        while (!GetOverlappedResult(Port->m_hComDev, &(Port->OlsRead), &iNRead, TRUE))
        {
          Error = GetLastError();
          if (Error == ERROR_IO_INCOMPLETE)
            continue;
          else
          {
            ClearCommError(Port->m_hComDev, &ErrorFlags, &ComStat);
            break;
          }
        }
      }
      else
      {
        // some other error occurred

        iNRead = 0;
        ClearCommError(Port->m_hComDev, &ErrorFlags, &ComStat);
      }
    }
  }
  return(iNRead);
}

UINT CommWatchProc(LPVOID lpData)
{
  char       InBuffer[MAXBLOCK + 1];
  DWORD       EvtMask;
  OVERLAPPED  os;
  int         Length = 1;
  CSerialPort*   Port = (CSerialPort*)lpData;

  memset(&os, 0, sizeof( os ));

  os.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (os.hEvent == NULL)
  {
    FxMessageBox("COM Error: Failed to create event for thread!");
    return (FALSE);
  }
  if (!SetCommMask(Port->m_hComDev, EV_RXCHAR))
    return (FALSE);
  while (Port->m_PortOpen)
  {
    EvtMask = 0;

    WaitCommEvent(Port->m_hComDev, &EvtMask, NULL);
    if ((EvtMask & EV_RXCHAR) == EV_RXCHAR)
    {
      char * p = InBuffer;
      do
      {
        Length = ReadCommBlock(Port, p, MAXBLOCK - (int)(p - InBuffer));
        p += Length;
      } while (Length);
      if (p != InBuffer)
        Port->RxString(InBuffer, (int)(p - InBuffer));
    }
  }
  CloseHandle(os.hEvent);
  Port->m_pWatchThread = NULL;
  SENDINFO("Read Thread Closed ");
  return(TRUE);
} // end of CommWatchProc()

bool CSerialPort::GetComPortNames( FXStringArray& Ports )
{
  //Make sure we clear out any elements which may already be in the array(s)
  Ports.RemoveAll( );

  //What will be the return value from this function (assume the worst)
  bool bSuccess = false;

  HKEY hSERIALCOMM;
  LSTATUS Stat = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
    _T( "HARDWARE\\DEVICEMAP\\SERIALCOMM" ) , 0 , KEY_QUERY_VALUE , &hSERIALCOMM );
  if (Stat == ERROR_SUCCESS)
  {
    //Get the max value name and max value lengths
    DWORD dwMaxValueNameLen;
    DWORD dwMaxValueLen;
    DWORD dwQueryInfo = RegQueryInfoKey( hSERIALCOMM ,
      NULL , NULL , NULL , NULL , NULL , NULL , NULL , &dwMaxValueNameLen , &dwMaxValueLen , NULL , NULL );
    if (dwQueryInfo == ERROR_SUCCESS)
    {
      DWORD dwMaxValueNameSizeInChars = dwMaxValueNameLen + 1; //Include space for the NULL terminator
      DWORD dwMaxValueNameSizeInBytes = dwMaxValueNameSizeInChars * sizeof( TCHAR );
      DWORD dwMaxValueDataSizeInChars = dwMaxValueLen / sizeof( TCHAR ) + 1; //Include space for the NULL terminator
      DWORD dwMaxValueDataSizeInBytes = dwMaxValueDataSizeInChars * sizeof( TCHAR );

      //Allocate some space for the value name and value data			
      TCHAR * valueName = new TCHAR[ dwMaxValueNameSizeInBytes ];
      BYTE * valueData = new BYTE[ dwMaxValueDataSizeInBytes ];
      if (valueName && valueData)
      {
        bSuccess = true;

        //Enumerate all the values underneath HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\SERIALCOMM
        DWORD dwIndex = 0;
        DWORD dwType;
        DWORD dwValueNameSize = dwMaxValueNameSizeInChars;
        DWORD dwDataSize = dwMaxValueDataSizeInBytes;
        memset( valueName , 0 , dwMaxValueNameSizeInBytes );
        memset( valueData , 0 , dwMaxValueDataSizeInBytes );
        LONG nEnum = RegEnumValue( hSERIALCOMM , dwIndex , valueName ,
          &dwValueNameSize , NULL , &dwType , valueData , &dwDataSize );
        while (nEnum == ERROR_SUCCESS)
        {
          //If the value is of the correct type, then add it to the array
          if (dwType == REG_SZ)
          {
            TCHAR* szPort = reinterpret_cast< TCHAR* >( valueData );
            Ports.Add( szPort );
          }

          //Prepare for the next time around
          dwValueNameSize = dwMaxValueNameSizeInChars;
          dwDataSize = dwMaxValueDataSizeInBytes;
          memset( valueName , 0 , dwMaxValueNameSizeInBytes );
          memset( valueData , 0 , dwMaxValueDataSizeInBytes );
          ++dwIndex;
          nEnum = RegEnumValue( hSERIALCOMM , dwIndex , valueName ,
            &dwValueNameSize , NULL , &dwType , valueData , &dwDataSize );
        }
      }
      else
        SetLastError( ERROR_OUTOFMEMORY );
      delete[ ] valueName;
      delete[ ] valueData;
    }

    //Close the registry key now that we are finished with it    
    RegCloseKey( hSERIALCOMM );

    if (dwQueryInfo != ERROR_SUCCESS)
      SetLastError( dwQueryInfo );
  }

  return bSuccess;
}

bool CSerialPort::ScanSettings( FXString& text , LPCTSTR pKnownBefore )
{
  FXStringArray ComPorts;
  FXString camlist( pKnownBefore ? pKnownBefore : "Not Selected(-1)" ) , paramlist , tmpS;
  if ( !camlist.IsEmpty() )
    camlist += ',' ;
  if (GetComPortNames( ComPorts ))
  {
    for (int i = 0; i < ComPorts.GetCount( ); i++)
    {
      tmpS.Format( "%s%s(%d)" , ( i == 0 ) ? "" : "," , ( LPCTSTR ) ComPorts[ i ] , i );
      camlist += tmpS;
    }
  }
  tmpS.Format( "ComboBox(Port(%s))," , camlist );
  paramlist += tmpS;
  paramlist += "ComboBox(WantsCR(No(0),Yes(1))),";
  paramlist += "EditBox(LineDelay_ms),";
  paramlist += "ComboBox(AppendLF(No(0),Yes(1))),";
  paramlist += "Spin(BlockSize,0,4096),";
  paramlist += "EditBox(BlockWaitTime_ms)";

  if (ComPorts.GetCount( ) && m_iSelectedPort >= 0)
  {
    paramlist += _T( ',' );
    FXString BaudList;
    int iIndex = 0;
    int iNextBaud;
    while (( iNextBaud = GetBaudRate( iIndex ) ) > 0)
    {
      tmpS.Format( "%s%d(%d)" , ( iIndex++ == 0 ) ? "" : "," , iNextBaud , iNextBaud );
      BaudList += tmpS;
    }
    tmpS.Format( "ComboBox(BaudRate(%s))," , BaudList );
    paramlist += tmpS;

    paramlist += "ComboBox(DataBits(5(0),6(1),7(2),8(3))),";
    paramlist += "ComboBox(Parity(No(0),Odd(1),Even(2),Mark(3),Space(4))),";
    paramlist += "ComboBox(StopBits(1(0),1.5(1), 2(2))),";
    paramlist += "ComboBox(XonXoff(No(0),Yes(1))),";
    paramlist += "ComboBox(DtrDsr(SetToOne(0),HandShake(1),SetToZero(2))),";
    paramlist += "ComboBox(RtsCts(No(0),Yes(1)))";
    //     paramlist += "EditBox(Dummy)" ;
  }
  text += paramlist ;
  return true;
}

bool CSerialPort::PrintProperties( FXString& text )
{
  FXPropertyKit pk;
  pk.WriteInt( "Port" , m_iSelectedPort );
  if (m_iSelectedPort >= 0 && m_iSelectedPort < m_AvailablePorts.GetCount( ))
  {
    pk.WriteString( "PortName" , GetPortName( m_iSelectedPort ) );
    pk.WriteInt( "BaudRate" , m_ComParam.BaudRate );
    pk.WriteInt( "DataBits" , m_ComParam.DataBits );
    pk.WriteInt( "Parity" , m_ComParam.Parity );
    pk.WriteInt( "StopBits" , m_ComParam.StopBits );
    pk.WriteInt( "XonXoff" , m_ComParam.XonXoff );
    pk.WriteInt( "DtrDsr" , m_ComParam.DtrDsr );
    pk.WriteInt( "RtsCts" , m_ComParam.RtsCts );
  }
  else
    pk.WriteString( "PortName" , "Not Selected" );
  pk.WriteInt( "WantsCR" , m_WantsReturn );
  pk.WriteInt( "AppendLF" , m_bAppendLF );
  pk.WriteInt( "BlockSize" , m_iBlockSize );
  pk.WriteDouble( "LineDelay_ms" , m_dLineDelay_ms );
  pk.WriteDouble( "BlockWaitTime_ms" , m_dBlockWaitTime_ms );
  text += pk;
  return true;
}

bool CSerialPort::ScanProperties( LPCTSTR text , bool& Invalidate )
{
  FXPropertyKit pk( text );
  bool bPortChanged = false;
  m_dwLastChanged = 0;
  if (GetComPortNames( m_AvailablePorts ))
  {
    FXString PortNumAsString;
    int iSelectedPort = -1;
    bool bSelected = false;
    if (bSelected |= pk.GetString( "PortName" , PortNumAsString ))
    {
      bSelected = false ;
      for (int i = 0; i < m_AvailablePorts.GetCount( ); i++)
      {
        if (PortNumAsString == GetPortName( i ))
        {
          iSelectedPort = i;
          bSelected = true ;
          break;
        }
      }
    }
    if (!bSelected)
    {
      if (bSelected |= pk.GetString( "Port" , PortNumAsString ))
      {
        PortNumAsString.Trim( );
        if (isdigit( PortNumAsString[ 0 ] ))
          iSelectedPort = atoi( PortNumAsString );
        else
        {
          for (int i = 0; i < m_AvailablePorts.GetCount( ); i++)
          {
            if (PortNumAsString == GetPortName( i ))
            {
              iSelectedPort = i;
              strcpy_s( m_szPortName , (LPCTSTR)PortNumAsString ) ;
              break;
            }
          }
        }
      }
    }
    if (bSelected && ( (PortNumAsString != m_szPortName) 
    || (m_iSelectedPort != iSelectedPort) 
    || (m_ComParam.Port != m_iSelectedPort) ))
    {
      m_dwLastChanged |= CPCHANGE_Port;

      m_iSelectedPort = iSelectedPort < 0 ? -1 : iSelectedPort;


      if (m_iSelectedPort >= 0)
      {
        strcpy_s( m_szPortName , GetPortName( m_iSelectedPort ) );
        m_ComParam.Port = m_iSelectedPort;
        bPortChanged = true;
      }

      Invalidate = true;

    }
    //else if ( m_iSelectedPort >= 0 )
    //  SENDERR( _T( "No COM port %s" ) , ( LPCTSTR )PortNumAsString ) ;
  }
  else
  {
    m_iSelectedPort = -1;
    Invalidate = true;
  }

  BOOL bOld = m_WantsReturn;
  BOOL bNew = FALSE;
  if (pk.GetInt( "WantsCR" , bNew ) && ( bNew != m_WantsReturn ))
  {
    m_WantsReturn = bNew;
    m_dwLastChanged |= CPCHANGE_FlagCR;
    Invalidate = true;
  }
  double dNew = m_dLineDelay_ms ;
  if (pk.GetDouble( "LineDelay_ms" , dNew ) && ( dNew != m_dLineDelay_ms ))
  {
    m_dLineDelay_ms = dNew;
    m_dwLastChanged |= CPCHANGE_LDelay;
    Invalidate = true;
  }
  bNew = FALSE;
  if (pk.GetInt( "AppendLF" , bNew ) && ( bNew != m_bAppendLF ))
  {
    m_bAppendLF = bNew;
    m_dwLastChanged |= CPCHANGE_AppLF;
    Invalidate = true;
  }
  int iNew = m_iBlockSize ;
  if (pk.GetInt( "BlockSize" , iNew ) && ( iNew != m_iBlockSize ))
  {
    m_iBlockSize = iNew;
    m_dwLastChanged |= CPCHANGE_BlkSiz;
    Invalidate = true;
  }
  dNew = m_dBlockWaitTime_ms ;
  if (pk.GetDouble( "BlockWaitTime_ms" , dNew ) && ( dNew != m_dBlockWaitTime_ms ))
  {
    m_dBlockWaitTime_ms = dNew;
    m_dwLastChanged |= CPCHANGE_BlkWait;
    Invalidate = true;
  }
  if (m_iSelectedPort >= 0)
  {
    if (pk.GetInt( "BaudRate" , iNew ) && ( iNew != m_ComParam.BaudRate ))
    {
      m_ComParam.BaudRate = iNew;
      m_dwLastChanged |= CPCHANGE_Baud;
      Invalidate = true;
    }
    if (pk.GetInt( "DataBits" , iNew ) && ( iNew != m_ComParam.DataBits ))
    {
      m_ComParam.DataBits = iNew;
      m_dwLastChanged |= CPCHANGE_Size;
      Invalidate = true;
    }
    if (pk.GetInt( "Parity" , iNew ) && ( iNew != m_ComParam.Parity ))
    {
      m_ComParam.Parity = iNew;
      m_dwLastChanged |= CPCHANGE_Parity;
      Invalidate = true;
    }
    if (pk.GetInt( "StopBits" , iNew ) && ( iNew != m_ComParam.StopBits ))
    {
      m_ComParam.StopBits = iNew;
      m_dwLastChanged |= CPCHANGE_Stop;
      Invalidate = true;
    }
    if (pk.GetInt( "XonXoff" , bNew ) && ( bNew != m_ComParam.XonXoff ))
    {
      m_ComParam.XonXoff = bNew;
      m_dwLastChanged |= CPCHANGE_XOnOff;
      Invalidate = true;
    }
    if (pk.GetInt( "DtrDsr" , bNew ) && ( bNew != m_ComParam.DtrDsr ))
    {
      if (m_PortOpen)
      {
        m_ComParam.DtrDsr = bNew;
        if (bNew != 1) // DTR-RTS handshake protocol
        {
          if (!EscapeCommFunction( m_hComDev , ( bNew == 2 ) ? CLRDTR : SETDTR ))
          {
            LPVOID lpMsgBuf;
            FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM ,
              NULL , GetLastError( ) , MAKELANGID( LANG_NEUTRAL , SUBLANG_DEFAULT ) ,
              ( LPTSTR ) &lpMsgBuf , 0 , NULL );
            SENDERR( "Can't %s DTR port %s: %s" ,
              ( bNew == 2 ) ? "CLR" : "SET" , GetPortName( m_ComParam.Port ) , lpMsgBuf );
            LocalFree( lpMsgBuf );
          }
        }
        else
        {
          m_dwLastChanged |= CPCHANGE_DtrDsr;
          Invalidate = true;
        }
      }
      else
      {
        m_dwLastChanged |= CPCHANGE_DtrDsr;
        Invalidate = true;
      }
    }
    if (pk.GetInt( "RtsCts" , bNew ) && ( bNew != m_ComParam.RtsCts ))
    {
      m_ComParam.RtsCts = bNew;
      m_dwLastChanged |= CPCHANGE_RtsCts;
      Invalidate = true;
    }
    if (m_dwLastChanged)
    {
      CloseConnection( );
      Sleep( 50 );
      FXString Changed = GetChanged( m_dwLastChanged );
      OpenConnection( ( LPCTSTR ) Changed );
    }
  }
  else
  {
    CloseConnection( );
    m_ComParam.Port = -1;
  }

  m_FlagChar = ( m_WantsReturn ) ? '\n' : 0 ;

  return true;
}




