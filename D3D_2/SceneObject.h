#pragma once

#include <DirectXMath.h>
#include <memory>

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

    // 设置/获取旋转
    void SetRotation(const DirectX::XMFLOAT3& rotation);
    DirectX::XMFLOAT3 GetRotation() const { return m_rotation; }

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
};