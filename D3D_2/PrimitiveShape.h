#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include <cstdint>
#include "d3dx12.h"

using Microsoft::WRL::ComPtr;

// 顶点结构（与 D3DManager.h 中的定义相同）
struct Vertex;

// 几何体数据类
class PrimitiveShape
{
public:
    PrimitiveShape();
    ~PrimitiveShape();

    // 初始化指定类型的形状
    bool Initialize(ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        int shapeType);

    // 获取顶点缓冲区视图
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;

    // 获取索引缓冲区视图
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;

    // 获取索引数量
    UINT GetIndexCount() const { return m_indexCount; }

    // 释放上传缓冲区（在GPU复制完成后调用）
    void DisposeUploaders();

private:
    // GPU 资源
    ComPtr<ID3D12Resource> m_vertexBufferGPU;
    ComPtr<ID3D12Resource> m_indexBufferGPU;

    // 上传堆（临时用于数据传输）
    ComPtr<ID3D12Resource> m_vertexBufferUploader;
    ComPtr<ID3D12Resource> m_indexBufferUploader;

    // 缓冲区属性
    UINT m_vertexByteStride = 0;
    UINT m_vertexBufferByteSize = 0;
    DXGI_FORMAT m_indexFormat = DXGI_FORMAT_R16_UINT;
    UINT m_indexBufferByteSize = 0;
    UINT m_indexCount = 0;

private:
    // 几何体生成函数
    void BuildSphere(std::vector<Vertex>& vertices,
        std::vector<std::uint16_t>& indices);

    void BuildCylinder(std::vector<Vertex>& vertices,
        std::vector<std::uint16_t>& indices);

    void BuildPlane(std::vector<Vertex>& vertices,
        std::vector<std::uint16_t>& indices);

    void BuildCube(std::vector<Vertex>& vertices,
        std::vector<std::uint16_t>& indices);

    void BuildTetrahedron(std::vector<Vertex>& vertices,
        std::vector<std::uint16_t>& indices);

    // 上传几何数据到GPU
    bool UploadGeometry(ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList,
        const std::vector<Vertex>& vertices,
        const std::vector<std::uint16_t>& indices);
};