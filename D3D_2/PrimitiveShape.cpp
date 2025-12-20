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

    // 顶点：北极
    Vertex topVertex;
    topVertex.Pos = XMFLOAT3(0.0f, radius, 0.0f);
    topVertex.Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
    topVertex.Color = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f); // 灰色
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
            float x = radius * sinf(phi) * cosf(theta);
            float y = radius * cosf(phi);
            float z = radius * sinf(phi) * sinf(theta);

            v.Pos = XMFLOAT3(x, y, z);

            // 法线 = 归一化的 (x, y, z) / radius，这里半径为 1 所以直接用单位向量
            XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&v.Pos));
            XMStoreFloat3(&v.Normal, n);

            v.Color = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f); // 灰色

            vertices.push_back(v);
        }
    }

    // 顶点：南极
    Vertex bottomVertex;
    bottomVertex.Pos = XMFLOAT3(0.0f, -radius, 0.0f);
    bottomVertex.Normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
    bottomVertex.Color = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f); // 灰色
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

    XMFLOAT4 gray(0.5f, 0.5f, 0.5f, 1.0f);
    XMFLOAT3 n(0.0f, 1.0f, 0.0f);


    // ------------------ 侧面顶点 ------------------
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
			vertex.Normal = XMFLOAT3(c, 0.0f, s); // 侧面法线
            vertex.Color = gray;

            vertices.push_back(vertex);
        }
    }

    // ------------------ 侧面索引 ------------------
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

    float dTheta = 2.0f * PI / sliceCount;

    // ------------------ 顶盖 ------------------
    // 顶盖圆环起始索引
    UINT topBaseIndex = (UINT)vertices.size();
    float topY = 0.5f * height;

    for (UINT i = 0; i <= sliceCount; ++i)
    {
        float x = topRadius * cosf(i * dTheta);
        float z = topRadius * sinf(i * dTheta);

        Vertex v;
        v.Pos = XMFLOAT3(x, topY, z);
        v.Normal = n;
        v.Color = gray;
        vertices.push_back(v);
    }

    // 顶盖中心点
    Vertex topCenter;
    topCenter.Pos = XMFLOAT3(0.0f, topY, 0.0f);
	topCenter.Normal = n;
    topCenter.Color = gray;
    vertices.push_back(topCenter);

    UINT topCenterIndex = (UINT)vertices.size() - 1;

    // 顶盖索引（注意三角形朝外的顺序）
    for (UINT i = 0; i < sliceCount; ++i)
    {
        indices.push_back(topCenterIndex);
        indices.push_back(topBaseIndex + i + 1);
        indices.push_back(topBaseIndex + i);
    }

    // ------------------ 底盖 ------------------
    // 底盖圆环起始索引
    UINT bottomBaseIndex = (UINT)vertices.size();
    float bottomY = -0.5f * height;

    for (UINT i = 0; i <= sliceCount; ++i)
    {
        float x = bottomRadius * cosf(i * dTheta);
        float z = bottomRadius * sinf(i * dTheta);

        Vertex v;
        v.Pos = XMFLOAT3(x, bottomY, z);
		v.Normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
        v.Color = gray;
        vertices.push_back(v);
    }

    // 底盖中心点
    Vertex bottomCenter;
    bottomCenter.Pos = XMFLOAT3(0.0f, bottomY, 0.0f);
	bottomCenter.Normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
    bottomCenter.Color = gray;
    vertices.push_back(bottomCenter);

    UINT bottomCenterIndex = (UINT)vertices.size() - 1;

    // 底盖索引（反向顺序，保持法线朝外）
    for (UINT i = 0; i < sliceCount; ++i)
    {
        indices.push_back(bottomCenterIndex);
        indices.push_back(bottomBaseIndex + i);
        indices.push_back(bottomBaseIndex + i + 1);
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
    XMFLOAT3 n(0.0f, 1.0f, 0.0f);

    vertices[0].Pos = XMFLOAT3(-width / 2, 0.0f, -depth / 2);
    vertices[0].Normal = n;
    vertices[0].Color = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

    vertices[1].Pos = XMFLOAT3(-width / 2, 0.0f, depth / 2);
    vertices[1].Normal = n;
    vertices[1].Color = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

    vertices[2].Pos = XMFLOAT3(width / 2, 0.0f, depth / 2);
    vertices[2].Normal = n;
    vertices[2].Color = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

    vertices[3].Pos = XMFLOAT3(width / 2, 0.0f, -depth / 2);
    vertices[3].Normal = n;
    vertices[3].Color = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

    indices = { 0, 1, 2, 0, 2, 3 };
}

// ============================================================================
// 构建立方体
// ============================================================================
void PrimitiveShape::BuildCube(std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices)
{
    XMFLOAT4 gray(0.5f, 0.5f, 0.5f, 1.0f);

    vertices.clear();
    vertices.reserve(24);

    indices.clear();
    indices.reserve(36);

    auto addFace = [&](const XMFLOAT3& desiredN, XMFLOAT3 v0, XMFLOAT3 v1, XMFLOAT3 v2, XMFLOAT3 v3)
        {
            // 自动检测 winding 是否与 desiredN 一致；不一致则翻转
            XMVECTOR p0 = XMLoadFloat3(&v0);
            XMVECTOR p1 = XMLoadFloat3(&v1);
            XMVECTOR p2 = XMLoadFloat3(&v2);

            XMVECTOR geomN = XMVector3Normalize(XMVector3Cross(p1 - p0, p2 - p0));
            XMVECTOR wantN = XMLoadFloat3(&desiredN);

            if (XMVectorGetX(XMVector3Dot(geomN, wantN)) < 0.0f)
            {
                // 翻转四边形：交换 v1 和 v3
                std::swap(v1, v3);
            }

            std::uint16_t base = static_cast<std::uint16_t>(vertices.size());

            vertices.push_back(Vertex{ v0, desiredN, gray });
            vertices.push_back(Vertex{ v1, desiredN, gray });
            vertices.push_back(Vertex{ v2, desiredN, gray });
            vertices.push_back(Vertex{ v3, desiredN, gray });

            // 两个三角形
            indices.push_back(base + 0);
            indices.push_back(base + 1);
            indices.push_back(base + 2);

            indices.push_back(base + 0);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
        };

    // 每个面给出“想要的外法线” + 4 个顶点（顺序无需保证，addFace 会自动校正）

    // +Z
    addFace(XMFLOAT3(0.0f, 0.0f, 1.0f),
        XMFLOAT3(-1.0f, -1.0f, 1.0f),
        XMFLOAT3(-1.0f, 1.0f, 1.0f),
        XMFLOAT3(1.0f, 1.0f, 1.0f),
        XMFLOAT3(1.0f, -1.0f, 1.0f));

    // -Z
    addFace(XMFLOAT3(0.0f, 0.0f, -1.0f),
        XMFLOAT3(-1.0f, -1.0f, -1.0f),
        XMFLOAT3(-1.0f, 1.0f, -1.0f),
        XMFLOAT3(1.0f, 1.0f, -1.0f),
        XMFLOAT3(1.0f, -1.0f, -1.0f));

    // -X
    addFace(XMFLOAT3(-1.0f, 0.0f, 0.0f),
        XMFLOAT3(-1.0f, -1.0f, -1.0f),
        XMFLOAT3(-1.0f, -1.0f, 1.0f),
        XMFLOAT3(-1.0f, 1.0f, 1.0f),
        XMFLOAT3(-1.0f, 1.0f, -1.0f));

    // +X
    addFace(XMFLOAT3(1.0f, 0.0f, 0.0f),
        XMFLOAT3(1.0f, -1.0f, -1.0f),
        XMFLOAT3(1.0f, 1.0f, -1.0f),
        XMFLOAT3(1.0f, 1.0f, 1.0f),
        XMFLOAT3(1.0f, -1.0f, 1.0f));

    // +Y
    addFace(XMFLOAT3(0.0f, 1.0f, 0.0f),
        XMFLOAT3(-1.0f, 1.0f, -1.0f),
        XMFLOAT3(-1.0f, 1.0f, 1.0f),
        XMFLOAT3(1.0f, 1.0f, 1.0f),
        XMFLOAT3(1.0f, 1.0f, -1.0f));

    // -Y
    addFace(XMFLOAT3(0.0f, -1.0f, 0.0f),
        XMFLOAT3(-1.0f, -1.0f, 1.0f),
        XMFLOAT3(-1.0f, -1.0f, -1.0f),
        XMFLOAT3(1.0f, -1.0f, -1.0f),
        XMFLOAT3(1.0f, -1.0f, 1.0f));
}

// ============================================================================
// 构建四面体
// ============================================================================
void PrimitiveShape::BuildTetrahedron(std::vector<Vertex>& vertices, std::vector<std::uint16_t>& indices)
{
    // 使用你原来的顶点位置
    const float a = 1.5f;
    const float h = sqrtf(2.0f / 3.0f) * a;

    XMFLOAT3 p0(0.0f, h, 0.0f);
    XMFLOAT3 p1(-a / 2, -h / 3, a * sqrtf(3.0f) / 6);
    XMFLOAT3 p2(a / 2, -h / 3, a * sqrtf(3.0f) / 6);
    XMFLOAT3 p3(0.0f, -h / 3, -a * sqrtf(3.0f) / 3);

    XMFLOAT4 gray(0.5f, 0.5f, 0.5f, 1.0f);

    vertices.clear();
    vertices.reserve(12);

    auto faceNormal = [](const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& c)
        {
            XMVECTOR va = XMLoadFloat3(&a);
            XMVECTOR vb = XMLoadFloat3(&b);
            XMVECTOR vc = XMLoadFloat3(&c);

            XMVECTOR n = XMVector3Normalize(XMVector3Cross(vb - va, vc - va));
            XMFLOAT3 out;
            XMStoreFloat3(&out, n);
            return out;
        };

    auto addTri = [&](const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& c)
        {
            XMFLOAT3 n = faceNormal(a, b, c);
            vertices.push_back(Vertex{ a, n, gray });
            vertices.push_back(Vertex{ b, n, gray });
            vertices.push_back(Vertex{ c, n, gray });
        };

    // 4 个面（确保 winding 一致；如果发现某个面变黑，交换 b/c 即可）
    addTri(p0, p2, p1);
    addTri(p0, p1, p3);
    addTri(p0, p3, p2);
    addTri(p1, p2, p3);

    indices.clear();
    indices.reserve(12);
    for (std::uint16_t i = 0; i < 12; ++i)
    {
        indices.push_back(i);
    }
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