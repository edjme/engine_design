#pragma once
#include <vector>
#include <wx/string.h>

enum class PressureUnit { BAR, MPA, KPA, PSI };

bool LoadPressureFromFile(const wxString& filename,
                          std::vector<double>& angles,
                          std::vector<double>& pressures,
                          wxString& errorMsg,
                          PressureUnit unit = PressureUnit::BAR);