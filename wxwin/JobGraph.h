class MyTaskBarIcon: public wxTaskBarIcon
{
public:
    MyTaskBarIcon() {};
    virtual void OnRButtonUp(wxEvent&);
    void OnMenuExit(wxCommandEvent&);

DECLARE_EVENT_TABLE()
};


// Define a new application
class MyApp: public wxApp
{
public:
    bool OnInit(void);
protected:
    MyTaskBarIcon   m_taskBarIcon;
};

class MyDialog: public wxDialog
{
public:
    MyDialog(wxWindow* parent, const wxWindowID id, const wxString& title,
        const wxPoint& pos, const wxSize& size, const long windowStyle = wxDEFAULT_DIALOG_STYLE);

    void OnOK(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnCloseWindow(wxCloseEvent& event);

DECLARE_EVENT_TABLE()
};


