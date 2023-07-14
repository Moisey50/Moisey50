
// LaserBurningConfigDlg.cpp : implementation file
//

#include "pch.h"
#include "stdlib.h"
#include "framework.h"
#include "LaserBurningConfig.h"
#include "LaserBurningConfigDlg.h"
#include "DlgProxy.h"
#include "afxdialogex.h"
#include "Hook.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern MyHook g_MouseHook ;

DWORD dwMsgFromSimulator = RegisterWindowMessage( "FromSimulator" ) ;
DWORD dwMouseMsg = RegisterWindowMessage( "FromMouse" ) ;


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

CString   GetTimeStamp( LPCTSTR prefix , LPCTSTR suffix )
{
  CString retV;
  SYSTEMTIME stime;
  //structure to store system time (in usual time format)
  FILETIME ltime;
  //structure to store local time (local time in 64 bits)
  FILETIME ftTimeStamp;
  GetSystemTimeAsFileTime( &ftTimeStamp ); //Gets the current system time

  FileTimeToLocalFileTime( &ftTimeStamp , &ltime );//convert to local time and store in ltime
  FileTimeToSystemTime( &ltime , &stime );//convert in system time and store in stime

  retV.Format( "%s%4d%02d%02d%02d%02d%02d%03d%s" , prefix ,
    stime.wYear , stime.wMonth , stime.wDay ,
    stime.wHour , stime.wMinute , stime.wSecond , stime.wMilliseconds ,
    suffix );

  return retV;
}



// CLaserBurningConfigDlg dialog


IMPLEMENT_DYNAMIC(CLaserBurningConfigDlg, CDialogEx);

LPCTSTR GetEventName( int iEvent )
{
  switch ( iEvent )
  {
    case TM_MoveCursorRelative:  return "TM_MoveCursorRelative" ;
    case TM_MoveCursorAbsolute:  return "TM_MoveCursorAbsolute" ;
    case TM_ClickLeftButton:     return "TM_ClickLeftButton" ;
    case TM_ClickRightButton:    return "TM_ClickRightButton" ;
    case TM_ReleaseLeftButton:   return "TM_ReleaseLeftButton" ;
    case TM_ReleaseRightButton:  return "TM_ReleaseRightButton" ;
    case TM_KeyStroke:           return "Key Pressed " ;
    case TM_ReleaseKeyStroke:    return "Key Released " ;
    case TM_KeyStrokes:          return "Keyb String " ;
 }
  return "Unknown" ;
}

WindowClassIsControl IsWindowAControl( HWND hWnd )
{
  TCHAR ClassName[ 100 ] = { 0 } ;
  GetClassName( hWnd , ClassName , 99 ) ;
  if ( _tcscmp( ClassName , "ComboBox" ) == 0 )
    return WCIC_Combo ;
  if ( _tcscmp( ClassName , "ListBox" ) == 0 )
    return WCIC_List ;
  if ( _tcscmp( ClassName , "Edit" ) == 0 )
    return WCIC_Edit ;
  return WCIC_No ;
}

LONG CLaserBurningConfigDlg::MouseControlLoop( 
  CLaserBurningConfigDlg * pHost )
{
  TRACE( "*********************MouseControlLoop entry\n" ) ;
  while ( !pHost->m_bMouseControlThreadExit )
  {
    int iRes = WaitForSingleObject( pHost->m_hMouseControlEvent , INFINITE ) ; 
    if ( (iRes == WAIT_OBJECT_0)  && !pHost->m_bMouseControlThreadExit )
    {
      ResetEvent( pHost->m_hMouseControlEvent ) ;
      iRes = SendInput( 1 , &pHost->m_KeybMouseData , sizeof( pHost->m_KeybMouseData ) ) ;
      ASSERT( iRes ) ;
      SetEvent( pHost->m_hMouseControlExecutedEvent ) ;
    }
    else if ( iRes == WAIT_ABANDONED )
    {
      SetEvent( pHost->m_hMouseControlExecutedEvent ) ;
      break ;
    }
  }
  TRACE( "*********************MouseControlLoop exit\n" ) ;
  return 0 ;
}

CLaserBurningConfigDlg::CLaserBurningConfigDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LASERBURNINGCONFIG_DIALOG, pParent)
  , m_iStationForConfiguration( 0 )
  , m_GetConfigScriptFileName( _T( "" ) )
  , m_Status( _T( "" ) )
  , m_bMouseHook( FALSE )
  , m_bMouseControlThreadExit( FALSE )
  , m_hMouseControlEvent( CreateEvent( NULL , TRUE , FALSE , NULL ) )
  , m_hMouseControlExecutedEvent( CreateEvent( NULL , TRUE , FALSE , NULL ) )
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pAutoProxy = nullptr;
  RestoreLastValues() ;

  m_pMouseControlThread = new std::thread( MouseControlLoop , this ) ;

}

CLaserBurningConfigDlg::~CLaserBurningConfigDlg()
{
  FXRegistry Reg( "FileX" ) ;

  Reg.WriteRegiInt(
    "LaserBurning" , "ViewMode(0-debug,1-work)" , ( int ) m_DialogViewMode ) ;

  if ( m_bMouseHook  )
  {
    g_MouseHook.UninstallHook() ;
    m_bMouseHook = FALSE ;
  }
  m_bMouseControlThreadExit = true ;
  SetEvent( m_hMouseControlEvent ) ;
  m_pMouseControlThread->join() ;
  Sleep(50) ;
  CloseHandle( m_hMouseControlEvent ) ;
  CloseHandle( m_hMouseControlExecutedEvent ) ;
  delete m_pMouseControlThread ;
	
  if (m_pAutoProxy != nullptr)
		m_pAutoProxy->m_pDialog = nullptr;


}

void CLaserBurningConfigDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange( pDX );
  DDX_Text( pDX , IDC_STATION , m_iStationForConfiguration );
  DDV_MinMaxInt( pDX , m_iStationForConfiguration , 1 , 5 );
  DDX_Text( pDX , IDC_GET_CONFIG_FILE_NAME , m_GetConfigScriptFileName );
  DDX_Text( pDX , IDC_STATUS , m_Status );
  DDX_Control( pDX , IDC_LOG , m_Log );
  DDX_Check( pDX , IDC_MOUSE_HOOK , m_bMouseHook );
}

BEGIN_MESSAGE_MAP(CLaserBurningConfigDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
  ON_REGISTERED_MESSAGE( dwMsgFromSimulator , OnMsgFromSimulator )
  ON_REGISTERED_MESSAGE( dwMouseMsg , OnMsgFromMouse )
  ON_BN_CLICKED( IDC_GET_CURRENT_CONFIG , &CLaserBurningConfigDlg::OnBnClickedGetCurrentConfig )
  ON_BN_CLICKED( IDC_ABORT_SCRIPT , &CLaserBurningConfigDlg::OnBnClickedAbortScript )
  ON_BN_CLICKED( IDC_MOUSE_HOOK , &CLaserBurningConfigDlg::OnBnClickedMouseHook )
  ON_BN_CLICKED( IDC_CLEAR_LOG , &CLaserBurningConfigDlg::OnBnClickedClearLog )
  ON_WM_TIMER()
  ON_BN_CLICKED( IDC_DO_CONFIGURATION , &CLaserBurningConfigDlg::OnBnClickedDoConfiguration )
  ON_BN_CLICKED( IDCRUN_TEST_SCRIPT , &CLaserBurningConfigDlg::OnBnClickedTestScript )
  ON_STN_CLICKED( IDC_STATION_STATUS , &CLaserBurningConfigDlg::OnStnClickedStationStatus )
  ON_STN_CLICKED( IDC_CONFIGURATION_STATUS , &CLaserBurningConfigDlg::OnStnClickedConfigurationStatus )
  ON_STN_DBLCLK( IDC_STATION_STATUS , &CLaserBurningConfigDlg::OnStnDblclickStationStatus )
END_MESSAGE_MAP()


// CLaserBurningConfigDlg message handlers

BOOL CLaserBurningConfigDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

  m_BlackLed.LoadBitmap( IDB_BLACK_LED ) ;
  m_BlueLed.LoadBitmap( IDB_BLUE_LED ) ;
  m_YellowLed.LoadBitmap( IDB_YELLOW_LED ) ;
  m_RedLed.LoadBitmap( IDB_RED_LED ) ;
  m_GreenLed.LoadBitmap( IDB_GREEN_LED ) ;

  ((CStatic*) GetDlgItem( IDC_STATUS_LED ))->SetBitmap( m_BlackLed ) ;

  m_Simulator.SetOwner( this->m_hWnd , dwMsgFromSimulator ) ;
  g_MouseHook.SetOwner( this->m_hWnd , dwMouseMsg ) ;

  CRect rcWinPos ;
  GetWindowRect( &rcWinPos ) ;

  m_iFullHeight = rcWinPos.Height() ;

  FXRegistry Reg( "FileX" ) ;

  m_DialogViewMode = ( ViewMode ) Reg.GetRegiInt(
    "LaserBurning" , "ViewMode(0-debug,1-work)" , VM_Debug ) ;

  if ( m_DialogViewMode == VM_Work )
    SetSmallSizeDlg() ;

  GetDlgItem( IDC_START_ADMIRAL )->EnableWindow(FALSE) ;

  SetDlgItemText( IDC_STATION_STATUS , "Get Configuration Result" ) ;
  SetDlgItemText( IDC_CONFIGURATION_STATUS , "Set Configuration Result" ) ;

  ::MessageBox( NULL , "Only one station can be connected \n"
    "to computer for configuration.\n"
    "Disconnect other stations is any." , "Program using restriction" , MB_OK ) ;

  CString PosAsText = Reg.GetRegiString( "LaserBurning" , "DialogPosition" , "1420,400" ) ;
  CPoint LeftTop ;
  sscanf( (LPCTSTR) PosAsText , "%d,%d" , &LeftTop.x , &LeftTop.y ) ;

  SetWindowPos( NULL , LeftTop.x , LeftTop.y , 0 , 0 , SWP_NOSIZE | SWP_SHOWWINDOW ) ;
  

  return TRUE;  // return TRUE  unless you set the focus to a control
}

void CLaserBurningConfigDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CLaserBurningConfigDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CLaserBurningConfigDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Automation servers should not exit when a user closes the UI
//  if a controller still holds on to one of its objects.  These
//  message handlers make sure that if the proxy is still in use,
//  then the UI is hidden but the dialog remains around if it
//  is dismissed.

void CLaserBurningConfigDlg::OnClose()
{
  WriteCurrentValues() ;
  if ( CanExit() )
		CDialogEx::OnClose();
}

void CLaserBurningConfigDlg::OnOK()
{
  WriteCurrentValues() ;
  if ( CanExit() )
		CDialogEx::OnOK();
}

void CLaserBurningConfigDlg::OnCancel()
{
  WriteCurrentValues() ;
  if ( CanExit() )
		CDialogEx::OnCancel();
}

BOOL CLaserBurningConfigDlg::CanExit()
{
	// If the proxy object is still around, then the automation
	//  controller is still holding on to this application.  Leave
	//  the dialog around, but hide its UI.
	if (m_pAutoProxy != nullptr)
	{
		ShowWindow(SW_HIDE);
		return FALSE;
	}

	return TRUE;
}

afx_msg LRESULT CLaserBurningConfigDlg::OnMsgFromSimulator( WPARAM wParam , LPARAM lParam )
{
  CString Msg( ( LPCTSTR ) lParam ) ;
  free( ( void* ) lParam ) ;
  switch ( wParam )
  {
  case KMS_DoStep:
    {
      ExecuteStep( Msg ) ;
      SetEvent( m_Simulator.m_hSyncHandle ) ;
    }
    break ;
  case KMS_ViewLog:
    {
      SetDlgItemText( IDC_STATION_STATUS , Msg ) ;
    }
    break ;
  case KMS_ViewDoConfigLog:
    {
      SetDlgItemText( IDC_CONFIGURATION_STATUS , Msg ) ;
    }
    break ;
  default:
    SetDlgItemText( IDC_STATUS , Msg ) ;
    Msg.Insert( 0 , "Simulator " ) ;
    m_Log.InsertString( 0 , Msg ) ;

  }

  return 0 ;
}

void CLaserBurningConfigDlg::ViewAndLog( LPCTSTR pSRc , LPCTSTR pMsg )
{
  CString Msg( pMsg ) ;

  SetDlgItemText( IDC_STATUS , Msg ) ;
  Msg.Insert( 0 , "Simulator " ) ;
  m_Log.InsertString( 0 , Msg ) ;
}

afx_msg LRESULT CLaserBurningConfigDlg::OnMsgFromMouse( WPARAM wParam , LPARAM lParam )
{
  CString Msg( ( LPCTSTR ) lParam ) ;

  SetDlgItemText( IDC_MOUSE_MSG , Msg ) ;
  Msg.Insert( 0 , "Mouse " ) ;
  m_Log.InsertString( 0 , Msg ) ;
  return 0 ;
}

int CLaserBurningConfigDlg::WriteCurrentValues()
{
  FXRegistry Reg( "FileX" ) ;

  Reg.WriteRegiInt(
    "LaserBurning" , "StationNum" , m_iStationForConfiguration ) ;
  Reg.WriteRegiString(
    "LaserBurning" , "ScriptName" , m_GetConfigScriptFileName ) ;
  return 0;
}

int CLaserBurningConfigDlg::RestoreLastValues()
{
  FXRegistry Reg( "FileX" ) ;

  m_iStationForConfiguration = Reg.GetRegiInt(
    "LaserBurning" , "StationNum" , 5 ) ;
  m_GetConfigScriptFileName = Reg.GetRegiString(
    "LaserBurning" , "ScriptName" , "c:/LBCfg/MoxaReadStatus.mks" ) ;
  return 0;
}


void CLaserBurningConfigDlg::OnBnClickedGetCurrentConfig()
{
  FXRegistry Reg( "FileX" ) ;

  m_GetConfigScriptFileName = Reg.GetRegiString(
    "LaserBurning" , "GetConfigScript" , "C:/LBCfg/LBSReadStatus.mks" ) ;
  SetDlgItemText( IDC_GET_CONFIG_FILE_NAME , m_GetConfigScriptFileName ) ;
  m_bViewGetConfig = true ;
  m_StationStatus.Empty() ;
  SetDlgItemText( IDC_STATION_STATUS , m_StationStatus ) ;
  OnBnClickedTestScript() ;
}

void CLaserBurningConfigDlg::OnBnClickedTestScript()
{
  GetDlgItemText( IDC_GET_CONFIG_FILE_NAME , m_GetConfigScriptFileName ) ;
  WriteCurrentValues() ;
  std::ifstream ScriptFile( m_GetConfigScriptFileName ) ;
  CString Script ;
  if ( ScriptFile.is_open() )
  {
    int Ch ;
    while ( !ScriptFile.eof() )
    {
      Ch = ScriptFile.get() ;
      Script += (TCHAR) Ch ;
    }
    ScriptFile.close() ;
    m_Simulator.StartScript( Script ) ;
    Sleep( 20 ) ;
    if ( m_Simulator.IsScriptRunning() )
      SetDlgItemText( IDC_STATUS , "Script started" ) ;
    else
      SetDlgItemText( IDC_STATUS , "Failed to start script" ) ;

  }
  else
    SetDlgItemText( IDC_STATUS , "Can't open file" ) ;
}

void CLaserBurningConfigDlg::OnBnClickedAbortScript()
{
  m_Simulator.AbortScript() ;
}

void CLaserBurningConfigDlg::OnBnClickedMouseHook()
{
  UINT State = ( ( CButton* ) GetDlgItem( IDC_MOUSE_HOOK ) )->GetState() ;
  m_bMouseHook = (State & 1) != 0 ;
  if ( m_bMouseHook )
  {
    g_MouseHook.InstallHook() ;
  }
  else
  {
    g_MouseHook.UninstallHook() ;
  }
}

void CLaserBurningConfigDlg::OnBnClickedClearLog()
{
  m_Log.ResetContent() ;
}

int CLaserBurningConfigDlg::ExecuteStep( CString Step )
{
  TRACE( "----------- Step: %s\n" , ( LPCTSTR ) Step ) ;

  Step.Trim() ;
  if ( Step.Find( "//" ) == 0 ) // comment
  {
    ViewAndLog( "Simulator " , Step ) ;
    return 0 ;
  }

  int iPos = 0 ;
  CString Command = Step.Tokenize( " ,;" , iPos ).MakeUpper() ;

  if ( Command == "FORLOG" )
  {
    CString Content = Step.Mid( iPos ) ;
    if ( m_bViewGetConfig )
    {
      m_StationStatus += Content ;
      m_Simulator.PostMsg( KMS_ViewLog , m_StationStatus ) ;
      return 1 ;
    }
    else if ( m_bViewDoConfig )
    {
      m_DoConfigurationStatus += Content ;
      m_Simulator.PostMsg( KMS_ViewDoConfigLog , m_StationStatus ) ;
      return 1 ;
    }
  }
  else if ( Command == "START" )
  {
    char * pOldValue = getenv( "__COMPAT_LAYER" ) ;
    int iRes = _putenv( "__COMPAT_LAYER=RunAsAdmin" ) ;
    // Format: START <full path to application>
    CString Application = Step.Mid( iPos ) ;
    char * pApp = ( char* ) ( LPCTSTR ) Application ;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si , sizeof( si ) );
    si.cb = sizeof( si );
    ZeroMemory( &pi , sizeof( pi ) );

    // Start the child process. 
    if ( !CreateProcess( Application ,   // No module name (use command line)
      NULL ,        // Command line
      NULL ,           // Process handle not inheritable
      NULL ,           // Thread handle not inheritable
      FALSE ,          // Set handle inheritance to FALSE
      0 ,              // No creation flags
      NULL ,           // Use parent's environment block
      NULL ,           // Use parent's starting directory 
      &si ,            // Pointer to STARTUPINFO structure
      &pi )           // Pointer to PROCESS_INFORMATION structure
      )
    {
      m_LastStatusMsg.Format( "CreateProcess %s failed (%d).\n" ,
        ( LPCTSTR ) Application , GetLastError() );
      TRACE( " %s\n" , ( LPCTSTR ) m_LastStatusMsg ) ;
      ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
      return 0 ;
    }
    Sleep( 800 ) ;
    m_LastStatusMsg.Format( "Application %s started" , ( LPCTSTR ) Application ) ;
    TRACE( " %s\n" , ( LPCTSTR ) m_LastStatusMsg ) ;
    ViewAndLog( "Simulator " , m_LastStatusMsg ) ;

    return 1 ;
  }
  else if ( Command == "STARTA" )
  {
    // Format: START <full path to application>
    CString Application = Step.Mid( iPos ) ;
    char * pApp = ( char* ) ( LPCTSTR ) Application ;
    system( (LPCTSTR)Application ) ;
    Sleep( 800 ) ;
    m_LastStatusMsg.Format( "Application %s started" , ( LPCTSTR ) Application ) ;
    TRACE( " %s\n" , ( LPCTSTR ) m_LastStatusMsg ) ;
    ViewAndLog( "Simulator " , m_LastStatusMsg ) ;

    return 1 ;
  }
  else if ( Command == "FINDWINDOW" )
  {
      // Format: FINDWINDOW <Window Caption>
    CString WindowCaption = Step.Mid( iPos ).Trim( " ,;" ) ;
    m_Simulator.m_hSelectedWindow = ::FindWindow( NULL , WindowCaption ) ;
    if ( m_Simulator.m_hSelectedWindow )
    {
      ::GetWindowRect( m_Simulator.m_hSelectedWindow , &m_Simulator.m_CurrentWindowRect ) ;
      m_LastStatusMsg.Format( "Window %s found, h=%p, Rect=%d,%d,%d,%d" , ( LPCTSTR ) WindowCaption ,
        m_Simulator.m_hSelectedWindow ,
        m_Simulator.m_CurrentWindowRect.left , m_Simulator.m_CurrentWindowRect.top ,
        m_Simulator.m_CurrentWindowRect.right , m_Simulator.m_CurrentWindowRect.bottom ) ;
      TRACE( " %s\n" , ( LPCTSTR ) m_LastStatusMsg ) ;
      ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
      return 1 ;
    }
    else
    {
      m_LastStatusMsg.Format( "Can't find window '%s'" , ( LPCTSTR ) WindowCaption ) ;
      ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
      return -1 ;
    }
  }
  else if ( (Command == "MOUSECLICK") || (Command == "RMOUSECLICK") )
  {
    ::ZeroMemory( &m_KeybMouseData , sizeof( m_KeybMouseData ) ) ;
    m_KeybMouseData.type = INPUT_MOUSE;
    bool bLeft = (Command == "MOUSECLICK") ;
    m_KeybMouseData.mi.dwFlags = bLeft ? 
      MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN ;
    int iRes = SendInput( 1 , &m_KeybMouseData , sizeof( m_KeybMouseData ) ) ;
    CPoint PtReadCursor ;
    GetCursorPos( &PtReadCursor ) ;
    HWND hWnd = ::WindowFromPoint( PtReadCursor ) ;
    m_LastStatusMsg.Format( "%s Res=%d h=%p Cur[%d,%d] " ,
      bLeft ? "MOUSECLICK" : "RMOUSECLICK", iRes ,
      hWnd , PtReadCursor.x , PtReadCursor.y ) ;

    SetTimer( bLeft ? TM_ReleaseLeftButton : TM_ReleaseRightButton , 50 , NULL ) ;
    return 1 ;
  }
  else if ( Command == "MOUSEMOVE" )
  {
    // Format: MOUSECLICK ABSOLUTE-or-RELATIVE (X,Y)
    // For Absolute - X and Y are integer
    // For Relative - X and Y are part of selected window size; range[0.0,1.0]
    CString Mode = Step.Tokenize( " ,;" , iPos ).MakeUpper() ;
    CString CoordinatesAsText = Step.Mid( iPos ).Trim() ;
    CoordinatesAsText.Remove( ' ' ) ;

    if ( CoordinatesAsText[ 0 ] == '(' )
      CoordinatesAsText.Delete( 0 ) ;
    double dX = atof( CoordinatesAsText ) ;
    int iCommaPos = CoordinatesAsText.Find( ',' ) ;
    if ( iCommaPos > 0 )
    {
      double dY = atof( ( ( LPCTSTR ) CoordinatesAsText ) + iCommaPos + 1 ) ;
      ClipCursor( NULL ) ;
      Sleep( 20 ) ;
      if ( Mode == "RELATIVE" )
      {
        CPoint CursorPos ;
        DWORD dwError = 0 ;
        GetCursorPos( &CursorPos ) ;
        double dAbsX = m_Simulator.m_CurrentWindowRect.left + /*m_CurrentWindowRect.Width() * */dX ;
        double dAbsY = m_Simulator.m_CurrentWindowRect.top + /*m_CurrentWindowRect.Height() * */dY ;
//         int iRes = SetCursorPos( (int)dAbsX , (int)dAbsY ) ;
        double fScreenWidth = ::GetSystemMetrics( SM_CXSCREEN ) - 1;
        double fScreenHeight = ::GetSystemMetrics( SM_CYSCREEN ) - 1;
        double fx = dAbsX * ( 65535.0 / fScreenWidth );
        double fy = dAbsY * ( 65535.0 / fScreenHeight );

        int iDx = ( int ) ( ( dAbsX - CursorPos.x ) / 10. ) ;
        int iDy = ( int ) ( ( dAbsY - CursorPos.y ) / 10. ) ;
        m_OrderedCursorPosition = CPoint( (int)dAbsX , (int)dAbsY ) ;
        ::ZeroMemory( &m_KeybMouseData , sizeof( m_KeybMouseData ) ) ;
        m_KeybMouseData.type = INPUT_MOUSE;
        m_KeybMouseData.mi.dx = ( long ) fx ; // desired X coordinate
        m_KeybMouseData.mi.dy = ( long ) fy ; // desired Y coordinate
        m_KeybMouseData.mi.dwFlags = MOUSEEVENTF_ABSOLUTE /*| MOUSEEVENTF_MOVE*/;
        SetTimer( TM_MoveCursorRelative , 10 , NULL ) ;
        return 1 ;
      }
      else if ( Mode == "ABSOLUTE" )
      {
        CPoint CursorPos ;
        DWORD dwError = 0 ;
        GetCursorPos( &CursorPos ) ;

        double dAbsX = dX ;
        double dAbsY = dY ;

        double fScreenWidth = ::GetSystemMetrics( SM_CXSCREEN ) - 1;
        double fScreenHeight = ::GetSystemMetrics( SM_CYSCREEN ) - 1;
        double fx = dAbsX * (65535.0 / fScreenWidth);
        double fy = dAbsY * (65535.0 / fScreenHeight);

        m_OrderedCursorPosition = CPoint( (int) dAbsX , (int) dAbsY ) ;
        ::ZeroMemory( &m_KeybMouseData , sizeof( m_KeybMouseData ) ) ;
        m_KeybMouseData.type = INPUT_MOUSE;
        m_KeybMouseData.mi.dx = (long) fx ; // desired X coordinate
        m_KeybMouseData.mi.dy = (long) fy ; // desired Y coordinate
        m_KeybMouseData.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
        SetTimer( TM_MoveCursorAbsolute , 10 , NULL ) ;
        CPoint Pt( (int) dX , (int) dY ) ;
        HWND hWnd = ::WindowFromPoint( Pt ) ;

        m_LastStatusMsg.Format( "  [%d,%d] h=%p" , ( int ) dX , ( int ) dY , hWnd ) ;
        m_LastStatusMsg.Insert( 0 , Step ) ;
        TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
        ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
        return 1 ;
      }
      else
      {
        m_LastStatusMsg.Format( "Bad MouseClick command format: '%s'" , ( LPCTSTR ) Step ) ;
        ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
        TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
      }
    }
    else
    {
      m_LastStatusMsg.Format( "Bad MouseClick coordinates: '%s'" , ( LPCTSTR ) Step ) ;
      ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
      TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
    }
    return -1 ;
  }
  else if ( Command == "GETTEXT" 
  || Command == "SELECT"
  || Command == "SELECTR"
  || Command == "SETTEXT" )
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
      TCHAR String[ 500 ] , String2[500] , String3[500];
      double dY = atof( ( ( LPCTSTR ) CoordinatesAsText ) + iCommaPos + 1 ) ;
//       dX = m_Simulator.m_CurrentWindowRect.left + /*m_CurrentWindowRect.Width() * */dX ;
//       dY = m_Simulator.m_CurrentWindowRect.top + /*m_CurrentWindowRect.Height() * */dY ;
      if ( Command == "SELECTR" )
      {
        dX += m_Simulator.m_CurrentWindowRect.left ;
        dY += m_Simulator.m_CurrentWindowRect.top ;
      }
      CPoint Pt( ( int ) dX , ( int ) dY ) ;
      HWND hWnd = ::WindowFromPoint( Pt ) ;
      int iLen = ::GetWindowText( hWnd , String , 500 ) ;
      WindowClassIsControl IsControl = IsWindowAControl( hWnd ) ;
      CRect WndRect ;
      ::GetWindowRect( hWnd , &WndRect ) ;
      CPoint InClient = Pt - WndRect.TopLeft() ;
      HWND hSelectedControl = ::ChildWindowFromPoint( hWnd , InClient ) ;
      HWND hControl = ::GetNextWindow( hSelectedControl , GW_HWNDNEXT ) ;
      if ( IsControl != WCIC_No )
        hControl = hWnd ;
      else
      {
        IsControl = IsWindowAControl( hSelectedControl ) ;
        if ( IsControl != WCIC_No )
          hControl = hSelectedControl ;
        else
          IsControl = IsWindowAControl( hControl ) ;
      }
      while ( hControl )
      {
        CRect WndRect ;
        ::GetWindowRect( hControl , &WndRect ) ;
        if ( WndRect.PtInRect( Pt ) )
        {
          if ( IsControl == WCIC_Combo )
          {
            int iSelected = ::SendMessage( hControl , CB_GETCURSEL , 0 , 0 ) ;
            if ( iSelected != CB_ERR )
            {
              iLen = ::SendMessage( hControl , CB_GETLBTEXT ,
                iSelected , (LPARAM) String3 ) ;

              if ( m_bViewGetConfig )
              {
                m_StationStatus += String3 ;
                if ( ::IsWindowEnabled( hControl ) )
                  m_StationStatus += '\n' ;
                else
                  m_StationStatus += "  " ;
                m_Simulator.PostMsg( KMS_ViewLog , m_StationStatus ) ;
              }
              else if ( m_bViewDoConfig )
              {
                m_DoConfigurationStatus += String3 ;
                if ( ::IsWindowEnabled( hControl ) )
                  m_DoConfigurationStatus += '\n' ;
                else
                  m_DoConfigurationStatus += "  " ;
                m_Simulator.PostMsg( KMS_ViewDoConfigLog , m_DoConfigurationStatus ) ;
              }
              m_LastStatusMsg = String3 ;
              m_LastStatusMsg.Insert( 0 , "Combo Box, Selected: " ) ;
              ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
              TRACE( " %s \n" , (LPCTSTR) m_LastStatusMsg ) ;

              if ( Command == "SELECT" || Command == "SELECTR" )
              {
                int iValuePos = Step.Find( "VALUE" ) ;
                if ( iValuePos > 0 )
                {
                  int iEqPos = Step.Find( '=' , iValuePos + 5 ) ;
                  if ( iEqPos > 0 )
                  {
                    CString Value = Step.Mid( iEqPos + 1 ) ;
                    Value.Trim() ;
                    int iNItems = ::SendMessage( hControl , CB_GETCOUNT , 0 , 0 ) ;
                    if ( iNItems )
                    {
                      for ( int i = 0 ; i < iNItems ; i++ )
                      {
                        iLen = ::SendMessage( hControl , CB_GETLBTEXT ,
                          i , (LPARAM) String2 ) ;
                        CString ItemText( String2 ) ;

                        if ( ItemText.Find( Value ) == 0 )
                        {
                          iLen = ::SendMessage( hControl , CB_SETCURSEL ,
                            i , 0 ) ;

                          m_LastStatusMsg += " Set to Value: " ;
                          m_LastStatusMsg += String2 ;
                          ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
                          TRACE( " %s \n" , (LPCTSTR) m_LastStatusMsg ) ;
                          return 1 ;
                        }
                      }
                    }
                  }
                }
              }
              return 1 ;
            }
          }
          else if ( IsControl == WCIC_List )
          {
            int iItemsPos = Step.Find( "VALUE" , iCommaPos ) ;
            CString Value ;
            if ( iItemsPos > 0 )
            {
              int iEqPos = Step.Find( '=' , iItemsPos + 5 ) ;
              if ( iEqPos > 0 )
                Value = Step.Mid( iEqPos + 1 ).Trim() ;
              if ( !Value.IsEmpty() )
              {
                int iNItems = ::SendMessage( hControl , LB_GETCOUNT , 0 , 0 ) ;
                for ( int i = 0 ; i < iNItems ; i++ )
                {
                  iLen = ::SendMessage( hControl , LB_GETTEXT ,
                    i , (LPARAM) String3 ) ;
                  CString ItemString( String3 ) ;
                  if ( ItemString.Find( Value ) >= 0 )
                  {
                    iLen = ::SendMessage( hControl , LB_SELITEMRANGE ,
                      FALSE , (LPARAM) ((iNItems - 1) << 16) ) ;
                    iLen = ::SendMessage( hControl , LB_SETSEL ,
                      TRUE , (LPARAM) i ) ;
                    m_LastStatusMsg += ", Selected " ;
                    m_LastStatusMsg += Value ;
                    ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
                    TRACE( " %s \n" , (LPCTSTR) m_LastStatusMsg ) ;
                    return 1 ;
                  }
                }
              }
            }
            int Selected[ 20 ] ;
            int iNSelected = ::SendMessage( hControl , 
              LB_GETSELITEMS , 20 , (LPARAM)Selected ) ;
            if ( iNSelected != CB_ERR )
            {
              CString SelectedAsText ;
              for ( int i = 0 ; i < iNSelected ; i++ )
              {
                iLen = ::SendMessage( hControl , LB_GETTEXT ,
                  Selected[i] , (LPARAM) String3 ) ;
                if ( i > 0 )
                  SelectedAsText += ',' ;
                SelectedAsText += String3 ;
                if ( m_bViewGetConfig )
                {
                  m_StationStatus += String3 ;
                  m_StationStatus += ": " ;
                  m_Simulator.PostMsg( KMS_ViewLog , m_StationStatus ) ;
                }
                else if ( m_bViewDoConfig )
                {
                  m_DoConfigurationStatus += String3 ;
                  m_DoConfigurationStatus += ": " ;
                  m_Simulator.PostMsg( KMS_ViewDoConfigLog , m_DoConfigurationStatus ) ;
                }
              }
              m_LastStatusMsg = SelectedAsText ;
              m_LastStatusMsg.Insert( 0 , "List Box, Selected: " ) ;
              ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
              TRACE( " %s \n" , (LPCTSTR) m_LastStatusMsg ) ;
              if ( Command == "SELECT" )
              {
                int iItemsPos = Step.Find( "ITEMS" , iCommaPos ) ;
                if ( iItemsPos > 0 )
                {
                  int iEqPos = Step.Find( '=' , iItemsPos + 5 ) ;
                  if ( iEqPos > 0 )
                  {
                    CString Value = Step.Mid( iEqPos + 1 ) ;
                    Value.Trim() ;
                    if ( Value.GetLength() )
                    {
                      int iNItems = ::SendMessage( hControl , LB_GETCOUNT , 0 , 0 ) ;
                      if ( iNItems )
                      {
                        if ( Value == "ALL" )
                        {
                          iLen = ::SendMessage( hControl , LB_SELITEMRANGE ,
                            TRUE , (LPARAM) ((iNItems - 1) << 16) ) ;
                          m_LastStatusMsg += ", All Selected " ;
                          ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
                          TRACE( " %s \n" , (LPCTSTR) m_LastStatusMsg ) ;
                        }
                        else if ( isdigit( Value[0] ) )
                        {
                          iLen = ::SendMessage( hControl , LB_SELITEMRANGE ,
                            FALSE , (LPARAM) ((iNItems - 1) << 16) ) ;
                          int iValue = atoi( Value ) ;
                          if ( iValue < iNItems )
                          {
                            iLen = ::SendMessage( hControl , LB_SETSEL ,
                              TRUE , (LPARAM) iValue ) ;
                            m_LastStatusMsg += ", Selected " ;
                            m_LastStatusMsg += Value ;
                            ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
                            TRACE( " %s \n" , (LPCTSTR) m_LastStatusMsg ) ;
                          }
                        }
                      }
                    }
                  }
                }
              }

              return 1 ;
            }
          }
          else if ( IsControl == WCIC_Edit )
          {
            
            ::SendMessage( hControl , WM_GETTEXT , 99 ,
              (LPARAM) String3 ) ;
            m_LastStatusMsg += " Read Value: " ;
            m_LastStatusMsg += String3 ;
            ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
            TRACE( " %s \n" , (LPCTSTR) m_LastStatusMsg ) ;

            if ( m_bViewGetConfig )
            {
              m_StationStatus += String3 ;
              m_StationStatus += '\n' ;
              m_Simulator.PostMsg( KMS_ViewLog , m_StationStatus ) ;
            }
            else if ( m_bViewDoConfig )
            {
              m_DoConfigurationStatus += String3 ;
              m_DoConfigurationStatus += ": " ;
              m_Simulator.PostMsg( KMS_ViewDoConfigLog , m_DoConfigurationStatus ) ;
            }
            int iValuePos = Step.Find( "VALUE" ) ;
            if ( iValuePos > 0 )
            {
              int iEqPos = Step.Find( '=' , iValuePos + 5 ) ;
              if ( iEqPos > 0 )
              {
                CString Value = Step.Mid( iEqPos + 1 ) ;
                Value.Trim() ;
                ::SendMessage( hControl , WM_SETTEXT , 0 , 
                  (LPARAM)(LPCTSTR)Value ) ;
                m_LastStatusMsg += " Set to Value: " ;
                m_LastStatusMsg += Value ;
                ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
                TRACE( " %s \n" , (LPCTSTR) m_LastStatusMsg ) ;
                if ( m_bViewDoConfig )
                {
                  m_DoConfigurationStatus += "Set to '" ;
                  m_DoConfigurationStatus += Value + "'\n" ;
                  m_Simulator.PostMsg( KMS_ViewDoConfigLog , m_DoConfigurationStatus ) ;
                }
              }
            }
            return 1 ;
          }
        }
        hControl = ::GetNextWindow( hControl , GW_HWNDNEXT ) ;
        if ( hControl )
          IsControl = IsWindowAControl( hControl ) ;
      }
      m_LastStatusMsg.Format( "No Result for %s" , ( LPCTSTR ) Step ) ;
      ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
      TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
    }
    else
    {
      m_LastStatusMsg.Format( "Bad command format: '%s'" , ( LPCTSTR ) Step ) ;
      ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
    }
    return -1 ;
  }
  else if ( Command == "KEYBSTROKE" )
  {
    CString Stroke = Step.Mid( iPos ) ;
    Stroke.Trim() ;
    int Code = ConvToBinary( Stroke ) ;
    ::ZeroMemory( &m_KeybMouseData , sizeof( m_KeybMouseData ) ) ;
    m_KeybMouseData.type = INPUT_KEYBOARD;
    m_KeybMouseData.ki.dwFlags = KEYEVENTF_UNICODE;
    m_KeybMouseData.ki.wScan = Code;
    SetTimer( TM_KeyStroke , 10 , NULL ) ;
    return 1 ;
  }
  else if ( Command == "KEYDOWN" )
  {
    CString Stroke = Step.Mid( iPos ) ;
    Stroke.Trim() ;
    int Code = ConvToBinary( Stroke ) ;
    ::ZeroMemory( &m_KeybMouseData , sizeof( m_KeybMouseData ) ) ;
    m_KeybMouseData.type = INPUT_KEYBOARD;
    //m_KeybMouseData.ki.dwFlags = KEYEVENTF_UNICODE;
    m_KeybMouseData.ki.wScan = Code;
    SendInput( 1 , &m_KeybMouseData , sizeof( INPUT ) ) ;
    m_LastStatusMsg = " Key Down: " ;
    m_LastStatusMsg += Stroke ;
    ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
    return 1 ;
  }
  else if ( Command == "KEYUP" )
  {
    CString Stroke = Step.Mid( iPos ) ;
    Stroke.Trim() ;
    int Code = ConvToBinary( Stroke ) ;
    ::ZeroMemory( &m_KeybMouseData , sizeof( m_KeybMouseData ) ) ;
    m_KeybMouseData.type = INPUT_KEYBOARD;
    m_KeybMouseData.ki.dwFlags = KEYEVENTF_KEYUP ;// KEYEVENTF_UNICODE;
    m_KeybMouseData.ki.wScan = Code;
    m_LastStatusMsg = " Key Up: " ;
    m_LastStatusMsg += Stroke ;
    ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
    SendInput( 1 , &m_KeybMouseData , sizeof( INPUT ) ) ;
    return 1 ;
  }
  else if ( Command == "KEYBSTROKES" )
  {
    while ( !m_Keystrokes.IsEmpty() ) ;
    m_Keystrokes = Step.Mid( iPos ) ;
    m_Keystrokes.Trim() ;
    SetTimer( TM_KeyStrokes , 10 , NULL ) ;
    return 1 ;
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
    ViewAndLog( "Simulator " , m_LastStatusMsg ) ;
    return 1 ;
  }
  else if ( Command == "ENDSCRIPT" )
  {
    if ( m_bViewGetConfig )
    {
      bool bConfigOK = IsGoodConfigurationResult( m_StationStatus ) ;
      m_bViewGetConfig = false ;
    }
  }
  return 0;
}

bool CLaserBurningConfigDlg::IsGoodConfigurationResult( LPCTSTR pResultAsText )
{
  CString ResultAsText( pResultAsText ) ;
  m_StationStatus = ResultAsText ;

  int iDT9850Station = 0 , iUSB202Station = 0 , iBaseCOMPort = INT_MAX ;
  bool bResult = false ;
#define NPORTS 8
  PortData Ports[ NPORTS ] ;

  
  int iPos = 0 ;
  CString Token = ResultAsText.Tokenize( "\n" , iPos ).Trim() ;
  while ( !Token.IsEmpty() )
  {
    if ( Token.Find( "DT" ) == 0 ) // Data Translation
    {
      for ( int i = Token.GetLength() - 1 ; i > 0 ; i--)
      {
        if ( isdigit(Token[i]) )
        {
          int iStationNum = atoi( (LPCTSTR)Token + i ) ;
          iDT9850Station = iStationNum ;
          break ;
        }
      }
    }
    else if ( Token.Find( "USB" ) == 0 ) 
    {
      for ( int i = Token.GetLength() - 1 ; i > 0 ; i-- )
      {
        if ( isdigit( Token[ i ] ) )
        {
          int iStationNum = atoi( ( LPCTSTR ) Token + i ) ;
          iUSB202Station = iStationNum ;
          break ;
        }
      }
    }
    else if ( Token.Find( "Port" ) == 0 )
    {
      int iPortIndex = atoi( (LPCTSTR)Token + 4 ) ;
      if ( (0 < iPortIndex)  && (iPortIndex <= NPORTS ) )
      {
        int iCOMPos = Token.Find( "COM" ) ;
        if ( iCOMPos > 0 )
        {
          int iComPort = atoi( (LPCTSTR)Token + iCOMPos + 3 ) ;
          Ports[ iPortIndex - 1 ].iPortNum = iComPort ;
          Ports[ iPortIndex - 1 ].bRS422 = (Token.Find("RS-422") > 0) ;
          if ( iComPort < iBaseCOMPort )
            iBaseCOMPort = iComPort ;
        }
      }
    }
    Token = ResultAsText.Tokenize( "\n" , iPos ).Trim() ;
  }
  if ( iDT9850Station == iUSB202Station )
  {
    int iBaseComForStation = ((iDT9850Station - 1) * 8) + 1 ;
    if ( iBaseCOMPort == iBaseComForStation )
    {
      bool bIsRS422 = true ;
      bool bAreAllCOMPorts = true ;
      for ( int i = 0 ; i < NPORTS ; i++)
      {
        bIsRS422 &= Ports[i].bRS422 ;
        bAreAllCOMPorts &= ( Ports[i].iPortNum == (iBaseCOMPort + i) ) ;
      }
      if ( !bIsRS422 )
      {
        m_StationStatus += "\nERROR:\nNOT ALL PORTS\n"
          "ARE RS-422. DO CONFIGURE!" ;
      }
      else if ( !bAreAllCOMPorts )
      {
        m_StationStatus += "\nERROR:\nNOT PROPER COM PORTS.\n"
          "DO CONFIGURE!" ;
      }
      else
      {
        m_iStationForConfiguration = GetDlgItemInt( IDC_STATION ) ;
        int iCheckedResult = IDOK ;
        if ( iDT9850Station != m_iStationForConfiguration )
        {
          CString Msg ;
          Msg.Format( "Current Selected Station %d is not equal\n"
            "to Configured Station %d. IS IT OK?" , 
            m_iStationForConfiguration , iDT9850Station ) ;
          iCheckedResult = ::MessageBox( this->m_hWnd , Msg , "Station numbers inconsistence" ,
            MB_OKCANCEL | MB_ICONQUESTION ) ;
        }
        if ( iCheckedResult == IDOK )
        {
          ((CStatic*) GetDlgItem( IDC_STATUS_LED ))->SetBitmap( m_GreenLed ) ;
        m_StationStatus += "\nCONFIGURATION IS OK." ;
          SetDlgItemInt( IDC_STATION , m_iStationForConfiguration = iDT9850Station ) ;
        GetDlgItem( IDC_START_ADMIRAL )->EnableWindow( TRUE ) ;
        bResult = true ; ;
        }
        else
        {
          ((CStatic*) GetDlgItem( IDC_STATUS_LED ))->SetBitmap( m_RedLed ) ;
          m_StationStatus += "\nCONFIGURATION IS OK." ;
          GetDlgItem( IDC_START_ADMIRAL )->EnableWindow( FALSE ) ;
        }
      }
    }
    else
    {
      m_StationStatus += "\nERROR:\nNOT PROPER BASE COM\n"
      "PORT NUMBER. DO CONFIGURE!" ;
    }
  }
  else
  {
    m_StationStatus += "\nERROR:\nNOT EQUAL DT9854 and USB202\n"
    "STATIONS IDs. DO CONFIGURE!" ;
  }
  GetDlgItem( IDC_START_ADMIRAL)->EnableWindow( bResult ) ;

  CString LogFileName = GetTimeStamp( "c:/LBCfg/LBSLog" , "" ) ;
  ResultAsText.Insert( 0 , GetTimeStamp( "LBS Configuration log at " , "\n" ) );

  std::ofstream ResultLog( LogFileName ) ;
  if ( ResultLog.is_open() )
  {
    ResultLog.write( ( LPCTSTR ) ResultAsText , ResultAsText.GetLength() ) ;
    ResultLog.close() ;
  }

  SetDlgItemText( IDC_STATION_STATUS , m_StationStatus ) ;

  return bResult ;
}

void CLaserBurningConfigDlg::OnTimer( UINT_PTR nIDEvent )
{
  int iRes = 0 ;
  KillTimer( nIDEvent ) ;


  switch( nIDEvent )
  {
  case TM_ReleaseLeftButton:
    {
      ::ZeroMemory( &m_KeybMouseData , sizeof( m_KeybMouseData ) ) ;
      m_KeybMouseData.type = INPUT_MOUSE;
      m_KeybMouseData.mi.dwFlags = MOUSEEVENTF_LEFTUP ;
      iRes = SendInput( 1 , &m_KeybMouseData , sizeof( m_KeybMouseData ) ) ;
      m_LastStatusMsg.Format( "%s " , GetEventName( nIDEvent ) ) ;
    }
    break ;
  case TM_ReleaseRightButton:
    {
      ::ZeroMemory( &m_KeybMouseData , sizeof( m_KeybMouseData ) ) ;
      m_KeybMouseData.type = INPUT_MOUSE;
      m_KeybMouseData.mi.dwFlags = MOUSEEVENTF_RIGHTUP ;
      iRes = SendInput( 1 , &m_KeybMouseData , sizeof( m_KeybMouseData ) ) ;
      m_LastStatusMsg.Format( "%s " , GetEventName( nIDEvent ) ) ;
    }
    break ;
    case TM_KeyStroke:
    {
      iRes = SendInput( 1 , &m_KeybMouseData , sizeof( m_KeybMouseData ) ) ;
      m_LastStatusMsg.Format( "%s KeyCode=%d(0x%X)" ,
        GetEventName( nIDEvent ) , m_KeybMouseData.ki.wVk ) ;
      m_KeybMouseData.type = INPUT_KEYBOARD;
      m_KeybMouseData.ki.dwFlags |= KEYEVENTF_KEYUP ;
      iRes = SendInput( 1 , &m_KeybMouseData , sizeof( m_KeybMouseData ) ) ;
    }
    break ;
    case TM_KeyStrokes:
    {
      int iSentCount = 0 ;
      CString Saved = m_Keystrokes ;
      std::vector<INPUT> vec;
      while ( Saved.GetLength() )
      {
        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.dwFlags = KEYEVENTF_UNICODE;
        input.ki.wScan = Saved[0];
        vec.push_back( input );

        input.ki.dwFlags |= KEYEVENTF_KEYUP;
        vec.push_back( input );
        Saved.Delete(0) ;
      }
      SendInput( vec.size() , vec.data() , sizeof( INPUT ) );

      m_LastStatusMsg.Format( "%s String=%s #=%d" ,
        GetEventName( nIDEvent ) , m_Keystrokes , iSentCount ) ;
        m_Keystrokes.Empty() ;
    }
    break ;
    case TM_MoveCursorRelative:
    case TM_MoveCursorAbsolute:
      iRes = SetCursorPos( m_OrderedCursorPosition.x , m_OrderedCursorPosition.y ) ;
    default:
    {
      CPoint PtReadCursor ;
      GetCursorPos( &PtReadCursor ) ;
      HWND hWnd = ::WindowFromPoint( PtReadCursor ) ;
      m_LastStatusMsg.Format( "  [%d,%d] h=%p Cur[%d,%d]" ,
        m_KeybMouseData.mi.dx , m_KeybMouseData.mi.dy ,
        hWnd , PtReadCursor.x , PtReadCursor.y ) ;
      m_LastStatusMsg.Insert( 0 , GetEventName( nIDEvent ) ) ;
    }
  }
  if ( !iRes )
    iRes = GetLastError() ;
  TRACE( " %s \n" , ( LPCTSTR ) m_LastStatusMsg ) ;
  ViewAndLog( "Simulator " , m_LastStatusMsg ) ;

  CDialogEx::OnTimer( nIDEvent );
}


void CLaserBurningConfigDlg::OnBnClickedDoConfiguration()
{
  if ( !m_Simulator.m_bScriptIsRunning )
  {
    FXRegistry Reg( "FileX" ) ;

    CString m_DoConfigScriptFileName = Reg.GetRegiString(
      "LaserBurning" , "SetConfigScript" , "C:/LBCfg/DoConfiguration.mks" ) ;

    CString USB202Format = Reg.GetRegiString(
      "LaserBurning" , "USB202_NameFormat" , "Station%d" ) ;
    CString DT9850Format = Reg.GetRegiString(
      "LaserBurning" , "DT9850_NameFormat" , "DT9854(Station%d)" ) ;
    CString BasePortFormat = Reg.GetRegiString(
      "LaserBurning" , "BasePort_NameFormat" , "COM%d" ) ;

    std::ifstream ScriptFile( m_DoConfigScriptFileName ) ;
    CString Script ;
    if ( ScriptFile.is_open() )
    {
      int Ch ;
      while ( !ScriptFile.eof() )
      {
        Ch = ScriptFile.get() ;
        Script += (TCHAR) Ch ;
      }
      ScriptFile.close() ;

      m_iStationForConfiguration = GetDlgItemInt( IDC_STATION ) ;
      CString USB202Name ;
      USB202Name.Format( USB202Format , m_iStationForConfiguration ) ;

      CString DT9854Name ;
      DT9854Name.Format( DT9850Format , m_iStationForConfiguration ) ;

      CString MOXAFirstPort ;
      MOXAFirstPort.Format( BasePortFormat , ((m_iStationForConfiguration - 1) * 8) + 1 ) ;

      Script.Replace( "%USB202Name" , USB202Name ) ;
      Script.Replace( "%DTName" , DT9854Name ) ;
      Script.Replace( "%BaseComPort" , MOXAFirstPort ) ;
      std::ofstream OScriptFile( "c:/LBCfg/LastSetConfigurationFile.mks" ) ;
      if ( OScriptFile.is_open() )
      {
        OScriptFile.write( (LPCTSTR) Script , Script.GetLength() ) ;
        OScriptFile.close() ;
      }
      SetDlgItemText( IDC_CONFIGURATION_STATUS , "" ) ;
      m_DoConfigurationStatus.Empty() ;
      m_bViewDoConfig = true ;
      m_Simulator.StartScript( Script ) ;
      Sleep( 20 ) ;
      if ( m_Simulator.IsScriptRunning() )
        SetDlgItemText( IDC_STATUS , "Script started" ) ;
      else
        SetDlgItemText( IDC_STATUS , "Failed to start script" ) ;
    }
    else
      SetDlgItemText( IDC_STATUS , "Can't open file" ) ;
  }
}

void CLaserBurningConfigDlg::SetSmallSizeDlg()
{
  CRect rcWinPos ;
  GetWindowRect( &rcWinPos ) ;
  CRect rcStatusPos ;
  GetDlgItem( IDC_STATUS )->GetWindowRect( &rcStatusPos ) ;
  rcWinPos.bottom = rcStatusPos.bottom + 20 ;
  SetWindowPos( NULL , rcWinPos.left , rcWinPos.top ,
    rcWinPos.Width() , rcWinPos.Height() , SWP_NOMOVE ) ;

  m_DialogViewMode = VM_Work ;
}

void CLaserBurningConfigDlg::SetFullSizeDlg()
{
  CRect rcWinPos ;
  GetWindowRect( &rcWinPos ) ;
  CRect rcStatusPos ;
  rcWinPos.bottom = rcWinPos.top + m_iFullHeight ;
  SetWindowPos( NULL , rcWinPos.left , rcWinPos.top ,
    rcWinPos.Width() , rcWinPos.Height() , SWP_NOMOVE ) ;

  m_DialogViewMode = VM_Debug ;
}

void CLaserBurningConfigDlg::OnStnClickedStationStatus()
{
  if ( GetAsyncKeyState( VK_CONTROL ) & 0x8000 )
    SetSmallSizeDlg() ;
}


void CLaserBurningConfigDlg::OnStnClickedConfigurationStatus()
{
  if ( GetAsyncKeyState( VK_CONTROL ) & 0x8000 )
    SetFullSizeDlg() ;
}


void CLaserBurningConfigDlg::OnStnDblclickStationStatus()
{
  if ( GetAsyncKeyState( VK_CONTROL ) & 0x8000 )
    SetSmallSizeDlg() ;
}
