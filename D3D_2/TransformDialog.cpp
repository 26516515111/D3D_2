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

    bool Validate01(HWND hDlg, int ctrlId, float v, const wchar_t* name)
    {
        if (v < 0.0f || v > 1.0f)
        {
            wchar_t msg[128]{};
            swprintf_s(msg, L"%s 必须在 0~1 之间。", name);
            MessageBoxW(hDlg, msg, L"输入错误", MB_OK | MB_ICONWARNING);
            SetFocus(GetDlgItem(hDlg, ctrlId));
            SendDlgItemMessageW(hDlg, ctrlId, EM_SETSEL, 0, -1);
            return false;
        }

        return true;
    }

    void InitTextureMappingCombo(HWND hDlg, const SceneObject* obj)
    {
        HWND hCombo = GetDlgItem(hDlg, IDC_TEX_MAPPING);
        if (!hCombo)
        {
            return;
        }

        SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"平面(Planar)");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"柱面(Cylindrical)");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"球面(Spherical)");

        int curSel = 0;
        if (obj)
        {
            curSel = (int)obj->GetTextureMappingMode();
            if (curSel < 0 || curSel > 2)
            {
                curSel = 0;
            }
        }

        SendMessageW(hCombo, CB_SETCURSEL, (WPARAM)curSel, 0);
    }

    void InitTextureStyleCombo(HWND hDlg, const SceneObject* obj)
    {
        HWND hCombo = GetDlgItem(hDlg, IDC_TEX_STYLE);
        if (!hCombo)
        {
            return;
        }

        SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"棋盘格(Checker)");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"条纹(Stripes)");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"图片(占位)");

        int curSel = 0;
        if (obj)
        {
            curSel = (int)obj->GetTextureStyle();
            if (curSel < 0 || curSel > 2)
            {
                curSel = 0;
            }
        }

        SendMessageW(hCombo, CB_SETCURSEL, (WPARAM)curSel, 0);
    }

    void BrowseTextureFile(HWND hDlg)
    {
        // A 方案：仅选择文件路径并保存，不负责加载纹理资源
        wchar_t fileBuf[MAX_PATH]{};

        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hDlg;
        ofn.lpstrFile = fileBuf;
        ofn.nMaxFile = (DWORD)_countof(fileBuf);

        // 不限制扩展名（后续你可按需要收紧）
        ofn.lpstrFilter = L"所有文件 (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameW(&ofn))
        {
            SetDlgItemTextW(hDlg, IDC_TEX_PATH, fileBuf);

            HWND hStyle = GetDlgItem(hDlg, IDC_TEX_STYLE);
            if (hStyle)
            {
                SendMessageW(hStyle, CB_SETCURSEL, (WPARAM)2, 0);
            }
        }
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

            // 变换
            XMFLOAT3 pos = state->Obj->GetPosition();
            XMFLOAT3 rotRad = state->Obj->GetRotation();
            float scale = state->Obj->GetScale();

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

            // 材质（1+2+3）
            const auto& mat = state->Obj->GetMaterial();
            SetDlgItemFloat(hDlg, IDC_MAT_R, mat.BaseColor.x);
            SetDlgItemFloat(hDlg, IDC_MAT_G, mat.BaseColor.y);
            SetDlgItemFloat(hDlg, IDC_MAT_B, mat.BaseColor.z);
            SetDlgItemFloat(hDlg, IDC_MAT_SPECULAR, mat.SpecularStrength);
            SetDlgItemFloat(hDlg, IDC_MAT_SHININESS, mat.Shininess);

            // 纹理（A 方案：仅保存路径 + 选择映射方式）
            SetDlgItemTextW(hDlg, IDC_TEX_PATH, state->Obj->GetTexturePath().c_str());
            InitTextureMappingCombo(hDlg, state->Obj);
            InitTextureStyleCombo(hDlg, state->Obj);

            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_TEX_BROWSE:
                BrowseTextureFile(hDlg);
                return TRUE;

            case IDOK:
            {
                auto* state = reinterpret_cast<TransformDialogState*>(GetWindowLongPtrW(hDlg, GWLP_USERDATA));
                if (!state || !state->Obj)
                {
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                }

                // 变换输入
                float px, py, pz;
                float rxDeg, ryDeg, rzDeg;
                float scale;

                // 材质输入
                float r, g, b;
                float specularStrength;
                float shininess;

                if (!TryGetDlgItemFloat(hDlg, IDC_POS_X, px) ||
                    !TryGetDlgItemFloat(hDlg, IDC_POS_Y, py) ||
                    !TryGetDlgItemFloat(hDlg, IDC_POS_Z, pz) ||
                    !TryGetDlgItemFloat(hDlg, IDC_ROT_X, rxDeg) ||
                    !TryGetDlgItemFloat(hDlg, IDC_ROT_Y, ryDeg) ||
                    !TryGetDlgItemFloat(hDlg, IDC_ROT_Z, rzDeg) ||
                    !TryGetDlgItemFloat(hDlg, IDC_SCALE, scale) ||
                    !TryGetDlgItemFloat(hDlg, IDC_MAT_R, r) ||
                    !TryGetDlgItemFloat(hDlg, IDC_MAT_G, g) ||
                    !TryGetDlgItemFloat(hDlg, IDC_MAT_B, b) ||
                    !TryGetDlgItemFloat(hDlg, IDC_MAT_SPECULAR, specularStrength) ||
                    !TryGetDlgItemFloat(hDlg, IDC_MAT_SHININESS, shininess))
                {
                    MessageBoxW(hDlg, L"请输入有效数字。", L"输入错误", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }

                if (scale <= 0.0f)
                {
                    MessageBoxW(hDlg, L"缩放必须大于 0。", L"输入错误", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }

                if (!Validate01(hDlg, IDC_MAT_R, r, L"BaseColor R") ||
                    !Validate01(hDlg, IDC_MAT_G, g, L"BaseColor G") ||
                    !Validate01(hDlg, IDC_MAT_B, b, L"BaseColor B") ||
                    !Validate01(hDlg, IDC_MAT_SPECULAR, specularStrength, L"Specular"))
                {
                    return TRUE;
                }

                if (shininess < 1.0f)
                {
                    MessageBoxW(hDlg, L"Shininess 必须 >= 1。", L"输入错误", MB_OK | MB_ICONWARNING);
                    SetFocus(GetDlgItem(hDlg, IDC_MAT_SHININESS));
                    SendDlgItemMessageW(hDlg, IDC_MAT_SHININESS, EM_SETSEL, 0, -1);
                    return TRUE;
                }

                // 纹理路径（A 方案：保存字符串）
                wchar_t texPath[512]{};
                GetDlgItemTextW(hDlg, IDC_TEX_PATH, texPath, (int)_countof(texPath));
                state->Obj->SetTexturePath(texPath);

                // 纹理映射方式
                int mappingIndex = 0;
                HWND hCombo = GetDlgItem(hDlg, IDC_TEX_MAPPING);
                if (hCombo)
                {
                    mappingIndex = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                    if (mappingIndex < 0)
                    {
                        mappingIndex = 0;
                    }
                }
                state->Obj->SetTextureMappingMode((TextureMappingMode)mappingIndex);

                int styleIndex = 0;
                HWND hStyle = GetDlgItem(hDlg, IDC_TEX_STYLE);
                if (hStyle)
                {
                    styleIndex = (int)SendMessageW(hStyle, CB_GETCURSEL, 0, 0);
                    if (styleIndex < 0)
                    {
                        styleIndex = 0;
                    }
                }
                state->Obj->SetTextureStyle((TextureStyle)styleIndex);

                // 应用变换
                XMFLOAT3 pos(px, py, pz);
                XMFLOAT3 rotRad(
                    XMConvertToRadians(rxDeg),
                    XMConvertToRadians(ryDeg),
                    XMConvertToRadians(rzDeg));

                state->Obj->SetPosition(pos);
                state->Obj->SetRotation(rotRad);
                state->Obj->SetScale(scale);

                // 应用材质
                Material mat = state->Obj->GetMaterial();
                mat.BaseColor = XMFLOAT3(r, g, b);
                mat.SpecularStrength = specularStrength;
                mat.Shininess = shininess;
                state->Obj->SetMaterial(mat);

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