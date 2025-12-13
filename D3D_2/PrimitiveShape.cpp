#include "PrimitiveShape.h"
#include "D3DManager.h"
#include <cmath>

using namespace DirectX;

const float PI = 3.14159265359f;

// ============================================================================
// 构造函数和析构函数
// ============================================================================
PrimitiveShape::PrimitiveShape()
{
}

PrimitiveShape::~PrimitiveShape()
{
}

// ============================================================================
// 初始化形状
// ============================================================================
bool PrimitiveShape::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, int shapeType)
{
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;

    // 根据形状类型生成几何数据
    switch (shapeType)
    {
    case 1: // Sphere
        BuildSphere(vertices, indices);
        break;
    case 2: // Cylinder
        BuildCylinder(vertices, indices);
        break;
    case 3: // Plane
        BuildPlane(vertices, indices);
        break;
    case 4: // Cube
        BuildCube(vertices, indices);
        break;
    case 5: // Tetrahedron
        BuildTetrahedron(vertices, indices);
        break;
    default:
        return false;
    }

    // 上传几何数据到GPU
    return UploadGeometry(device, commandList, vertices, indices);
}

// ============================================================================
// 构建球体
// ============================================================================
void PrimitiveShape::BuildSphere(std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices)
{
    const float radius = 1.0f;
    const UINT sliceCount = 20;
    const UINT stackCount = 20;

    // 顶点
    Vertex topVertex;
    topVertex.Pos = XMFLOAT3(0.0f, radius, 0.0f);
    topVertex.Color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    vertices.push_back(topVertex);

    float phiStep = PI / stackCount;
    float thetaStep = 2.0f * PI / sliceCount;

    for (UINT i = 1; i <= stackCount - 1; ++i)
    {
        float phi = i * phiStep;
        for (UINT j = 0; j <= sliceCount; ++j)
        {
            float theta = j * thetaStep;

            Vertex v;
            v.Pos.x = radius * sinf(phi) * cosf(theta);
            v.Pos.y = radius * cosf(phi);
            v.Pos.z = radius * sinf(phi) * sinf(theta);

            float r = (float)i / stackCount;
            float g = (float)j / sliceCount;
            v.Color = XMFLOAT4(r, g, 1.0f - r, 1.0f);

            vertices.push_back(v);
        }
    }

    Vertex bottomVertex;
    bottomVertex.Pos = XMFLOAT3(0.0f, -radius, 0.0f);
    bottomVertex.Color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
    vertices.push_back(bottomVertex);

    // 索引 - 顶部
    for (UINT i = 1; i <= sliceCount; ++i)
    {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i);
    }

    // 索引 - 中间
    UINT baseIndex = 1;
    UINT ringVertexCount = sliceCount + 1;
    for (UINT i = 0; i < stackCount - 2; ++i)
    {
        for (UINT j = 0; j < sliceCount; ++j)
        {
            indices.push_back(baseIndex + i * ringVertexCount + j);
            indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

            indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
        }
    }

    // 索引 - 底部
    UINT southPoleIndex = (UINT)vertices.size() - 1;
    baseIndex = southPoleIndex - ringVertexCount;
    for (UINT i = 0; i < sliceCount; ++i)
    {
        indices.push_back(southPoleIndex);
        indices.push_back(baseIndex + i);
        indices.push_back(baseIndex + i + 1);
    }
}

// ============================================================================
// 构建柱体
// ============================================================================
void PrimitiveShape::BuildCylinder(std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices)
{
    const float topRadius = 1.0f;
    const float bottomRadius = 1.0f;
    const float height = 2.0f;
    const UINT sliceCount = 20;
    const UINT stackCount = 10;

    float stackHeight = height / stackCount;
    float radiusStep = (topRadius - bottomRadius) / stackCount;
    UINT ringCount = stackCount + 1;

    // 侧面顶点
    for (UINT i = 0; i < ringCount; ++i)
    {
        float y = -0.5f * height + i * stackHeight;
        float r = bottomRadius + i * radiusStep;
        float dTheta = 2.0f * PI / sliceCount;

        for (UINT j = 0; j <= sliceCount; ++j)
        {
            Vertex vertex;
            float c = cosf(j * dTheta);
            float s = sinf(j * dTheta);

            vertex.Pos = XMFLOAT3(r * c, y, r * s);

            float colorVal = (float)i / stackCount;
            vertex.Color = XMFLOAT4(colorVal, 1.0f - colorVal, 0.5f, 1.0f);

            vertices.push_back(vertex);
        }
    }

    // 侧面索引
    UINT ringVertexCount = sliceCount + 1;
    for (UINT i = 0; i < stackCount; ++i)
    {
        for (UINT j = 0; j < sliceCount; ++j)
        {
            indices.push_back(i * ringVertexCount + j);
            indices.push_back((i + 1) * ringVertexCount + j);
            indices.push_back((i + 1) * ringVertexCount + j + 1);

            indices.push_back(i * ringVertexCount + j);
            indices.push_back((i + 1) * ringVertexCount + j + 1);
            indices.push_back(i * ringVertexCount + j + 1);
        }
    }

    // 顶部中心
    UINT baseIndex = (UINT)vertices.size();
    float y = 0.5f * height;
    float dTheta = 2.0f * PI / sliceCount;

    for (UINT i = 0; i <= sliceCount; ++i)
    {
        float x = topRadius * cosf(i * dTheta);
        float z = topRadius * sinf(i * dTheta);

        Vertex v;
        v.Pos = XMFLOAT3(x, y, z);
        v.Color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
        vertices.push_back(v);
    }

    Vertex topCenter;
    topCenter.Pos = XMFLOAT3(0.0f, y, 0.0f);
    topCenter.Color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    vertices.push_back(topCenter);

    UINT centerIndex = (UINT)vertices.size() - 1;
    for (UINT i = 0; i < sliceCount; ++i)
    {
        indices.push_back(centerIndex);
        indices.push_back(baseIndex + i + 1);
        indices.push_back(baseIndex + i);
    }

    // 底部
    baseIndex = (UINT)vertices.size();
    y = -0.5f * height;

    for (UINT i = 0; i <= sliceCount; ++i)
    {
        float x = bottomRadius * cosf(i * dTheta);
        float z = bottomRadius * sinf(i * dTheta);

        Vertex v;
        v.Pos = XMFLOAT3(x, y, z);
        v.Color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
        vertices.push_back(v);
    }

    Vertex bottomCenter;
    bottomCenter.Pos = XMFLOAT3(0.0f, y, 0.0f);
    bottomCenter.Color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
    vertices.push_back(bottomCenter);

    centerIndex = (UINT)vertices.size() - 1;
    for (UINT i = 0; i < sliceCount; ++i)
    {
        indices.push_back(centerIndex);
        indices.push_back(baseIndex + i);
        indices.push_back(baseIndex + i + 1);
    }
}

// ============================================================================
// 构建平面
// ============================================================================
void PrimitiveShape::BuildPlane(std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices)
{
    const float width = 2.0f;
    const float depth = 2.0f;

    vertices.resize(4);

    vertices[0].Pos = XMFLOAT3(-width / 2, 0.0f, -depth / 2);
    vertices[0].Color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

    vertices[1].Pos = XMFLOAT3(-width / 2, 0.0f, depth / 2);
    vertices[1].Color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

    vertices[2].Pos = XMFLOAT3(width / 2, 0.0f, depth / 2);
    vertices[2].Color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

    vertices[3].Pos = XMFLOAT3(width / 2, 0.0f, -depth / 2);
    vertices[3].Color = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);

    indices = { 0, 1, 2, 0, 2, 3 };
}

// ============================================================================
// 构建立方体
// ============================================================================
void PrimitiveShape::BuildCube(std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices)
{
    vertices.resize(8);

    // 前面
    vertices[0].Pos = XMFLOAT3(-1.0f, -1.0f, -1.0f);
    vertices[0].Color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

    vertices[1].Pos = XMFLOAT3(-1.0f, 1.0f, -1.0f);
    vertices[1].Color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

    vertices[2].Pos = XMFLOAT3(1.0f, 1.0f, -1.0f);
    vertices[2].Color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

    vertices[3].Pos = XMFLOAT3(1.0f, -1.0f, -1.0f);
    vertices[3].Color = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);

    // 后面
    vertices[4].Pos = XMFLOAT3(-1.0f, -1.0f, 1.0f);
    vertices[4].Color = XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);

    vertices[5].Pos = XMFLOAT3(-1.0f, 1.0f, 1.0f);
    vertices[5].Color = XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f);

    vertices[6].Pos = XMFLOAT3(1.0f, 1.0f, 1.0f);
    vertices[6].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    vertices[7].Pos = XMFLOAT3(1.0f, -1.0f, 1.0f);
    vertices[7].Color = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

    indices = {
        // 前面
        0, 1, 2, 0, 2, 3,
        // 后面
        4, 6, 5, 4, 7, 6,
        // 左面
        4, 5, 1, 4, 1, 0,
        // 右面
        3, 2, 6, 3, 6, 7,
        // 上面
        1, 5, 6, 1, 6, 2,
        // 下面
        4, 0, 3, 4, 3, 7
    };
}

// ============================================================================
// 构建四面体
// ============================================================================
void PrimitiveShape::BuildTetrahedron(std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices)
{
    const float a = 1.5f;
    const float h = sqrtf(2.0f / 3.0f) * a;

    vertices.resize(4);

    // 顶点
    vertices[0].Pos = XMFLOAT3(0.0f, h, 0.0f);
    vertices[0].Color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

    vertices[1].Pos = XMFLOAT3(-a / 2, -h / 3, a * sqrtf(3.0f) / 6);
    vertices[1].Color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

    vertices[2].Pos = XMFLOAT3(a / 2, -h / 3, a * sqrtf(3.0f) / 6);
    vertices[2].Color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

    vertices[3].Pos = XMFLOAT3(0.0f, -h / 3, -a * sqrtf(3.0f) / 3);
    vertices[3].Color = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);

    // 索引
    indices = {
        0, 2, 1,  // 前面
        0, 1, 3,  // 左面
        0, 3, 2,  // 右面
        1, 2, 3   // 底面
    };
}

// ============================================================================
// 上传几何数据到GPU
// ============================================================================
bool PrimitiveShape::UploadGeometry(ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList,
    const std::vector<Vertex>& vertices,
    const std::vector<std::uint16_t>& indices)
{
    m_vertexByteStride = sizeof(Vertex);
    m_vertexBufferByteSize = (UINT)vertices.size() * sizeof(Vertex);
    m_indexFormat = DXGI_FORMAT_R16_UINT;
    m_indexBufferByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
    m_indexCount = (UINT)indices.size();

    // 创建默认堆（GPU专用）的顶点缓冲区
    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferByteSize);

    if (FAILED(device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_vertexBufferGPU))))
    {
        return false;
    }

    // 创建上传堆的顶点缓冲区
    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);

    if (FAILED(device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &vertexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_vertexBufferUploader))))
    {
        return false;
    }

    // 创建默认堆的索引缓冲区
    CD3DX12_RESOURCE_DESC indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferByteSize);

    if (FAILED(device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_indexBufferGPU))))
    {
        return false;
    }

    // 创建上传堆的索引缓冲区
    if (FAILED(device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &indexBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_indexBufferUploader))))
    {
        return false;
    }

    // 将顶点数据复制到上传堆
    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = vertices.data();
    vertexData.RowPitch = m_vertexBufferByteSize;
    vertexData.SlicePitch = vertexData.RowPitch;

    // 转换资源状态
    CD3DX12_RESOURCE_BARRIER vertexBarrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_vertexBufferGPU.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_DEST
    );
    commandList->ResourceBarrier(1, &vertexBarrier1);

    // 使用辅助函数更新子资源
    BYTE* pVertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    m_vertexBufferUploader->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    memcpy(pVertexDataBegin, vertices.data(), m_vertexBufferByteSize);
    m_vertexBufferUploader->Unmap(0, nullptr);

    commandList->CopyBufferRegion(m_vertexBufferGPU.Get(), 0, m_vertexBufferUploader.Get(), 0, m_vertexBufferByteSize);

    CD3DX12_RESOURCE_BARRIER vertexBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_vertexBufferGPU.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ
    );
    commandList->ResourceBarrier(1, &vertexBarrier2);

    // 将索引数据复制到上传堆
    D3D12_SUBRESOURCE_DATA indexData = {};
    indexData.pData = indices.data();
    indexData.RowPitch = m_indexBufferByteSize;
    indexData.SlicePitch = indexData.RowPitch;

    CD3DX12_RESOURCE_BARRIER indexBarrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_indexBufferGPU.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_DEST
    );
    commandList->ResourceBarrier(1, &indexBarrier1);

    BYTE* pIndexDataBegin;
    m_indexBufferUploader->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
    memcpy(pIndexDataBegin, indices.data(), m_indexBufferByteSize);
    m_indexBufferUploader->Unmap(0, nullptr);

    commandList->CopyBufferRegion(m_indexBufferGPU.Get(), 0, m_indexBufferUploader.Get(), 0, m_indexBufferByteSize);

    CD3DX12_RESOURCE_BARRIER indexBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_indexBufferGPU.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ
    );
    commandList->ResourceBarrier(1, &indexBarrier2);

    return true;
}

// ============================================================================
// 获取顶点缓冲区视图
// ============================================================================
D3D12_VERTEX_BUFFER_VIEW PrimitiveShape::GetVertexBufferView() const
{
    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();
    vbv.StrideInBytes = m_vertexByteStride;
    vbv.SizeInBytes = m_vertexBufferByteSize;
    return vbv;
}

// ============================================================================
// 获取索引缓冲区视图
// ============================================================================
D3D12_INDEX_BUFFER_VIEW PrimitiveShape::GetIndexBufferView() const
{
    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
    ibv.Format = m_indexFormat;
    ibv.SizeInBytes = m_indexBufferByteSize;
    return ibv;
}

// ============================================================================
// 释放上传缓冲区
// ============================================================================
void PrimitiveShape::DisposeUploaders()
{
    m_vertexBufferUploader.Reset();
    m_indexBufferUploader.Reset();
}