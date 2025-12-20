#include "LightDialog.h"
#include "resource.h"

#include <cwchar>

namespace
{
    void SetDlgItemFloat(HWND hDlg, int id, float value)
    {
        wchar_t buf[64]{};
        swprintf_s(buf, L"%.4f", value);
        SetDlgItemTextW(hDlg, id, buf);
    }

    bool TryGetDlgItemFloat(HWND hDlg, int id, float& value)
    {
        wchar_t buf[128]{};
        GetDlgItemTextW(hDlg, id, buf, (int)_countof(buf));

        wchar_t* endPtr = nullptr;
        double v = wcstod(buf, &endPtr);
        if (endPtr == buf)
        {
            return false;
        }

        value = (float)v;
        return true;
    }

    INT_PTR CALLBACK LightDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_INITDIALOG:
        {
            auto* settings = reinterpret_cast<LightSettings*>(lParam);
            SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)settings);

            if (!settings)
            {
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }

            SetDlgItemFloat(hDlg, IDC_LIGHT_POS_X, settings->PosX);
            SetDlgItemFloat(hDlg, IDC_LIGHT_POS_Y, settings->PosY);
            SetDlgItemFloat(hDlg, IDC_LIGHT_POS_Z, settings->PosZ);
            SetDlgItemFloat(hDlg, IDC_LIGHT_AMBIENT, settings->Ambient);
            SetDlgItemFloat(hDlg, IDC_LIGHT_DIFFUSE, settings->Diffuse);
            SetDlgItemFloat(hDlg, IDC_LIGHT_SPECULAR, settings->Specular);
            SetDlgItemFloat(hDlg, IDC_LIGHT_SHININESS, settings->Shininess);
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDOK:
            {
                auto* settings = reinterpret_cast<LightSettings*>(GetWindowLongPtrW(hDlg, GWLP_USERDATA));
                if (!settings)
                {
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                }

                if (!TryGetDlgItemFloat(hDlg, IDC_LIGHT_POS_X, settings->PosX) ||
                    !TryGetDlgItemFloat(hDlg, IDC_LIGHT_POS_Y, settings->PosY) ||
                    !TryGetDlgItemFloat(hDlg, IDC_LIGHT_POS_Z, settings->PosZ) ||
                    !TryGetDlgItemFloat(hDlg, IDC_LIGHT_AMBIENT, settings->Ambient) ||
                    !TryGetDlgItemFloat(hDlg, IDC_LIGHT_DIFFUSE, settings->Diffuse) ||
                    !TryGetDlgItemFloat(hDlg, IDC_LIGHT_SPECULAR, settings->Specular) ||
                    !TryGetDlgItemFloat(hDlg, IDC_LIGHT_SHININESS, settings->Shininess))
                {
                    MessageBoxW(hDlg, L"请输入有效数字。", L"输入错误", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }

                if (settings->Shininess < 1.0f)
                {
                    MessageBoxW(hDlg, L"高光指数建议 >= 1。", L"输入错误", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }

                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            case IDCANCEL:
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            default:
                return FALSE;
            }
        }
        default:
            return FALSE;
        }
    }
}

bool ShowLightDialog(HWND parentHwnd, LightSettings& settings)
{
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(parentHwnd, GWLP_HINSTANCE);

    INT_PTR ret = DialogBoxParamW(
        hInst,
        MAKEINTRESOURCEW(IDD_LIGHT_DIALOG),
        parentHwnd,
        LightDlgProc,
        (LPARAM)&settings);

    return ret == IDOK;
}