#include "D3DManager.h"
#include "PrimitiveShape.h"
#include <comdef.h>
#include"TransformDialog.h"
//#include <commctrl.h> // 若后续加工具条可用；此处可不加


using namespace DirectX;

// ============================================================================
// 构造函数和析构函数
// ============================================================================
D3DManager::D3DManager()
    : m_hWnd(nullptr)
    , m_clientWidth(800)
    , m_clientHeight(600)
    , m_eyePos(0.0f, 5.0f, -15.0f)
    , m_lookAt(0.0f, 0.0f, 0.0f)
{
    XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&m_view, I);
    XMStoreFloat4x4(&m_proj, I);
}

D3DManager::~D3DManager()
{
    Cleanup();
}

// ============================================================================
// 初始化 D3D12
// ============================================================================
bool D3DManager::InitD3D(HWND hWnd, int width, int height)
{
    m_hWnd = hWnd;
    m_clientWidth = width;
    m_clientHeight = height;

    if (!CreateDevice()) return false;
    if (!CreateCommandObjects()) return false;
    if (!CreateSwapChain()) return false;
    if (!CreateDescriptorHeaps()) return false;
    if (!CreateRenderTargets()) return false;
    if (!CreateDepthStencil()) return false;

    m_commandList->Close();
    ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    FlushCommandQueue();

    if (!CompileShaders()) return false;
    if (!BuildRootSignature()) return false;
    if (!BuildPSO()) return false;
    if (!BuildConstantBuffers()) return false;
    if (!BuildShapeGeometry()) return false;

    m_screenViewport.TopLeftX = 0;
    m_screenViewport.TopLeftY = 0;
    m_screenViewport.Width = static_cast<float>(width);
    m_screenViewport.Height = static_cast<float>(height);
    m_screenViewport.MinDepth = 0.0f;
    m_screenViewport.MaxDepth = 1.0f;

    m_scissorRect = { 0, 0, width, height };

    return true;
}

void D3DManager::MoveCamera(float dx, float dy, float dz)
{
    // dx, dy, dz 表示相机局部坐标系中的位移：
   // x: 右 (+x)，y: 上 (+y)，z: 前 (+z)

   // 当前相机参数
    XMVECTOR eye = XMLoadFloat3(&m_eyePos);
    XMVECTOR target = XMLoadFloat3(&m_lookAt);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    // 计算相机的“前、右、上”方向（世界坐标系）
    XMVECTOR forward = XMVector3Normalize(target - eye);            // 相机前方向
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward)); // 相机右方向
    XMVECTOR camUp = XMVector3Normalize(XMVector3Cross(forward, right)); // 正交化后的上方向

    // 根据局部位移组合成世界位移
    XMVECTOR delta = XMVectorZero();
    if (dx != 0.0f)
    {
        delta += right * dx;
    }
    if (dy != 0.0f)
    {
        delta += camUp * dy;
    }
    if (dz != 0.0f)
    {
        delta += forward * dz;
    }

    // 更新 eye 和 lookAt（保持视线方向不变）
    eye += delta;
    target += delta;

    XMStoreFloat3(&m_eyePos, eye);
    XMStoreFloat3(&m_lookAt, target);
}

void D3DManager::DeleteSelectedObject()
{
    if (!m_selectedObject)
    {
        return;
    }

    // 从 vector 中删除选中对象
    for (auto it = m_sceneObjects.begin(); it != m_sceneObjects.end(); ++it)
    {
        if (it->get() == m_selectedObject)
        {
            m_sceneObjects.erase(it);
            break;
        }
    }

    m_selectedObject = nullptr;
}

// ============================================================================
// 创建设备
// ============================================================================
bool D3DManager::CreateDevice()
{
#if defined(DEBUG) || defined(_DEBUG)
    // 启用调试层
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
#endif

    // 创建 DXGI Factory
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory))))
        return false;

    // 尝试创建硬件设备
    HRESULT hardwareResult = D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_d3dDevice)
    );

    // 如果失败，使用 WARP 设备
    if (FAILED(hardwareResult))
    {
        ComPtr<IDXGIAdapter> pWarpAdapter;
        m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter));

        if (FAILED(D3D12CreateDevice(
            pWarpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_d3dDevice))))
        {
            return false;
        }
    }

    // 创建 Fence
    if (FAILED(m_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))))
        return false;

    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr)
        return false;

    // 查询描述符大小
    m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_cbvSrvUavDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    return true;
}

// ============================================================================
// 创建命令对象
// ============================================================================
bool D3DManager::CreateCommandObjects()
{
    // 创建命令队列
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    if (FAILED(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue))))
        return false;

    // 创建命令分配器
    if (FAILED(m_d3dDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&m_commandAllocator))))
        return false;

    // 创建命令列表
    if (FAILED(m_d3dDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_commandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&m_commandList))))
        return false;

    return true;
}

// ============================================================================
// 创建交换链
// ============================================================================
bool D3DManager::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferDesc.Width = m_clientWidth;
    sd.BufferDesc.Height = m_clientHeight;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = SwapChainBufferCount;
    sd.OutputWindow = m_hWnd;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain> swapChain;
    if (FAILED(m_dxgiFactory->CreateSwapChain(
        m_commandQueue.Get(),
        &sd,
        &swapChain)))
        return false;

    swapChain.As(&m_swapChain);
    return true;
}

// ============================================================================
// 创建描述符堆
// ============================================================================
bool D3DManager::CreateDescriptorHeaps()
{
    // RTV 堆
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(m_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap))))
        return false;

    // DSV 堆
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (FAILED(m_d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap))))
        return false;

    // CBV 堆
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if (FAILED(m_d3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap))))
        return false;

    return true;
}

// ============================================================================
// 创建渲染目标视图
// ============================================================================
bool D3DManager::CreateRenderTargets()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

    for (UINT i = 0; i < SwapChainBufferCount; i++)
    {
        if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffer[i]))))
            return false;

        m_d3dDevice->CreateRenderTargetView(m_swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
    }

    return true;
}

// ============================================================================
// 创建深度模板缓冲区
// ============================================================================
bool D3DManager::CreateDepthStencil()
{
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = m_clientWidth;
    depthStencilDesc.Height = m_clientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    if (FAILED(m_d3dDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(&m_depthStencilBuffer))))
        return false;

    m_d3dDevice->CreateDepthStencilView(
        m_depthStencilBuffer.Get(),
        nullptr,
        DepthStencilView()
    );

    // 转换资源状态
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_depthStencilBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_DEPTH_WRITE
    );
    m_commandList->ResourceBarrier(1, &barrier);

    return true;
}

// ============================================================================
// 编译着色器
// ============================================================================
bool D3DManager::CompileShaders()
{
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> errors;

    // 编译顶点着色器
    HRESULT hr = D3DCompileFromFile(
        L"Shaders.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VS", "vs_5_0",
        compileFlags, 0,
        &m_vsByteCode,
        &errors
    );

    if (errors != nullptr)
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
    }
    if (FAILED(hr)) return false;

    // 编译像素着色器
    hr = D3DCompileFromFile(
        L"Shaders.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PS", "ps_5_0",
        compileFlags, 0,
        &m_psByteCode,
        &errors
    );

    if (errors != nullptr)
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
    }
    if (FAILED(hr)) return false;

    return true;
}

// ============================================================================
// 构建根签名
// ============================================================================
bool D3DManager::BuildRootSignature()
{
    CD3DX12_ROOT_PARAMETER slotRootParameter[2];
    slotRootParameter[0].InitAsConstantBufferView(0); // b0: per-object
    slotRootParameter[1].InitAsConstantBufferView(1); // b1: per-pass (light/camera)

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
        _countof(slotRootParameter), slotRootParameter,
        0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &errorBlob
    );

    if (errorBlob != nullptr)
    {
        OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    if (FAILED(hr)) return false;

    if (FAILED(m_d3dDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature))))
        return false;

    return true;
}

// ============================================================================
// 构建管线状态对象
// ============================================================================
bool D3DManager::BuildPSO()
{
    // 顶点输入布局
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        // 位置在偏移 0
     { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
     // 法线在偏移 12
     { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
     // 颜色在偏移 24
    { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { m_vsByteCode->GetBufferPointer(), m_vsByteCode->GetBufferSize() };
    psoDesc.PS = { m_psByteCode->GetBufferPointer(), m_psByteCode->GetBufferSize() };

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    if (FAILED(m_d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState))))
        return false;

    return true;
}

// ============================================================================
// 构建常量缓冲区
// ============================================================================
bool D3DManager::BuildConstantBuffers()
{
    // 256 对齐
        m_objCBByteSize = (sizeof(ObjectConstants) + 255) & ~255;

    UINT bufferSize = m_objCBByteSize * MaxObjects;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    if (FAILED(m_d3dDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_objectCB))))
    {
        return false;
    }

    if (FAILED(m_objectCB->Map(0, nullptr, reinterpret_cast<void**>(&m_cbMappedData))))
        return false;

    // 这里只创建一个 CBV 指向整个 buffer，实际使用时用偏移
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_objectCB->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = m_objCBByteSize; // 根签名用的只是起始位置，这里设为一个对象大小即可

    m_d3dDevice->CreateConstantBufferView(
        &cbvDesc,
        m_cbvHeap->GetCPUDescriptorHandleForHeapStart());

    // -------- per-pass CB (b1) --------
    m_passCBByteSize = (sizeof(PassConstants) + 255) & ~255;

    CD3DX12_RESOURCE_DESC passDesc = CD3DX12_RESOURCE_DESC::Buffer(m_passCBByteSize);
    if (FAILED(m_d3dDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &passDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_passCB))))
    {
        return false;
    }

    if (FAILED(m_passCB->Map(0, nullptr, reinterpret_cast<void**>(&m_passCbMappedData))))
    {
        return false;
    }

    return true;
}

// ============================================================================
// 构建所有形状几何体
// ============================================================================
bool D3DManager::BuildShapeGeometry()
{
    // 重置命令列表
    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), nullptr);

    // 创建几何体模板
    m_sphereTemplate = std::make_shared<PrimitiveShape>();
    if (!m_sphereTemplate->Initialize(m_d3dDevice.Get(), m_commandList.Get(), (int)ShapeType::Sphere))
        return false;

    m_cylinderTemplate = std::make_shared<PrimitiveShape>();
    if (!m_cylinderTemplate->Initialize(m_d3dDevice.Get(), m_commandList.Get(), (int)ShapeType::Cylinder))
        return false;

    m_planeTemplate = std::make_shared<PrimitiveShape>();
    if (!m_planeTemplate->Initialize(m_d3dDevice.Get(), m_commandList.Get(), (int)ShapeType::Plane))
        return false;

    m_cubeTemplate = std::make_shared<PrimitiveShape>();
    if (!m_cubeTemplate->Initialize(m_d3dDevice.Get(), m_commandList.Get(), (int)ShapeType::Cube))
        return false;

    m_tetrahedronTemplate = std::make_shared<PrimitiveShape>();
    if (!m_tetrahedronTemplate->Initialize(m_d3dDevice.Get(), m_commandList.Get(), (int)ShapeType::Tetrahedron))
        return false;

    // 执行命令列表
    m_commandList->Close();
    ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    FlushCommandQueue();

    // 释放上传缓冲区
    m_sphereTemplate->DisposeUploaders();
    m_cylinderTemplate->DisposeUploaders();
    m_planeTemplate->DisposeUploaders();
    m_cubeTemplate->DisposeUploaders();
    m_tetrahedronTemplate->DisposeUploaders();

    return true;
}

// ============================================================================
// 添加对象到场景
// ============================================================================
void D3DManager::AddObject(ShapeType type, const XMFLOAT3& position)
{
    auto shapeTemplate = GetShapeTemplate(type);
    if (shapeTemplate)
    {
        auto obj = std::make_unique<SceneObject>(type, shapeTemplate);
        obj->SetPosition(position);
        m_sceneObjects.push_back(std::move(obj));
    }
}

// ============================================================================
// 清空场景
// ============================================================================
void D3DManager::ClearScene()
{
    m_sceneObjects.clear();
    m_selectedObject = nullptr;
}

// ============================================================================
// 获取形状模板
// ============================================================================
std::shared_ptr<PrimitiveShape> D3DManager::GetShapeTemplate(ShapeType type)
{
    switch (type)
    {
    case ShapeType::Sphere: return m_sphereTemplate;
    case ShapeType::Cylinder: return m_cylinderTemplate;
    case ShapeType::Plane: return m_planeTemplate;
    case ShapeType::Cube: return m_cubeTemplate;
    case ShapeType::Tetrahedron: return m_tetrahedronTemplate;
    default: return nullptr;
    }
}

void D3DManager::OnMouseDoubleClick(int x, int y)
{
    if (!m_editMode)
    {
        return;
    }

    // 如果没有选中对象，双击也尝试拾取一次
    if (!m_selectedObject)
    {
        SceneObject* pickedObj = PickObject(x, y);
        if (pickedObj)
        {
            if (m_selectedObject)
            {
                m_selectedObject->SetSelected(false);
            }

            m_selectedObject = pickedObj;
            m_selectedObject->SetSelected(true);
        }
    }

    if (!m_selectedObject)
    {
        return;
    }
    else {
        // 弹出属性对话框
		ShowTransformDialog(m_hWnd, m_selectedObject);
    }
}

void D3DManager::ShowLightSettingsDialog()
{
    if (!m_hWnd)
    {
        return;
    }
    (void)ShowLightDialog(m_hWnd, m_lightSettings);

}

// ============================================================================
// 渲染函数
// ============================================================================
void D3DManager::Render()
{
    m_commandAllocator->Reset();
    m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get());

    m_commandList->RSSetViewports(1, &m_screenViewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    CD3DX12_RESOURCE_BARRIER barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    m_commandList->ResourceBarrier(1, &barrier1);

    const float clearColor[] = { 0.2f, 0.3f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(CurrentBackBufferView(), clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(DepthStencilView(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CurrentBackBufferView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = DepthStencilView();
    m_commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    UpdateCamera();

    // 更新并绑定 per-pass 常量（b1）
    PassConstants pass{};
    pass.LightPosW = DirectX::XMFLOAT3(m_lightSettings.PosX, m_lightSettings.PosY, m_lightSettings.PosZ);
    pass.Ambient = m_lightSettings.Ambient;
    pass.Diffuse = m_lightSettings.Diffuse;
    pass.Specular = m_lightSettings.Specular;
    pass.Shininess = m_lightSettings.Shininess;
    pass.EyePosW = m_eyePos;

    memcpy(m_passCbMappedData, &pass, sizeof(pass));
    m_commandList->SetGraphicsRootConstantBufferView(1, m_passCB->GetGPUVirtualAddress());
    UINT objIndex = 0;

    for (auto& obj : m_sceneObjects)
    {
        if (objIndex >= MaxObjects)
            break; // 防御：不要越界

        // 更新该对象的 CB
        UpdateObjectCB(obj.get(), objIndex);

        // 为这个对象设置对应的 CBV GPU 地址
        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress =
            m_objectCB->GetGPUVirtualAddress() + objIndex * m_objCBByteSize;

        m_commandList->SetGraphicsRootConstantBufferView(0, objCBAddress);

        PrimitiveShape* shape = obj->GetShape();
        if (shape)
        {
            D3D12_VERTEX_BUFFER_VIEW vbv = shape->GetVertexBufferView();
            D3D12_INDEX_BUFFER_VIEW ibv = shape->GetIndexBufferView();

            m_commandList->IASetVertexBuffers(0, 1, &vbv);
            m_commandList->IASetIndexBuffer(&ibv);
            m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            m_commandList->DrawIndexedInstanced(shape->GetIndexCount(), 1, 0, 0, 0);
        }

        ++objIndex;
    }

    CD3DX12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    m_commandList->ResourceBarrier(1, &barrier2);

    m_commandList->Close();

    ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    m_swapChain->Present(0, 0);
    m_currBackBuffer = (m_currBackBuffer + 1) % SwapChainBufferCount;

    FlushCommandQueue();
}

// ============================================================================
// 更新摄像机
// ============================================================================
void D3DManager::UpdateCamera()
{
    XMVECTOR pos = XMLoadFloat3(&m_eyePos);
    XMVECTOR target = XMLoadFloat3(&m_lookAt);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&m_view, view);

    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(60.0f),
        static_cast<float>(m_clientWidth) / m_clientHeight,
        1.0f,
        1000.0f
    );
    XMStoreFloat4x4(&m_proj, proj);
}

// ============================================================================
// 更新常量缓冲区
// ============================================================================
void D3DManager::UpdateObjectCB(SceneObject* obj, UINT objectIndex)
{
    XMMATRIX world = obj->GetWorldMatrix();
    XMMATRIX view = XMLoadFloat4x4(&m_view);
    XMMATRIX proj = XMLoadFloat4x4(&m_proj);
    XMMATRIX worldViewProj = world * view * proj;

    ObjectConstants objConstants{};
    XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));

    if (obj->IsSelected())
    {
        objConstants.HighlightColor = XMFLOAT4(1.0f, 1.0f, 0.0f, 0.3f);
    }
    else
    {
        objConstants.HighlightColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    const auto& mat = obj->GetMaterial();
    objConstants.BaseColor = mat.BaseColor;
    objConstants.SpecularStrength = mat.SpecularStrength;
    objConstants.MatShininess = mat.Shininess;

    const auto mapping = obj->GetTextureMappingMode();
    objConstants.TexMappingMode = (int)mapping;
    objConstants.TexStyle = (int)obj->GetTextureStyle();

    // 先给默认：可以后放进对话框参数
    objConstants.TexScale = 1.0f;
    objConstants.TexOffsetU = 0.0f;
    objConstants.TexOffsetV = 0.0f;

    UINT offset = objectIndex * m_objCBByteSize;
    memcpy(m_cbMappedData + offset, &objConstants, sizeof(ObjectConstants));
}

// ============================================================================
// 鼠标按下事件
// ============================================================================
void D3DManager::OnMouseDown(int x, int y)
{
    m_isDragging = true;
    m_lastMouseX = x;
    m_lastMouseY = y;

    SceneObject* pickedObj = PickObject(x, y);

    if (m_selectedObject)
    {
        m_selectedObject->SetSelected(false);
    }

    m_selectedObject = pickedObj;
    if (m_selectedObject)
    {
        m_selectedObject->SetSelected(true);
        //MessageBox(m_hWnd, L"选中对象", L"Pick", MB_OK); // 调试用
    }
    else
    {
        //MessageBox(m_hWnd, L"未选中任何对象", L"Pick", MB_OK); // 调试用
    }
}

// ============================================================================
// 鼠标移动事件
// ============================================================================
void D3DManager::OnMouseMove(int x, int y)
{
    if (!m_isDragging) { return; }
    int dx = x - m_lastMouseX;
    int dy = y - m_lastMouseY;

    // 选中对象时：拖动物体
    if (m_selectedObject)
    {
        // 按相机坐标系平移选中物体
         // dx: 沿相机右方向，dy: 沿相机上方向（屏幕向下为负）
        float moveScale = 0.01f; // 鼠标像素 -> 世界单位的比例，可自行调节

        // 当前相机参数
        XMVECTOR eye = XMLoadFloat3(&m_eyePos);
        XMVECTOR target = XMLoadFloat3(&m_lookAt);
        XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        // 相机前/右/上方向（世界坐标）
        XMVECTOR forward = XMVector3Normalize(target - eye);
        XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, forward));
        XMVECTOR camUp = XMVector3Normalize(XMVector3Cross(forward, right));

        // 根据鼠标移动计算世界空间位移
        XMVECTOR delta = XMVectorZero();
        if (dx != 0)
        {
            delta += right * (dx * moveScale);
        }
        if (dy != 0)
        {
            delta += camUp * (-dy * moveScale); // 屏幕向下为正，所以上方向取 -dy
        }

        XMFLOAT3 currentPos = m_selectedObject->GetPosition();
        XMVECTOR pos = XMLoadFloat3(&currentPos);
        pos += delta;
        XMStoreFloat3(&currentPos, pos);

        m_selectedObject->SetPosition(currentPos);
    }
    else
    {
        // 未选中对象时：旋转摄像机
        // 以 m_lookAt 为中心，绕 Y 轴和 X 轴旋转 eyePos
        float rotateSpeed = 0.005f; // 调整灵敏度

        float yaw = -dx * rotateSpeed; // 水平
        float pitch = -dy * rotateSpeed; // 垂直

        // eyePos 相对中心点的向量
        XMVECTOR center = XMLoadFloat3(&m_lookAt);
        XMVECTOR eye = XMLoadFloat3(&m_eyePos);
        XMVECTOR offset = eye - center;

        // 绕 Y 轴旋转（水平）
        XMMATRIX rotY = XMMatrixRotationY(yaw);
        offset = XMVector3TransformNormal(offset, rotY);

        // 计算当前到中心的距离和仰角，避免 pitch 过大翻转
        XMVECTOR rVec = offset;
        float radius = XMVectorGetX(XMVector3Length(rVec));

        if (radius > 0.0001f)
        {
            // 把 offset 转换到球坐标，限制 pitch
            float cx = XMVectorGetX(offset);
            float cy = XMVectorGetY(offset);
            float cz = XMVectorGetZ(offset);

            float theta = atan2f(cz, cx);         // 水平角
            float phi = acosf(cy / radius);     // 0..pi

            phi += pitch;
            const float phiMin = 0.1f;
            const float phiMax = XM_PI - 0.1f;
            if (phi < phiMin) phi = phiMin;
            if (phi > phiMax) phi = phiMax;

            // 重新由球坐标转回笛卡尔坐标
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);
            float cosTheta = cosf(theta);
            float sinTheta = sinf(theta);

            float nx = radius * sinPhi * cosTheta;
            float ny = radius * cosPhi;
            float nz = radius * sinPhi * sinTheta;

            offset = XMVectorSet(nx, ny, nz, 0.0f);
        }

        // 更新 eyePos
        eye = center + offset;
        XMStoreFloat3(&m_eyePos, eye);
    }

    m_lastMouseX = x;
    m_lastMouseY = y;
}

// ============================================================================
// 鼠标释放事件
// ============================================================================
void D3DManager::OnMouseUp()
{
    m_isDragging = false;
}

// ============================================================================
// 鼠标滚轮事件
// ============================================================================
void D3DManager::OnMouseWheel(int delta)
{
    if (m_selectedObject)
    {
        // 滚轮沿摄像机前/后方向移动选中物体
         // delta > 0：向前靠近相机视线方向；delta < 0：远离
        float moveStep = 0.5f; // 单位可自行调整
        float amount = (delta > 0) ? moveStep : -moveStep;

        // 相机参数
        XMVECTOR eye = XMLoadFloat3(&m_eyePos);
        XMVECTOR target = XMLoadFloat3(&m_lookAt);

        // 相机前方向（世界坐标）
        XMVECTOR forward = XMVector3Normalize(target - eye);

        // 当前位置 + 前方向 * amount
        XMFLOAT3 currentPos = m_selectedObject->GetPosition();
        XMVECTOR pos = XMLoadFloat3(&currentPos);

        pos += forward * amount;

        XMStoreFloat3(&currentPos, pos);
        m_selectedObject->SetPosition(currentPos);
    }
}

// ============================================================================
// 射线拾取对象
// ============================================================================
SceneObject* D3DManager::PickObject(int mouseX, int mouseY)
{
    XMVECTOR rayOrigin, rayDir;
    ScreenToWorldRay(mouseX, mouseY, rayOrigin, rayDir);

    float minDistance = FLT_MAX;
    SceneObject* closestObject = nullptr;

    // 遍历所有对象，找到最近的相交对象
    for (auto& obj : m_sceneObjects)
    {
        float distance;
        if (obj->IntersectRay(rayOrigin, rayDir, distance))
        {
            if (distance < minDistance)
            {
                minDistance = distance;
                closestObject = obj.get();
            }
        }
    }

    return closestObject;
}

// ============================================================================
// 屏幕坐标转世界射线
// ============================================================================
void D3DManager::ScreenToWorldRay(int mouseX, int mouseY,
    XMVECTOR& rayOrigin,
    XMVECTOR& rayDir)
{
    // 屏幕 -> NDC
    float ndcX = (2.0f * mouseX) / m_clientWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / m_clientHeight;

    // 准备矩阵
    XMMATRIX proj = XMLoadFloat4x4(&m_proj);
    XMMATRIX view = XMLoadFloat4x4(&m_view);
    XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
    XMMATRIX invView = XMMatrixInverse(nullptr, view);

    // 近裁面和远裁面上的点（NDC）
    XMVECTOR ndcNear = XMVectorSet(ndcX, ndcY, 0.0f, 1.0f);
    XMVECTOR ndcFar = XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);

    // NDC -> 视空间
    XMVECTOR viewNear = XMVector4Transform(ndcNear, invProj);
    XMVECTOR viewFar = XMVector4Transform(ndcFar, invProj);

    viewNear = XMVectorScale(viewNear, 1.0f / XMVectorGetW(viewNear));
    viewFar = XMVectorScale(viewFar, 1.0f / XMVectorGetW(viewFar));

    // 视空间 -> 世界空间
    XMVECTOR worldNear = XMVector4Transform(viewNear, invView);
    XMVECTOR worldFar = XMVector4Transform(viewFar, invView);

    worldNear = XMVectorSetW(worldNear, 1.0f);
    worldFar = XMVectorSetW(worldFar, 1.0f);

    rayOrigin = worldNear;
    rayDir = XMVector3Normalize(worldFar - worldNear);
}

// ============================================================================
// 刷新命令队列（等待GPU完成）
// ============================================================================
void D3DManager::FlushCommandQueue()
{
    m_currentFence++;

    m_commandQueue->Signal(m_fence.Get(), m_currentFence);

    if (m_fence->GetCompletedValue() < m_currentFence)
    {
        m_fence->SetEventOnCompletion(m_currentFence, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

// ============================================================================
// 窗口大小改变
// ============================================================================
void D3DManager::OnResize(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;

    m_clientWidth = width;
    m_clientHeight = height;

    FlushCommandQueue();

    m_commandList->Reset(m_commandAllocator.Get(), nullptr);

    // 释放旧资源
    for (int i = 0; i < SwapChainBufferCount; ++i)
        m_swapChainBuffer[i].Reset();
    m_depthStencilBuffer.Reset();

    // 调整交换链大小
    m_swapChain->ResizeBuffers(
        SwapChainBufferCount,
        m_clientWidth, m_clientHeight,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
    );

    m_currBackBuffer = 0;

    // 重新创建RTV
    CreateRenderTargets();

    // 重新创建深度模板缓冲区
    CreateDepthStencil();

    m_commandList->Close();
    ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    FlushCommandQueue();

    // 更新视口
    m_screenViewport.TopLeftX = 0;
    m_screenViewport.TopLeftY = 0;
    m_screenViewport.Width = static_cast<float>(m_clientWidth);
    m_screenViewport.Height = static_cast<float>(m_clientHeight);
    m_screenViewport.MinDepth = 0.0f;
    m_screenViewport.MaxDepth = 1.0f;

    m_scissorRect = { 0, 0, m_clientWidth, m_clientHeight };
}

// ============================================================================
// 清理资源
// ============================================================================
void D3DManager::Cleanup()
{
    if (m_d3dDevice != nullptr)
        FlushCommandQueue();

    if (m_objectCB != nullptr && m_cbMappedData != nullptr)
    {
        m_objectCB->Unmap(0, nullptr);
        m_cbMappedData = nullptr;
    }
    if (m_passCB != nullptr && m_passCbMappedData != nullptr)
    {
        m_passCB->Unmap(0, nullptr);
        m_passCbMappedData = nullptr;
    }

    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
}

// ============================================================================
// 辅助函数
// ============================================================================
D3D12_CPU_DESCRIPTOR_HANDLE D3DManager::CurrentBackBufferView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        m_currBackBuffer,
        m_rtvDescriptorSize
    );
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DManager::DepthStencilView() const
{
    return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

ID3D12Resource* D3DManager::CurrentBackBuffer() const
{
    return m_swapChainBuffer[m_currBackBuffer].Get();
}