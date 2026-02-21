#include "dynamic_input.h"
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <algorithm>
#include <map>

bool LoadPressureFromFile(const wxString& filename,
                          std::vector<double>& angles,
                          std::vector<double>& pressures,
                          wxString& errorMsg,
                          PressureUnit unit)
{
    wxTextFile file;
    if (!file.Open(filename))
    {
        errorMsg = wxString::FromUTF8("Не удалось открыть файл.");
        return false;
    }

    angles.clear();
    pressures.clear();

    double convFactor = 1.0;
    switch (unit)
    {
    case PressureUnit::BAR: convFactor = 1e5; break;
    case PressureUnit::MPA: convFactor = 1e6; break;
    case PressureUnit::KPA: convFactor = 1e3; break;
    case PressureUnit::PSI: convFactor = 6894.75729; break;
    }

    // Определяем наличие заголовка и разделитель
    bool hasHeader = false;
    wxChar delimiter = ';'; // по умолчанию
    std::map<wxChar, int> delimCount;

    // Пропускаем возможный BOM в первой строке
    wxString firstLine = file[0];
    if (!firstLine.IsEmpty() && firstLine[0] == 0xFEFF)
        firstLine = firstLine.Mid(1);
    firstLine.Trim(true).Trim(false);

    // Анализируем первые несколько строк для определения разделителя
    size_t linesToCheck = std::min((size_t)5, file.GetLineCount());
    for (size_t i = 0; i < linesToCheck; ++i)
    {
        wxString line = file[i];
        if (!line.IsEmpty() && line[0] == 0xFEFF)
            line = line.Mid(1);
        line.Trim(true).Trim(false);
        if (line.empty()) continue;

        for (const wxChar& ch : line)
        {
            if (ch == ',' || ch == ';' || ch == '\t')
                delimCount[ch]++;
        }
    }

    // Выбираем разделитель с максимальным количеством вхождений
    wxChar bestDelim = ';';
    int maxCount = 0;
    for (const auto& pair : delimCount)
    {
        if (pair.second > maxCount)
        {
            maxCount = pair.second;
            bestDelim = pair.first;
        }
    }
    if (maxCount > 0)
        delimiter = bestDelim;

    // Проверка на заголовок: если первая строка содержит нечисловые данные после разбиения
    if (file.GetLineCount() > 0)
    {
        wxString testLine = file[0];
        if (!testLine.IsEmpty() && testLine[0] == 0xFEFF)
            testLine = testLine.Mid(1);
        testLine.Trim(true).Trim(false);
        wxStringTokenizer tokenizer(testLine, delimiter);
        if (tokenizer.HasMoreTokens())
        {
            wxString token = tokenizer.GetNextToken();
            token.Trim(true).Trim(false);
            token.Replace(",", ".");
            double dummy;
            if (!token.ToCDouble(&dummy))
                hasHeader = true;
        }
    }

    size_t startLine = hasHeader ? 1 : 0;
    for (size_t i = startLine; i < file.GetLineCount(); ++i)
    {
        wxString line = file[i];
        if (!line.IsEmpty() && line[0] == 0xFEFF)
            line = line.Mid(1);
        line.Trim(true).Trim(false);
        if (line.empty()) continue;

        wxStringTokenizer tokenizer(line, delimiter);
        if (tokenizer.CountTokens() < 2) continue;

        wxString tokenAngle = tokenizer.GetNextToken();
        wxString tokenPressure = tokenizer.GetNextToken();

        // Очистка и замена десятичной запятой
        tokenAngle.Trim(true).Trim(false);
        tokenPressure.Trim(true).Trim(false);
        tokenAngle.Replace(",", ".");
        tokenPressure.Replace(",", ".");

        double angle, pressure;
        if (!tokenAngle.ToCDouble(&angle) || !tokenPressure.ToCDouble(&pressure))
            continue;

        angles.push_back(angle);
        pressures.push_back(pressure * convFactor);
    }

    file.Close();

    if (angles.empty())
    {
        errorMsg = wxString::FromUTF8("Файл не содержит числовых данных.");
        return false;
    }

    // Проверка монотонности углов
    for (size_t i = 1; i < angles.size(); ++i)
    {
        if (angles[i] <= angles[i-1])
        {
            errorMsg = wxString::FromUTF8("Углы должны быть строго возрастающими.");
            return false;
        }
    }

    return true;
}