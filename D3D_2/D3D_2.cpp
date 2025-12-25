#include <windows.h>
#include "D3DManager.h"
#include "resource.h"

// 全局变量
D3DManager* g_pD3DManager = nullptr;

// 窗口过程函数声明
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// 创建菜单
HMENU CreateAppMenu()
{
    HMENU hMenu = CreateMenu();

    // 添加形状菜单
    HMENU hAddMenu = CreatePopupMenu();
    AppendMenu(hAddMenu, MF_STRING, IDM_ADD_SPHERE, L"球体(&S)");
    AppendMenu(hAddMenu, MF_STRING, IDM_ADD_CYLINDER, L"柱体(&C)");
    AppendMenu(hAddMenu, MF_STRING, IDM_ADD_PLANE, L"平面(&P)");
    AppendMenu(hAddMenu, MF_STRING, IDM_ADD_CUBE, L"立方体(&U)");
    AppendMenu(hAddMenu, MF_STRING, IDM_ADD_TETRAHEDRON, L"四面体(&T)");

    // 场景菜单
    HMENU hSceneMenu = CreatePopupMenu();
    AppendMenu(hSceneMenu, MF_STRING, IDM_LIGHT_SETTINGS, L"光照设置(&L)");
    AppendMenu(hSceneMenu, MF_STRING, IDM_CLEAR_SCENE, L"清空场景(&C)");


    HMENU hPicMenu = CreatePopupMenu();
    AppendMenu(hPicMenu, MF_STRING, IDM_EDIT_TRANSFORM, L"编辑图形参数(&E)");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hAddMenu, L"添加形状(&A)");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSceneMenu, L"场景(&S)");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hPicMenu, L"图形编辑(&M)");
    return hMenu;
}

// WinMain 入口函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    // 注册窗口类
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"D3D12ShapesWindow";

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(nullptr, L"窗口注册失败！", L"错误", MB_OK | MB_ICONERROR);
        return 0;
    }

    // 创建窗口
    int screenWidth = 1280;
    int screenHeight = 720;

    RECT rc = { 0, 0, screenWidth, screenHeight };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, TRUE);

    HWND hWnd = CreateWindow(
        L"D3D12ShapesWindow",
        L"Direct3D 12 交互式三维场景",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, CreateAppMenu(), hInstance, nullptr
    );

    if (!hWnd)
    {
        MessageBox(nullptr, L"窗口创建失败！", L"错误", MB_OK | MB_ICONERROR);
        return 0;
    }

    // 初始化 D3D
    g_pD3DManager = new D3DManager();
    if (!g_pD3DManager->InitD3D(hWnd, screenWidth, screenHeight))
    {
        MessageBox(hWnd,
            L"Direct3D 12 初始化失败！\n请确保您的显卡支持 DirectX 12。",
            L"错误", MB_OK | MB_ICONERROR);
        delete g_pD3DManager;
        return 0;
    }

    // 添加一些初始对象到场景
    g_pD3DManager->AddObject(ShapeType::Cube, DirectX::XMFLOAT3(-3.0f, 0.0f, 0.0f));
    g_pD3DManager->AddObject(ShapeType::Sphere, DirectX::XMFLOAT3(0.0f, 0.0f, 2.0f));
    g_pD3DManager->AddObject(ShapeType::Cylinder, DirectX::XMFLOAT3(3.0f, 0.0f, -2.0f));

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 消息循环
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // 渲染场景
            if (g_pD3DManager)
            {
                g_pD3DManager->Render();
            }
        }
    }

    // 清理
    if (g_pD3DManager)
    {
        g_pD3DManager->Cleanup();
        delete g_pD3DManager;
        g_pD3DManager = nullptr;
    }

    return (int)msg.wParam;
}

// 窗口过程
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);

        // 计算新对象的位置（随机偏移）
        static int objectCount = 0;
        float xOffset = (objectCount % 5 - 2) * 2.0f;
        float yOffset = 0.0f;
        float zOffset = (objectCount / 5) * 2.0f;
        DirectX::XMFLOAT3 newPos(xOffset, yOffset, zOffset);

        switch (wmId)
        {
        case IDM_ADD_SPHERE:
            g_pD3DManager->AddObject(ShapeType::Sphere, newPos);
            objectCount++;
            break;
        case IDM_ADD_CYLINDER:
            g_pD3DManager->AddObject(ShapeType::Cylinder, newPos);
            objectCount++;
            break;
        case IDM_ADD_PLANE:
            g_pD3DManager->AddObject(ShapeType::Plane, newPos);
            objectCount++;
            break;
        case IDM_ADD_CUBE:
            g_pD3DManager->AddObject(ShapeType::Cube, newPos);
            objectCount++;
            break;
        case IDM_ADD_TETRAHEDRON:
            g_pD3DManager->AddObject(ShapeType::Tetrahedron, newPos);
            objectCount++;
            break;
        case IDM_CLEAR_SCENE:
            g_pD3DManager->ClearScene();
            objectCount = 0;
            break;
        case IDM_EDIT_TRANSFORM: {
            bool enabled = !g_pD3DManager->IsEditMode();
            g_pD3DManager->SetEditMode(enabled);
            CheckMenuItem(GetMenu(hWnd), IDM_EDIT_TRANSFORM, MF_BYCOMMAND | (enabled ? MF_CHECKED : MF_UNCHECKED));
            break;
        }
        case IDM_LIGHT_SETTINGS:
            g_pD3DManager->ShowLightSettingsDialog();
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }

    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        if (g_pD3DManager)
        {
            g_pD3DManager->OnMouseDown(x, y);
        }
        break;
    }
    case WM_LBUTTONDBLCLK:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        if (g_pD3DManager)
        {
            g_pD3DManager->OnMouseDoubleClick(x, y);
        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        if (g_pD3DManager)
        {
            g_pD3DManager->OnMouseMove(x, y);
        }
        break;
    }

    case WM_LBUTTONUP:
    {
        if (g_pD3DManager)
        {
            g_pD3DManager->OnMouseUp();
        }
        break;
    }

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (g_pD3DManager)
        {
            g_pD3DManager->OnMouseWheel(delta);
        }
        break;
    }
    case WM_KEYDOWN:
    {
        if (!g_pD3DManager)
        {
            break;
        }

        const float moveStep = 0.5f;

        switch (wParam)
        {
        case 'W': // 前
            g_pD3DManager->MoveCamera(0.0f, 0.0f, moveStep);
            break;
        case 'S': // 后
            g_pD3DManager->MoveCamera(0.0f, 0.0f, -moveStep);
            break;
        case 'A': // 左
            g_pD3DManager->MoveCamera(-moveStep, 0.0f, 0.0f);
            break;
        case 'D': // 右
            g_pD3DManager->MoveCamera(moveStep, 0.0f, 0.0f);
            break;
        case 'Q': // 下
            g_pD3DManager->MoveCamera(0.0f, -moveStep, 0.0f);
            break;
        case 'E': // 上
            g_pD3DManager->MoveCamera(0.0f, moveStep, 0.0f);
            break;
        case VK_DELETE: // 删除选中对象
            g_pD3DManager->DeleteSelectedObject();
            break;
        case VK_SPACE: // 空格清空场景
            g_pD3DManager->ClearScene();
            break;
        default:
            break;
        }
        break;
    }

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        if (g_pD3DManager && width > 0 && height > 0)
        {
            g_pD3DManager->OnResize(width, height);
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}