#include "SceneObject.h"
#include "PrimitiveShape.h"
using namespace DirectX;

SceneObject::SceneObject(ShapeType type, std::shared_ptr<PrimitiveShape> shape)
    : m_type(type)
    , m_shape(shape)
    , m_position(0.0f, 0.0f, 0.0f)
    , m_rotation(0.0f, 0.0f, 0.0f)
    , m_scale(1.0f)
    , m_isSelected(false)
{
}

SceneObject::~SceneObject()
{
}

void SceneObject::SetPosition(const XMFLOAT3& pos)
{
    m_position = pos;
}

void SceneObject::SetScale(float scale)
{
    m_scale = scale;
}

void SceneObject::SetRotation(const XMFLOAT3& rotation)
{
    m_rotation = rotation;
}

XMMATRIX SceneObject::GetWorldMatrix() const
{
    XMMATRIX scale = XMMatrixScaling(m_scale, m_scale, m_scale);
    XMMATRIX rotX = XMMatrixRotationX(m_rotation.x);
    XMMATRIX rotY = XMMatrixRotationY(m_rotation.y);
    XMMATRIX rotZ = XMMatrixRotationZ(m_rotation.z);
    XMMATRIX rotation = rotX * rotY * rotZ;
    XMMATRIX translation = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);

    return scale * rotation * translation;
}

float SceneObject::GetBoundingRadius() const
{
    // 返回近似边界球半径
    float baseRadius = 1.5f; // 基础半径

    switch (m_type)
    {
    case ShapeType::Sphere:
        baseRadius = 1.0f;
        break;
    case ShapeType::Cylinder:
        baseRadius = 1.2f;
        break;
    case ShapeType::Cube:
        baseRadius = 1.732f; // sqrt(3)
        break;
    case ShapeType::Tetrahedron:
        baseRadius = 1.5f;
        break;
    case ShapeType::Plane:
        baseRadius = 1.414f; // sqrt(2)
        break;
    }

    return baseRadius * m_scale;
}

bool SceneObject::IntersectRay(const XMVECTOR& rayOrigin, const XMVECTOR& rayDir, float& distance) const
{
    // 简单的射线-球体相交测试
    XMVECTOR objPos = XMLoadFloat3(&m_position);
    XMVECTOR toObject = objPos - rayOrigin;

    float radius = GetBoundingRadius();

    // 投影距离
    float projDist = XMVectorGetX(XMVector3Dot(toObject, rayDir));

    // 如果投影为负，物体在射线后方
    if (projDist < 0)
        return false;

    // 最近点到物体中心的距离
    XMVECTOR closestPoint = rayOrigin + rayDir * projDist;
    float distToCenter = XMVectorGetX(XMVector3Length(closestPoint - objPos));

    if (distToCenter <= radius)
    {
        // 计算交点距离
        float offset = sqrtf(radius * radius - distToCenter * distToCenter);
        distance = projDist - offset;
        return true;
    }

    return false;
}