#include <wx/wx.h>
#include "MainFrame.h"

class EngineDesignApp : public wxApp
{
public:
    bool OnInit() override
    {
        wxPuts("OnInit() started"); // вывод в консоль

        auto* frame = new MainFrame("Engine Design - Kinematic");
        frame->Show(true);

        wxPuts("MainFrame created and shown");
        return true;
    }
};

wxIMPLEMENT_APP(EngineDesignApp);
