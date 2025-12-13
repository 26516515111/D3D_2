#pragma once
#include <windows.h>
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include "d3dx12.h"

#include <string>
#include <memory>
#include <vector>
#include "SceneObject.h"

using Microsoft::WRL::ComPtr;

// 顶点结构体
struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

// 常量缓冲区结构体
struct ObjectConstants
{
    DirectX::XMFLOAT4X4 WorldViewProj;
    DirectX::XMFLOAT4 HighlightColor; // 用于高亮选中对象
};

class PrimitiveShape;

class D3DManager
{
public:
    D3DManager();
    ~D3DManager();

    bool InitD3D(HWND hWnd, int width, int height);
    void Cleanup();
    void Render();
    void OnResize(int width, int height);

    // 场景对象管理
    void AddObject(ShapeType type, const DirectX::XMFLOAT3& position);
    void ClearScene();

    // 鼠标交互
    void OnMouseDown(int x, int y);
    void OnMouseMove(int x, int y);
    void OnMouseUp();
    void OnMouseWheel(int delta);

    // 获取对象数量
    int GetObjectCount() const { return (int)m_sceneObjects.size(); }
private:
    // D3D12 核心对象
    ComPtr<IDXGIFactory4> m_dxgiFactory;
    ComPtr<ID3D12Device> m_d3dDevice;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;

    // 同步对象
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_currentFence = 0;
    HANDLE m_fenceEvent;

    // 描述符堆
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap;

    UINT m_rtvDescriptorSize = 0;
    UINT m_dsvDescriptorSize = 0;
    UINT m_cbvSrvUavDescriptorSize = 0;

    // 渲染目标和深度缓冲
    static const int SwapChainBufferCount = 2;
    ComPtr<ID3D12Resource> m_swapChainBuffer[SwapChainBufferCount];
    ComPtr<ID3D12Resource> m_depthStencilBuffer;
    int m_currBackBuffer = 0;

    // 视口和裁剪矩形
    D3D12_VIEWPORT m_screenViewport;
    D3D12_RECT m_scissorRect;

    // 管线状态
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    // 着色器
    ComPtr<ID3DBlob> m_vsByteCode;
    ComPtr<ID3DBlob> m_psByteCode;

    // 常量缓冲区
    ComPtr<ID3D12Resource> m_objectCB;
    BYTE* m_cbMappedData = nullptr;
    UINT m_objCBByteSize = 0;
    static const UINT MaxObjects = 256;

    // 几何体模板（共享）
    std::shared_ptr<PrimitiveShape> m_sphereTemplate;
    std::shared_ptr<PrimitiveShape> m_cylinderTemplate;
    std::shared_ptr<PrimitiveShape> m_planeTemplate;
    std::shared_ptr<PrimitiveShape> m_cubeTemplate;
    std::shared_ptr<PrimitiveShape> m_tetrahedronTemplate;

    // 场景对象列表
    std::vector<std::unique_ptr<SceneObject>> m_sceneObjects;
    SceneObject* m_selectedObject = nullptr;

    // 窗口信息
    HWND m_hWnd;
    int m_clientWidth;
    int m_clientHeight;

    // 摄像机
    DirectX::XMFLOAT3 m_eyePos;
    DirectX::XMFLOAT3 m_lookAt;
    DirectX::XMFLOAT4X4 m_view;
    DirectX::XMFLOAT4X4 m_proj;

    // 鼠标交互状态
    bool m_isDragging = false;
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;
public:
    // 相机移动
    void MoveCamera(float dx, float dy, float dz);
    // 删除当前选中对象
    void DeleteSelectedObject();
private:
    // 初始化辅助函数
    bool CreateDevice();
    bool CreateCommandObjects();
    bool CreateSwapChain();
    bool CreateDescriptorHeaps();
    bool CreateRenderTargets();
    bool CreateDepthStencil();
    bool CompileShaders();
    bool BuildRootSignature();
    bool BuildPSO();
    bool BuildShapeGeometry();
    bool BuildConstantBuffers();

    // 渲染辅助函数
    void UpdateCamera();
    void UpdateObjectCB(SceneObject* obj, UINT objectIndex);
    void FlushCommandQueue();

    // 射线拾取
    SceneObject* PickObject(int mouseX, int mouseY);
    void ScreenToWorldRay(int mouseX, int mouseY,
        DirectX::XMVECTOR& rayOrigin,
        DirectX::XMVECTOR& rayDir);

    // 工具函数
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;
    ID3D12Resource* CurrentBackBuffer() const;

    std::shared_ptr<PrimitiveShape> GetShapeTemplate(ShapeType type);
};