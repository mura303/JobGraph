//////////////////////////////////////////////////////////////////////////////////////////////
//
// �E�C���h�E�^�C�g���̃��O���Ƃ�Â���c�[��
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

    dialog->Show (FALSE ); // ���C���̃E�C���h�E�͍ŏ�����B���Ƃ�

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
// �E�B���h�E�n���h������A���̃��W���[���̃t���p�X���擾���܂��B
//
// �p�����[�^
//      hWnd
//          ���ׂ�E�B���h�E�̃n���h��
//      lpszFileName
//          ���W���[���̊��S�C���p�X���󂯎��o�b�t�@�ւ̃|�C���^
//      nSize
//          �擾�����������̍ő�̒���
//
// �߂�l
//      ����ɃE�B���h�E���쐬�������W���[���̃t���p�X�����擾�ł�
//      ���� TRUE ���Ԃ�܂��B����ȊO�� FALSE ���Ԃ�܂��B
//
BOOL GetFileNameFromHwnd(HWND hWnd, LPTSTR lpszFileName, DWORD nSize)
{
    BOOL bResult = FALSE;
    // �E�B���h�E���쐬�����v���Z�X�� ID ���擾
    DWORD dwProcessId;
    GetWindowThreadProcessId(hWnd , &dwProcessId);

    // ���ݎ��s���Ă��� OS �̃o�[�W���������擾
    OSVERSIONINFO osverinfo;
    osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if ( !GetVersionEx(&osverinfo) )
        return FALSE;

    // �v���b�g�t�H�[���� Windows NT/2000 �̏ꍇ
    if ( osverinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
    {
        // PSAPI �֐��̃|�C���^
        BOOL (WINAPI *lpfEnumProcessModules)
                            (HANDLE, HMODULE*, DWORD, LPDWORD);
        DWORD (WINAPI *lpfGetModuleFileNameEx)
                            (HANDLE, HMODULE, LPTSTR, DWORD);

        // PSAPI.DLL ���C�u���������[�h
        HINSTANCE hInstLib = LoadLibrary("PSAPI.DLL");
        if ( hInstLib == NULL )
            return FALSE ;

        // �v���V�[�W���̃A�h���X���擾
        lpfEnumProcessModules = (BOOL(WINAPI *)
            (HANDLE, HMODULE *, DWORD, LPDWORD))GetProcAddress(
                        hInstLib, "EnumProcessModules");
        lpfGetModuleFileNameEx =(DWORD (WINAPI *)
            (HANDLE, HMODULE, LPTSTR, DWORD))GetProcAddress(
                        hInstLib, "GetModuleFileNameExA");

        if ( lpfEnumProcessModules && lpfGetModuleFileNameEx )
        {
            // �v���Z�X�I�u�W�F�N�g�̃n���h�����擾
            HANDLE hProcess;
            hProcess = OpenProcess(
                    PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                    FALSE, dwProcessId);
            if ( hProcess )
            {
                // .EXE ���W���[���̃n���h�����擾
                HMODULE hModule;
                DWORD dwNeed;
                if (lpfEnumProcessModules(hProcess,
                            &hModule, sizeof(hModule), &dwNeed))
                {
                    // ���W���[���n���h������t���p�X�����擾
                    if ( lpfGetModuleFileNameEx(hProcess, hModule,
                                            lpszFileName, nSize) )
                        bResult = TRUE;
                }
                // �v���Z�X�I�u�W�F�N�g�̃n���h�����N���[�Y
                CloseHandle( hProcess ) ;
            }
        }
        // PSAPI.DLL ���C�u�������J��
        FreeLibrary( hInstLib ) ;
    }
    // �v���b�g�t�H�[���� Windows 9x �̏ꍇ
    else if ( osverinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
    {
        // ToolHelp �֐��|�C���^
        HANDLE (WINAPI *lpfCreateSnapshot)(DWORD, DWORD);
        BOOL (WINAPI *lpfProcess32First)(HANDLE, LPPROCESSENTRY32);
        BOOL (WINAPI *lpfProcess32Next)(HANDLE, LPPROCESSENTRY32);

        // �v���V�[�W���̃A�h���X���擾
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

        // �V�X�e�� �v���Z�X�� Toolhelp �X�i�b�v�V���b�g���쐬
        HANDLE hSnapshot;
        hSnapshot = lpfCreateSnapshot(TH32CS_SNAPPROCESS , 0);
        if (hSnapshot != (HANDLE)-1)
        {
            // �ŏ��̃v���Z�X�Ɋւ�������擾
            PROCESSENTRY32 pe;
            pe.dwSize = sizeof(PROCESSENTRY32);
            if ( lpfProcess32First(hSnapshot, &pe) )
            {
                do {
                    // �����v���Z�XID �ł���΁A�t�@�C�������擾
                    if (pe.th32ProcessID == dwProcessId)
                    {
                        lstrcpy(lpszFileName, pe.szExeFile);
                        bResult = TRUE;
                        break;
                    }
                } while ( lpfProcess32Next(hSnapshot, &pe) );
            }
            // �X�i�b�v�V���b�g��j��
            CloseHandle(hSnapshot);
        }
    }
    else
        return FALSE;

    return bResult;
}
