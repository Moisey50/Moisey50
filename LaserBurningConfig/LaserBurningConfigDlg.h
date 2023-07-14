
// LaserBurningConfigDlg.h : header file
//

#pragma once
#include "FXRegistry.h"
#include "KeybMouseSimulator.h"
#include "hook.h"

enum TimerMsg
{
  TM_MoveCursorRelative = 10 ,
  TM_MoveCursorAbsolute ,
  TM_ClickLeftButton ,
  TM_ReleaseLeftButton ,
  TM_ClickRightButton ,
  TM_ReleaseRightButton ,
  TM_KeyStroke ,
  TM_ReleaseKeyStroke ,
  TM_KeyStrokes
};

enum WindowClassIsControl
{
  WCIC_No = 0 ,
  WCIC_Combo ,
  WCIC_List ,
  WCIC_Edit
};

enum ViewMode
{
  VM_Debug = 0 ,
  VM_Work
};

class PortData
{
public:
  int iPortNum = -1 ;
  bool bRS422 = false ;
} ;

class CLaserBurningConfigDlgAutoProxy;


// CLaserBurningConfigDlg dialog
class CLaserBurningConfigDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CLaserBurningConfigDlg);
	friend class CLaserBurningConfigDlgAutoProxy;

// Construction
public:
	CLaserBurningConfigDlg(CWnd* pParent = nullptr);	// standard constructor
	virtual ~CLaserBurningConfigDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LASERBURNINGCONFIG_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	CLaserBurningConfigDlgAutoProxy* m_pAutoProxy;
	HICON        m_hIcon;
  std::thread  * m_pMouseControlThread = NULL ;
  HANDLE       m_hMouseControlEvent ;
  HANDLE       m_hMouseControlExecutedEvent ;
  INPUT        m_KeybMouseData ;
  CPoint       m_OrderedCursorPosition ;
  CString      m_Keystrokes ;
  int          m_Timer = 0 ;
  ViewMode     m_DialogViewMode = VM_Debug ;
  int          m_iFullHeight ;
  CListBox     m_Log;
  BOOL         m_bMouseHook = FALSE ;
  BOOL         m_bMouseControlThreadExit = FALSE ;
  int          m_iStationForConfiguration;
  KeybMouseSimulator m_Simulator ;
  CString      m_GetConfigScriptFileName;
  CString      m_Status;
  CString      m_StationStatus ;
  CString      m_DoConfigurationStatus ;
  bool         m_bViewGetConfig = false ;
  bool         m_bViewDoConfig = false ;
  CString      m_LastStatusMsg ;
  CBitmap      m_BlackLed , m_GreenLed , m_YellowLed , m_BlueLed , m_RedLed ;

  BOOL         CanExit();

  static LONG MouseControlLoop( CLaserBurningConfigDlg * pHost ) ;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
  afx_msg LRESULT OnMsgFromSimulator( WPARAM wParam , LPARAM lParam );
  afx_msg LRESULT OnMsgFromMouse( WPARAM wParam , LPARAM lParam );
  void ViewAndLog( LPCTSTR pSRc , LPCTSTR Msg ) ;
  virtual void OnOK();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP()
public:
  int WriteCurrentValues();
  int RestoreLastValues();
  afx_msg void OnBnClickedGetCurrentConfig();
  afx_msg void OnBnClickedAbortScript();
  afx_msg void OnBnClickedMouseHook();
  afx_msg void OnBnClickedClearLog();
  int  ExecuteStep( CString Step ) ;
  void SetFullSizeDlg() ;
  void SetSmallSizeDlg() ;
  bool IsGoodConfigurationResult( LPCTSTR pResultAsText ) ;

  afx_msg void OnTimer( UINT_PTR nIDEvent );
  afx_msg void OnBnClickedDoConfiguration();
  afx_msg void OnBnClickedTestScript();
  afx_msg void OnStnClickedStationStatus();
  afx_msg void OnStnClickedConfigurationStatus();
  afx_msg void OnStnDblclickStationStatus();
};
