#pragma once

#include <DirectXMath.h>
#include <memory>
#include <string>

class PrimitiveShape;

enum class ShapeType
{
    None,
    Sphere,
    Cylinder,
    Plane,
    Cube,
    Tetrahedron
};

enum class TextureMappingMode
{
    Planar = 0,
    Cylindrical = 1,
    Spherical = 2,
};

enum class TextureStyle
{
    Checker = 0,
    Stripes = 1,
    ImagePlaceholder = 2,
};

struct Material
{
    DirectX::XMFLOAT3 BaseColor = DirectX::XMFLOAT3(0.7f, 0.7f, 0.7f);
    float SpecularStrength = 0.5f;
    float Shininess = 32.0f;
};

// 场景中的一个可交互对象
class SceneObject
{
public:
    SceneObject(ShapeType type, std::shared_ptr<PrimitiveShape> shape);
    ~SceneObject();

    // 设置/获取位置
    void SetPosition(const DirectX::XMFLOAT3& pos);
    DirectX::XMFLOAT3 GetPosition() const { return m_position; }

    // 设置/获取缩放
    void SetScale(float scale);
    float GetScale() const { return m_scale; }

    // 设置/获取旋转（内部弧度）
    void SetRotation(const DirectX::XMFLOAT3& rotation);
    DirectX::XMFLOAT3 GetRotation() const { return m_rotation; }

    // 材质
    const Material& GetMaterial() const { return m_material; }
    void SetMaterial(const Material& material) { m_material = material; }

    // 纹理（A 方案：保存路径 + 风格 + 映射方式）
    void SetTexturePath(const std::wstring& path) { m_texturePath = path; }
    const std::wstring& GetTexturePath() const { return m_texturePath; }

    void SetTextureMappingMode(TextureMappingMode mode) { m_textureMappingMode = mode; }
    TextureMappingMode GetTextureMappingMode() const { return m_textureMappingMode; }

    void SetTextureStyle(TextureStyle style) { m_textureStyle = style; }
    TextureStyle GetTextureStyle() const { return m_textureStyle; }

    // 选中状态
    void SetSelected(bool selected) { m_isSelected = selected; }
    bool IsSelected() const { return m_isSelected; }

    // 获取形状类型
    ShapeType GetType() const { return m_type; }

    // 获取几何体
    PrimitiveShape* GetShape() const { return m_shape.get(); }

    // 获取世界矩阵
    DirectX::XMMATRIX GetWorldMatrix() const;

    // 边界球检测（用于鼠标拾取）
    float GetBoundingRadius() const;
    bool IntersectRay(const DirectX::XMVECTOR& rayOrigin,
        const DirectX::XMVECTOR& rayDir,
        float& distance) const;

private:
    ShapeType m_type;
    std::shared_ptr<PrimitiveShape> m_shape;

    DirectX::XMFLOAT3 m_position;
    DirectX::XMFLOAT3 m_rotation;
    float m_scale;
    bool m_isSelected;

    Material m_material{};

    std::wstring m_texturePath;
    TextureMappingMode m_textureMappingMode = TextureMappingMode::Planar;
    TextureStyle m_textureStyle = TextureStyle::Checker;
};