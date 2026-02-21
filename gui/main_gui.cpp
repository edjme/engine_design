#include <wx/wx.h>
#include "MainFrame.h"
#include <wx/image.h>

class EngineDesignApp : public wxApp
{
public:
    bool OnInit() override
    {
        wxPuts("OnInit() started"); // вывод в консоль
        wxImage::AddHandler(new wxPNGHandler);

        auto* frame = new MainFrame("Engine Design - Kinematic");
        frame->Show(true);

        wxPuts("MainFrame created and shown");
        return true;
    }
};

wxIMPLEMENT_APP(EngineDesignApp);
