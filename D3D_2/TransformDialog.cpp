#include "TransformDialog.h"
#include "SceneObject.h"
#include "resource.h"

#include <DirectXMath.h>
#include <cwchar>

using namespace DirectX;

namespace
{
    struct TransformDialogState
    {
        SceneObject* Obj = nullptr;
    };

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

    INT_PTR CALLBACK TransformDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_INITDIALOG:
        {
            auto* state = reinterpret_cast<TransformDialogState*>(lParam);
            SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)state);

            if (!state || !state->Obj)
            {
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }

            // 读取对象当前变换，填入对话框
            XMFLOAT3 pos = state->Obj->GetPosition();
            XMFLOAT3 rotRad = state->Obj->GetRotation();
            float scale = state->Obj->GetScale();

            // 旋转：对话框用“角度”，内部是“弧度”
            XMFLOAT3 rotDeg(
                XMConvertToDegrees(rotRad.x),
                XMConvertToDegrees(rotRad.y),
                XMConvertToDegrees(rotRad.z));

            SetDlgItemFloat(hDlg, IDC_POS_X, pos.x);
            SetDlgItemFloat(hDlg, IDC_POS_Y, pos.y);
            SetDlgItemFloat(hDlg, IDC_POS_Z, pos.z);

            SetDlgItemFloat(hDlg, IDC_ROT_X, rotDeg.x);
            SetDlgItemFloat(hDlg, IDC_ROT_Y, rotDeg.y);
            SetDlgItemFloat(hDlg, IDC_ROT_Z, rotDeg.z);

            SetDlgItemFloat(hDlg, IDC_SCALE, scale);

            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDOK:
            {
                auto* state = reinterpret_cast<TransformDialogState*>(GetWindowLongPtrW(hDlg, GWLP_USERDATA));
                if (!state || !state->Obj)
                {
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                }

                float px, py, pz;
                float rxDeg, ryDeg, rzDeg;
                float scale;

                if (!TryGetDlgItemFloat(hDlg, IDC_POS_X, px) ||
                    !TryGetDlgItemFloat(hDlg, IDC_POS_Y, py) ||
                    !TryGetDlgItemFloat(hDlg, IDC_POS_Z, pz) ||
                    !TryGetDlgItemFloat(hDlg, IDC_ROT_X, rxDeg) ||
                    !TryGetDlgItemFloat(hDlg, IDC_ROT_Y, ryDeg) ||
                    !TryGetDlgItemFloat(hDlg, IDC_ROT_Z, rzDeg) ||
                    !TryGetDlgItemFloat(hDlg, IDC_SCALE, scale))
                {
                    MessageBoxW(hDlg, L"请输入有效数字。", L"输入错误", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }

                if (scale <= 0.0f)
                {
                    MessageBoxW(hDlg, L"缩放必须大于 0。", L"输入错误", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }

                XMFLOAT3 pos(px, py, pz);

                // 角度 -> 弧度
                XMFLOAT3 rotRad(
                    XMConvertToRadians(rxDeg),
                    XMConvertToRadians(ryDeg),
                    XMConvertToRadians(rzDeg));

                state->Obj->SetPosition(pos);
                state->Obj->SetRotation(rotRad);
                state->Obj->SetScale(scale);

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

bool ShowTransformDialog(HWND parentHwnd, SceneObject* obj)
{
    TransformDialogState state;
    state.Obj = obj;

    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(parentHwnd, GWLP_HINSTANCE);

    INT_PTR ret = DialogBoxParamW(
        hInst,
        MAKEINTRESOURCEW(IDD_TRANSFORM_DIALOG),
        parentHwnd,
        TransformDlgProc,
        (LPARAM)&state);

    return ret == IDOK;
}