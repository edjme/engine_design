#include <wx/wx.h>

// ==== объявляем GUI класс ====

class KinematicFrame : public wxFrame
{
public:
    KinematicFrame();

private:
    void OnCalculate(wxCommandEvent &event);

    wxTextCtrl *rpmInput;
    wxStaticText *resultLabel;
};

// ==== реализация ====

KinematicFrame::KinematicFrame()
    : wxFrame(nullptr, wxID_ANY, "Kinematic calculator", wxDefaultPosition, wxSize(400, 200))
{
    wxPanel *panel = new wxPanel(this);

    auto *vbox = new wxBoxSizer(wxVERTICAL);

    // поле ввода RPM
    auto *hbox1 = new wxBoxSizer(wxHORIZONTAL);
    hbox1->Add(new wxStaticText(panel, wxID_ANY, "RPM:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    rpmInput = new wxTextCtrl(panel, wxID_ANY);
    hbox1->Add(rpmInput, 1, wxALL | wxEXPAND, 5);

    // кнопка "Рассчитать"
    auto *button = new wxButton(panel, wxID_ANY, "Рассчитать");
    button->Bind(wxEVT_BUTTON, &KinematicFrame::OnCalculate, this);

    // вывод результата
    resultLabel = new wxStaticText(panel, wxID_ANY, "Результат будет здесь");

    vbox->Add(hbox1, 0, wxEXPAND | wxALL, 5);
    vbox->Add(button, 0, wxALIGN_CENTER | wxALL, 5);
    vbox->Add(resultLabel, 0, wxALL, 10);

    panel->SetSizer(vbox);
}

void KinematicFrame::OnCalculate(wxCommandEvent &event)
{
    // читаем текст и преобразуем в число
    double rpm = std::stod(rpmInput->GetValue().ToStdString());

    // для примера считаем ω = RPM * PI / 30
    double result = rpm * 3.14159 / 30.0;

    resultLabel->SetLabel("ω = " + std::to_string(result) + " rad/s");
}

// ==== точка входа ====

class MyApp : public wxApp
{
public:
    virtual bool OnInit()
    {
        KinematicFrame *frame = new KinematicFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
