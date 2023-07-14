// FXRegistry.cpp: implementation of the FXRegistry class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "FXRegistry.h"

static LPCTSTR Delimiters = _T( " ,\t\r\n()[]{}" ) ;
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FXRegistry::FXRegistry()
{
  InitKey( _T("FileX\\Untitled") ) ; 
  m_hkRootKeyForInstance = HKEY_CURRENT_USER ;
}


FXRegistry::FXRegistry(LPCTSTR szRootKeyName , HKEY hkRootKey)
{
  InitKey( szRootKeyName ) ;
  m_hkRootKeyForInstance = hkRootKey ;
}

FXRegistry::~FXRegistry()
{

}

HKEY FXRegistry::FormKeyName( TCHAR * pBuf , int iBufLenElements , TCHAR * pName )
{
  if ( _tcscpy_s( pBuf , iBufLenElements , _T("Software\\") ) )
    return NULL ;
  if ( _tcscat_s( pBuf , iBufLenElements , m_KeyPrefix ) )
    return NULL;
  if ( _tcscat_s( pBuf , iBufLenElements , _T("\\") ) )
    return NULL;
  if ( _tcscat_s( pBuf , iBufLenElements , pName ) )
    return NULL;
  HKEY Key ;
  int iRes = RegOpenKeyEx( m_hkRootKeyForInstance ,
    pBuf , 0 , KEY_ALL_ACCESS ,
    &Key ) ;
  if ( iRes != ERROR_SUCCESS )
  {                                  
    DWORD dwDisposition = 0 ;  
    TRACE( "\nCan't open Registry key %s" , pBuf)  ;
    iRes = RegCreateKeyEx( m_hkRootKeyForInstance , pBuf , 0 , NULL ,
      REG_OPTION_NON_VOLATILE , KEY_ALL_ACCESS , NULL , &Key , 
      &dwDisposition ) ;
    if ( iRes == ERROR_SUCCESS )
    {
      m_KeyCreated = true ;
      return Key ;
    }
  }
  m_KeyCreated = false ;
  return ( iRes == ERROR_SUCCESS ) ? Key : NULL ;
}

int FXRegistry::GetRegiInt(
  LPCTSTR Key, LPCTSTR ValueName, int iDefault)
{
  DWORD iResult = 0xcdcdcdcd ;
  HKEY FileCompanyKey ;
  TCHAR Buf[2000] ;
  if ( (FileCompanyKey = FormKeyName( Buf , 2000 , (TCHAR*) Key)) == NULL )
    return iDefault ;

  DWORD dwType = REG_DWORD ;
  DWORD Len = 4 ;
  int iRes ;
  if ( m_KeyCreated )
  {
    iRes = RegSetValueEx( 
      FileCompanyKey ,
      ValueName , 
      0 , 
      REG_DWORD ,
      ( BYTE * )&iDefault ,
      4 ) ;
    iResult = iDefault ;
  }
  else
  {
    iRes = RegQueryValueEx( 
    FileCompanyKey ,
    ValueName , 
    0 , 
    &dwType ,
    ( unsigned char * )&iResult ,
    &Len ) ;
  if ( iRes != ERROR_SUCCESS  ||  dwType != REG_DWORD )
  {
    if ( dwType != REG_DWORD )
      RegDeleteKey( FileCompanyKey , ValueName ) ;

    iRes = RegSetValueEx( 
      FileCompanyKey ,
      ValueName , 
      0 , 
      REG_DWORD ,
      ( BYTE * )&iDefault ,
      4 ) ;
    if ( iRes != ERROR_SUCCESS )
    {
      TRACE( "\nCan't write default value %s into Registry %s" , ValueName , Buf) ;
    }
    
    iResult = iDefault ;
  }
  }

  RegCloseKey( FileCompanyKey ) ;

  return iResult ;

}

void FXRegistry::InitKey(LPCTSTR szNewKeyPrefix , HKEY hkRootKey )
{
  _tcscpy( m_KeyPrefix , szNewKeyPrefix ) ;
  m_hkRootKeyForInstance = hkRootKey ;
}

void FXRegistry::WriteRegiInt(
  LPCTSTR Key, LPCTSTR ValueName, int Value)
{
  HKEY FileCompanyKey ;
  TCHAR Buf[2000] ;
  if ( (FileCompanyKey = FormKeyName( Buf , 2000 , (TCHAR*) Key)) == NULL )
    return ;

  int iRes = RegSetValueEx( 
    FileCompanyKey ,
    ValueName , 
    0 , 
    REG_DWORD ,
    ( BYTE * )&Value ,
    4 ) ;

  if ( iRes != ERROR_SUCCESS )
  {
    TRACE( "\nCan't write %s into Registry %s" , ValueName , Buf ) ;
  }
  RegCloseKey( FileCompanyKey ) ;
}


TCHAR *  FXRegistry::GetRegiString(
    LPCTSTR Key, LPCTSTR ValueName, LPCTSTR  Default) 
{
  HKEY FileCompanyKey ;
  TCHAR Buf[2000] ;
  _tcscpy( m_TmpBuffer , Default ) ;
  if ( (FileCompanyKey = FormKeyName( Buf , 2000 , (TCHAR*) Key)) == NULL )
  {
    return  (TCHAR*)&m_TmpBuffer ;
  }
  DWORD dwType = REG_SZ ;
  DWORD Len = sizeof( m_TmpBuffer ) ;

  if ( m_KeyCreated )
  {
    int     iRes = RegSetValueEx( 
      FileCompanyKey ,
      ValueName , 
      0 , 
      REG_SZ ,
      ( BYTE * )Default ,
      (DWORD) (_tcslen( Default ) + 1) * sizeof(TCHAR) ) ;
  }
  else
  {
  int iRes = RegQueryValueEx( 
    FileCompanyKey ,
    ValueName , 
    0 , 
    &dwType ,
    ( BYTE * )m_TmpBuffer ,
    &Len ) ;

  if ( iRes == ERROR_MORE_DATA )
  {
    BYTE * buf1 = (BYTE*)malloc( Len + 2 );
		
		iRes = RegQueryValueEx( 
      FileCompanyKey ,
      ValueName , 
      0 , 
      &dwType ,
      buf1 ,
      &Len ) ;

    memcpy( m_TmpBuffer , buf1 , sizeof(m_TmpBuffer) - 1 ) ;
		m_TmpBuffer[ 2047 ] = 0 ;

    free( buf1 ) ;
  }
  else if ( (iRes != ERROR_SUCCESS  ||  dwType != REG_SZ )
      &&  Default )
  {
      if ( iRes == ERROR_SUCCESS  &&  dwType != REG_SZ )
    {
      //TRACE( "Value of %s in REGISTRY is not string" , ValueName ) ;
      RegDeleteKey( FileCompanyKey , ValueName ) ;
    }

    iRes = RegSetValueEx( 
      FileCompanyKey ,
      ValueName , 
      0 , 
      REG_SZ ,
      ( BYTE * )Default ,
      (DWORD) (_tcslen( Default ) + 1) * sizeof(TCHAR) ) ;

    if ( iRes != ERROR_SUCCESS )
    {
      TRACE( "\nCan't write default value %s into Registry %s" , ValueName , Buf) ;
    }
    
    _tcscpy( m_TmpBuffer , Default ) ;
  }
	else
		m_TmpBuffer[ Len ] = 0 ;
  }
      
  RegCloseKey( FileCompanyKey ) ;

  return m_TmpBuffer ;
}

double  FXRegistry::GetRegiDouble(
      LPCTSTR Key, LPCTSTR ValueName, double dDefault) 
{
  TCHAR szDef[ 100 ] ;
  double dResult = 1e30 ;
  _stprintf_s( szDef , _T("%g") , dDefault ) ;

  _stscanf( GetRegiString( Key , ValueName , szDef ) , _T("%lg") , &dResult ) ;

  return dResult ;
};

__int64  FXRegistry::GetRegiInt64(
   LPCTSTR Key, LPCTSTR ValueName, __int64 i64Default) 
{
  TCHAR szDef[ 100 ] ;
  __int64 i64Result = 111111111111 ;
  _stprintf_s( szDef , _T("%I64d") , i64Default ) ;

  _stscanf( GetRegiString( Key , ValueName , szDef ) , _T("%I64d") , &i64Result ) ;

  return i64Result ;
};

void    FXRegistry::WriteRegiString(
  LPCTSTR Key, LPCTSTR ValueName, LPCTSTR  Value) 
{
  HKEY FileCompanyKey ;
  TCHAR Buf[2000] ;
  if ( (FileCompanyKey = FormKeyName( Buf , 2000 , (TCHAR*) Key)) == NULL )
    return ;

  int iRes = RegSetValueEx( 
      FileCompanyKey ,
      ValueName , 
      0 , 
      REG_SZ ,
      ( const BYTE * )Value ,
       (DWORD) (_tcslen( Value ) + 1) * sizeof(TCHAR) ) ;

  if ( iRes != ERROR_SUCCESS )
  {
    TRACE( "\nCan't write value %s into Registry %s" , ValueName , Buf) ;
  }
  RegCloseKey( FileCompanyKey ) ;
};

void    FXRegistry::WriteRegiDouble(
  LPCTSTR Key, LPCTSTR ValueName, double dValue , LPCTSTR pFormat )
{
  TCHAR szDef[ 100 ] ;
  _stprintf_s( szDef , pFormat ? pFormat : _T("%g") , dValue ) ;

  WriteRegiString( Key , ValueName , szDef ) ;
};

void    FXRegistry::WriteRegiInt64(
                                   LPCTSTR Key, LPCTSTR ValueName, __int64 i64Val )
{
  TCHAR szDef[ 100 ] ;
  _stprintf_s( szDef , _T("%I64d") , i64Val ) ;

  WriteRegiString( Key , ValueName , szDef ) ;
};

void    FXRegistry::AddToMultiString(
  LPCTSTR Key, LPCTSTR ValueName, LPCTSTR  Value )
{
  HKEY FileCompanyKey ;
  DWORD dwType ;
  BYTE buf[ 2048 ] ;
  DWORD iLen = 2048 ;
  TCHAR Buf[2000] ;
  if ( (FileCompanyKey = FormKeyName( Buf , 2000 , (TCHAR*) Key)) == NULL )
    return ;

	int iRes = RegQueryValueEx( 
    FileCompanyKey ,
    ValueName , 
    0 , 
    &dwType ,
    buf ,
    &iLen ) ;

  if ( iRes != ERROR_SUCCESS )
  {
    iRes = RegSetValueEx( 
      FileCompanyKey ,
      ValueName , 
      0 , 
      REG_MULTI_SZ ,
      ( const BYTE * )Value ,
       (DWORD) (_tcslen( Value ) + 1) * sizeof(TCHAR) ) ;
  }
  else if ( dwType == REG_MULTI_SZ )
  {
    int iPtr = 0 ;
    int iLastLen ;
    int ValueLen = (int) _tcslen( Value ) ;
    TCHAR Upper[200] ;
    _tcscpy( Upper , Value ) ;
    _tcsupr( Upper ) ;
    while ( iLastLen = (int) _tcslen( (LPCTSTR)(buf + iPtr) ) )
    {
      if ( iLastLen == ValueLen )
      {
        TCHAR Up1[200] ;
        _tcscpy( Up1 , (LPCTSTR)(buf + iPtr) ) ;
        _tcsupr( Up1 ) ;

        if ( !_tcscmp( Up1 , Upper ) ) // Name Already exists
        {
          iPtr = -1 ;
          break ;
        }
      }
      iPtr += iLastLen + 1 ; // plus terminating zero
    }
    if ( iPtr >= 0 )
    {
      _tcscpy( (TCHAR *)(buf + iPtr) , Value ) ;
      buf[ iPtr + ValueLen + 1 ] = 0 ;
      iRes = RegSetValueEx( 
        FileCompanyKey ,
        ValueName , 
        0 , 
        REG_MULTI_SZ ,
        ( const BYTE * )buf ,
        (iPtr + ValueLen + 2) * sizeof(TCHAR) ) ;
    }

  }

  RegCloseKey( FileCompanyKey ) ;
};


  // Gets string from registry and parse into array of integers
  // returns amount of converted numbers
  // 
int FXRegistry::GetRegiIntSerie( 
  LPCTSTR  Key , LPCTSTR  ValueName , 
  int * iResultArray, 
  int iArrayLength // if not equal to returned value - not enogh space in array
                           // In this case this variable will be a length of necessary
                           // array
  )
{
  TCHAR * p = GetRegiString( Key , ValueName , _T("") ) ;

  if ( !p  ||  !(*p) )
    return 0 ;

  TCHAR * tok = _tcstok( ( TCHAR * )p , _T(" ,") );

  int iConverted = 0 ;

  while ( tok )
  {
    
    DWORD dwNewNumber = 0xffffffff ;
    int iRes ;
    if ( (tok[0] == _T('0'))  &&  (_totlower(tok[1]) == _T('x')) )
      iRes = _stscanf( tok + 2 , _T("%x") , &dwNewNumber ) ;
    else 
      iRes = _stscanf( tok , _T("%d") , &dwNewNumber ) ;

    if ( iRes )
    {
      if ( iConverted < iArrayLength )
        iResultArray[ iConverted ] = dwNewNumber ;

      iConverted++ ;

      tok = _tcstok( NULL , _T(" ,") ) ;
    }
    else
      break ;
  }

  return iConverted;
};  

// Gets string from registry and parse into complex number
// returns true if real and/or imag parts scanned properly
// else returns false 
// bool FXRegistry::GetRegiCmplx(
//   LPCTSTR  Key , LPCTSTR  ValueName , cmplx& cResult , cmplx& cDefault )
// {
//   TCHAR * p = GetRegiString( Key , ValueName , _T( "" ) ) ;
// 
//   if ( !p || !(*p) )
//   {
//     cResult = cDefault ;
//     WriteRegiCmplx( Key , ValueName , cResult ) ;
//     return true ;
//   }
// 
//   TCHAR * tok = _tcstok( (TCHAR *) p , Delimiters );
// 
//   int iConverted = 0 ;
//   cmplx cRes ;
//   int iRes = _stscanf( tok , _T( "%lg" ) , (double*)&cRes ) ;
//   tok = _tcstok( NULL , Delimiters ) ;
//   if ( tok )
//   {
//     iRes += _stscanf( tok , _T( "%lg" ) , ((double*) &cRes) + 1 ) ;
//   }
//   if ( iRes )
//     cResult = cRes ;
// 
//   return iRes != 0 ;
// };

// Gets string from registry and parse into complex number
// returns true if real and/or imag parts scanned properly
// else returns false 
// void FXRegistry::WriteRegiCmplx(
//   LPCTSTR Key , LPCTSTR ValueName , cmplx& cVal )
// {
//   _stprintf_s( m_TmpBuffer , "%16g,%16g" , cVal.real() , cVal.imag() ) ;
//   WriteRegiString( Key , ValueName , m_TmpBuffer ) ;
// };
// 
// int FXRegistry::EnumerateFolderForSubfolders( LPCTSTR pFolderName , FXStringArray& Items )
// {
//   HKEY hKey ;
// 
//   TCHAR Buf[ 2000 ] ;
//   if ( (hKey = FormKeyName( Buf , 2000 , (TCHAR*) pFolderName )) == NULL )
//     return 0 ;
// 
// 
//   
//   DWORD    cbName;                   // size of name string 
//   TCHAR    achClass[ MAX_PATH ] = TEXT( "" );  // buffer for class name 
//   DWORD    cchClassName = MAX_PATH;  // size of class string 
//   DWORD    cSubKeys = 0;               // number of subkeys 
//   DWORD    cbMaxSubKey;              // longest subkey size 
//   DWORD    cchMaxClass;              // longest class string 
//   DWORD    cValues;              // number of values for key 
//   DWORD    cchMaxValue;          // longest value name 
//   DWORD    cbMaxValueData;       // longest value data 
//   DWORD    cbSecurityDescriptor; // size of security descriptor 
//   FILETIME ftLastWriteTime;      // last write time 
// 
//   DWORD i , retCode;
// 
//   DWORD cchValue = MAX_VALUE_NAME;
// 
//   // Get the class name and the value count. 
//   retCode = RegQueryInfoKey(
//     hKey ,                    // key handle 
//     achClass ,                // buffer for class name 
//     &cchClassName ,           // size of class string 
//     NULL ,                    // reserved 
//     &cSubKeys ,               // number of subkeys 
//     &cbMaxSubKey ,            // longest subkey size 
//     &cchMaxClass ,            // longest class string 
//     &cValues ,                // number of values for this key 
//     &cchMaxValue ,            // longest value name 
//     &cbMaxValueData ,         // longest value data 
//     &cbSecurityDescriptor ,   // security descriptor 
//     &ftLastWriteTime );       // last write time 
// 
// // Enumerate the subkeys, until RegEnumKeyEx fails.
// 
//   if ( cSubKeys )
//   {
//     //printf( "\nNumber of subkeys: %d\n" , cSubKeys );
// 
//     for ( i = 0; i < cSubKeys; i++ )
//     {
//       BYTE KeyName[ MAX_KEY_LENGTH * 2 ];   // buffer for subkey name
//       TCHAR Class[ 200 ] ;
//       DWORD dwClassLen = 199 ;
//       cbName = MAX_KEY_LENGTH;
//       retCode = RegEnumKeyEx( hKey , i , (TCHAR*)KeyName , &cbName , NULL , Class , &dwClassLen , &ftLastWriteTime );
//       if ( retCode == ERROR_SUCCESS )
//       {
//         FXString NewSubfolder( (LPCTSTR)KeyName ) ;
//         Items.Add( NewSubfolder ) ;
//       }
//     }
//   }
//   
//   return (int)Items.Count() ;
// }
