//////////////////////////////////////////////////////////////////////////////////////////////
//
// ウインドウタイトルのログをとりつづけるツール
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"


#ifdef __BORLANDC__
#pragma hdrstop
#endif


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <psapi.h>
#include <tlhelp32.h>
#include "Shlwapi.h"
#include <string>

#include "wx/taskbar.h"
#include "wx/timer.h"

#include "JobGraph.h"

#if 0 //__WXDEBUG__
const int TIMERPERIOD = 1*1000; // [milliseconds]
#else
const int TIMERPERIOD = 60*1000; // [milliseconds]
#endif



MyDialog   *dialog = NULL;

BOOL GetFileNameFromHwnd(HWND hWnd, LPTSTR lpszFileName, DWORD nSize);


class MyTimer : public wxTimer
{
public:
    virtual void Notify();
};



void
MyTimer::Notify()
{

    SYSTEMTIME st_filename;
    char       filename[256];

    GetLocalTime(&st_filename);

    sprintf( filename,
             "windowlog_%02d_%02d.csv",
             st_filename.wMonth,
             st_filename.wDay           );
    
    
    FILE * f;
    f = fopen( filename, "a" );
    assert( f );

    char buf[512];

    HWND fgwindow = ::GetForegroundWindow();
    ::GetWindowText( fgwindow, buf, sizeof(buf) );

    char exepath[MAX_PATH];
    char* exefile;
    GetFileNameFromHwnd( fgwindow, exepath, sizeof(exepath) );
    exefile = PathFindFileName( exepath );

    SYSTEMTIME st;
    GetLocalTime(&st);

    fprintf( f, "%02d:%02d,% 16s, %s\n", st.wHour, st.wMinute, exefile, buf );

    fclose( f );

}


MyTimer * timer;

IMPLEMENT_APP(MyApp)

    bool MyApp::OnInit(void)
{
    wxIcon icon ( wxT("jobgraph_icon") );

    if ( ! m_taskBarIcon.SetIcon ( icon,
                                   wxT("JobGraph")
                                   )
         )
    {
        wxMessageBox(wxT("Could not set icon."));
    }

    dialog = new MyDialog ( NULL,
                            -1,
                            wxT("dummy"),
                            wxPoint(-1, -1),
                            wxSize(100, 100),
                            wxDIALOG_MODELESS | wxDEFAULT_DIALOG_STYLE );

    dialog->Show (FALSE ); // メインのウインドウは最初から隠しとく

    timer = new MyTimer;
    assert( timer );
    timer->Start( TIMERPERIOD );

    return TRUE;
}


BEGIN_EVENT_TABLE(MyDialog, wxDialog)
    EVT_CLOSE(MyDialog::OnCloseWindow)
END_EVENT_TABLE()

namespace{
}

MyDialog::MyDialog ( wxWindow*        parent,
                     const wxWindowID id,
                     const wxString&  title,
                     const wxPoint&   pos,
                     const wxSize&    size,
                     const long       windowStyle )
        :  wxDialog ( parent,
                      id,
                      title,
                      pos,
                      size,
                      windowStyle )
{
    // do nothing
}


void MyDialog::OnCloseWindow ( wxCloseEvent& event )
{
    Destroy();
}


enum {
    PU_EXIT = 10001
};



BEGIN_EVENT_TABLE(MyTaskBarIcon, wxTaskBarIcon)
    EVT_MENU(PU_EXIT,    MyTaskBarIcon::OnMenuExit)
END_EVENT_TABLE()


namespace{}

void MyTaskBarIcon::OnMenuExit(wxCommandEvent& )
{
    dialog->Close(TRUE);
    wxGetApp().ProcessIdle();
}

void MyTaskBarIcon::OnRButtonUp(wxEvent&)
{
    wxMenu      menu;

    menu.Append(PU_EXIT,    _T("E&xit"));
    PopupMenu(&menu);
}









// 
// http://techtips.belution.com/ja/vc/0022/
//
// ウィンドウハンドルから、そのモジュールのフルパスを取得します。
//
// パラメータ
//      hWnd
//          調べるウィンドウのハンドル
//      lpszFileName
//          モジュールの完全修飾パスを受け取るバッファへのポインタ
//      nSize
//          取得したい文字の最大の長さ
//
// 戻り値
//      正常にウィンドウを作成したモジュールのフルパス名が取得でき
//      たら TRUE が返ります。それ以外は FALSE が返ります。
//
BOOL GetFileNameFromHwnd(HWND hWnd, LPTSTR lpszFileName, DWORD nSize)
{
    BOOL bResult = FALSE;
    // ウィンドウを作成したプロセスの ID を取得
    DWORD dwProcessId;
    GetWindowThreadProcessId(hWnd , &dwProcessId);

    // 現在実行している OS のバージョン情報を取得
    OSVERSIONINFO osverinfo;
    osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if ( !GetVersionEx(&osverinfo) )
        return FALSE;

    // プラットフォームが Windows NT/2000 の場合
    if ( osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
    {
        // PSAPI 関数のポインタ
        BOOL (WINAPI *lpfEnumProcessModules)
                            (HANDLE, HMODULE*, DWORD, LPDWORD);
        DWORD (WINAPI *lpfGetModuleFileNameEx)
                            (HANDLE, HMODULE, LPTSTR, DWORD);

        // PSAPI.DLL ライブラリをロード
        HINSTANCE hInstLib = LoadLibrary("PSAPI.DLL");
        if ( hInstLib == NULL )
            return FALSE ;

        // プロシージャのアドレスを取得
        lpfEnumProcessModules = (BOOL(WINAPI *)
            (HANDLE, HMODULE *, DWORD, LPDWORD))GetProcAddress(
                        hInstLib, "EnumProcessModules");
        lpfGetModuleFileNameEx =(DWORD (WINAPI *)
            (HANDLE, HMODULE, LPTSTR, DWORD))GetProcAddress(
                        hInstLib, "GetModuleFileNameExA");

        if ( lpfEnumProcessModules && lpfGetModuleFileNameEx )
        {
            // プロセスオブジェクトのハンドルを取得
            HANDLE hProcess;
            hProcess = OpenProcess(
                    PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                    FALSE, dwProcessId);
            if ( hProcess )
            {
                // .EXE モジュールのハンドルを取得
                HMODULE hModule;
                DWORD dwNeed;
                if (lpfEnumProcessModules(hProcess,
                            &hModule, sizeof(hModule), &dwNeed))
                {
                    // モジュールハンドルからフルパス名を取得
                    if ( lpfGetModuleFileNameEx(hProcess, hModule,
                                            lpszFileName, nSize) )
                        bResult = TRUE;
                }
                // プロセスオブジェクトのハンドルをクローズ
                CloseHandle( hProcess ) ;
            }
        }
        // PSAPI.DLL ライブラリを開放
        FreeLibrary( hInstLib ) ;
    }
    // プラットフォームが Windows 9x の場合
    else if ( osverinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
    {
        // ToolHelp 関数ポインタ
        HANDLE (WINAPI *lpfCreateSnapshot)(DWORD, DWORD);
        BOOL (WINAPI *lpfProcess32First)(HANDLE, LPPROCESSENTRY32);
        BOOL (WINAPI *lpfProcess32Next)(HANDLE, LPPROCESSENTRY32);

        // プロシージャのアドレスを取得
        lpfCreateSnapshot =
            (HANDLE(WINAPI*)(DWORD,DWORD))GetProcAddress(
                                GetModuleHandle("kernel32.dll"),
                                "CreateToolhelp32Snapshot" );
        lpfProcess32First=
            (BOOL(WINAPI*)(HANDLE,LPPROCESSENTRY32))GetProcAddress(
                                GetModuleHandle("kernel32.dll"),
                                "Process32First" );
        lpfProcess32Next=
            (BOOL(WINAPI*)(HANDLE,LPPROCESSENTRY32))GetProcAddress(
                                GetModuleHandle("kernel32.dll"),
                                "Process32Next" );
        if ( !lpfCreateSnapshot ||
            !lpfProcess32First ||
            !lpfProcess32Next)
            return FALSE;

        // システム プロセスの Toolhelp スナップショットを作成
        HANDLE hSnapshot;
        hSnapshot = lpfCreateSnapshot(TH32CS_SNAPPROCESS , 0);
        if (hSnapshot != (HANDLE)-1)
        {
            // 最初のプロセスに関する情報を取得
            PROCESSENTRY32 pe;
            pe.dwSize = sizeof(PROCESSENTRY32);
            if ( lpfProcess32First(hSnapshot, &pe) )
            {
                do {
                    // 同じプロセスID であれば、ファイル名を取得
                    if (pe.th32ProcessID == dwProcessId)
                    {
                        lstrcpy(lpszFileName, pe.szExeFile);
                        bResult = TRUE;
                        break;
                    }
                } while ( lpfProcess32Next(hSnapshot, &pe) );
            }
            // スナップショットを破棄
            CloseHandle(hSnapshot);
        }
    }
    else
        return FALSE;

    return bResult;
}
