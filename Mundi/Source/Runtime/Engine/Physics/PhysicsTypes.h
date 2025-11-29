#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// PhysicsTypes.h
// Mundi 엔진과 PhysX 간의 좌표계 변환 유틸리티
// ─────────────────────────────────────────────────────────────────────────────
//
// 좌표계 차이:
// ┌─────────────┬─────────────────────────┬─────────────────────────┐
// │    항목     │      Mundi 엔진         │        PhysX            │
// ├─────────────┼─────────────────────────┼─────────────────────────┤
// │   Up 축     │         +Z              │         +Y              │
// │  Forward 축 │         +X              │         -Z              │
// │  Right 축   │         +Y              │         +X              │
// │ Handedness  │     Left-Handed         │     Right-Handed        │
// └─────────────┴─────────────────────────┴─────────────────────────┘
//
// 변환 규칙:
// - 위치: Mundi(X,Y,Z) → PhysX(Y, Z, -X)
// - 회전: 축 재배치 + Handedness 반전 (허수부 부호 반전)
// ─────────────────────────────────────────────────────────────────────────────

#include "Vector.h"
#include <PxPhysicsAPI.h>

namespace PhysicsConversion
{
    // ═══════════════════════════════════════════════════════════════════════════
    // 위치 변환 (FVector ↔ PxVec3)
    // 축 재배치: Mundi(X,Y,Z) → PhysX(Y, Z, -X)
    // ═══════════════════════════════════════════════════════════════════════════

    /**
     * @brief FVector를 PxVec3로 변환
     * @param V Mundi 좌표계의 벡터 (X=Forward, Y=Right, Z=Up)
     * @return PhysX 좌표계의 벡터 (X=Right, Y=Up, Z=Back)
     *
     * @note PhysX는 Right-Handed이므로 Forward = -Z
     */
    inline physx::PxVec3 ToPxVec3(const FVector& V)
    {
        // Mundi: X=Forward, Y=Right, Z=Up
        // PhysX: X=Right,   Y=Up,    Z=Back (Forward = -Z)
        // 변환: (Mundi.Y, Mundi.Z, -Mundi.X)
        return physx::PxVec3(V.Y, V.Z, -V.X);
    }

    /**
     * @brief PxVec3를 FVector로 변환
     * @param V PhysX 좌표계의 벡터 (X=Right, Y=Up, Z=Back)
     * @return Mundi 좌표계의 벡터 (X=Forward, Y=Right, Z=Up)
     *
     * @note PhysX는 Right-Handed이므로 Forward = -Z
     */
    inline FVector ToFVector(const physx::PxVec3& V)
    {
        // PhysX: X=Right, Y=Up, Z=Back
        // Mundi: X=Forward, Y=Right, Z=Up
        // 변환: (-PhysX.z, PhysX.x, PhysX.y)
        return FVector(-V.z, V.x, V.y);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // 회전 변환 (FQuat ↔ PxQuat)
    // 축 재배치 + Handedness 반전 (허수부 부호 반전)
    // ═══════════════════════════════════════════════════════════════════════════

    /**
     * @brief FQuat를 PxQuat로 변환
     * @param Q Mundi 좌표계의 쿼터니언
     * @return PhysX 좌표계의 쿼터니언
     *
     * @note 변환 과정:
     *       1. 축 재배치: (Qx,Qy,Qz) → (Qy, Qz, -Qx)
     *       2. Handedness 반전: 허수부 부호 반전
     *       최종: (-Qy, -Qz, Qx, Qw)
     */
    inline physx::PxQuat ToPxQuat(const FQuat& Q)
    {
        // 축 재배치: (Qx,Qy,Qz) → (Qy,Qz,-Qx)
        // Handedness 반전: 허수부 부호 반전
        // 최종: (-Qy, -Qz, Qx, Qw)
        return physx::PxQuat(-Q.Y, -Q.Z, Q.X, Q.W);
    }

    /**
     * @brief PxQuat를 FQuat로 변환
     * @param Q PhysX 좌표계의 쿼터니언
     * @return Mundi 좌표계의 쿼터니언
     *
     * @note 변환 과정:
     *       1. Handedness 반전: 허수부 부호 반전
     *       2. 축 재배치: (x,y,z) → (-z,x,y)
     *       최종: (Qz, -Qx, -Qy, Qw)
     */
    inline FQuat ToFQuat(const physx::PxQuat& Q)
    {
        // 역변환: 축 재배치 + Handedness 반전
        // PhysX(x,y,z,w) → Mundi(z, -x, -y, w)
        return FQuat(Q.z, -Q.x, -Q.y, Q.w);
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // Transform 변환 (FTransform ↔ PxTransform)
    // ═══════════════════════════════════════════════════════════════════════════

    /**
     * @brief FTransform를 PxTransform로 변환
     * @param T Mundi 좌표계의 Transform
     * @return PhysX 좌표계의 Transform
     *
     * @note PhysX는 스케일을 지원하지 않으므로 위치와 회전만 변환됨
     */
    inline physx::PxTransform ToPxTransform(const FTransform& T)
    {
        return physx::PxTransform(
            ToPxVec3(T.Translation),
            ToPxQuat(T.Rotation)
        );
    }

    /**
     * @brief PxTransform를 FTransform로 변환
     * @param T PhysX 좌표계의 Transform
     * @return Mundi 좌표계의 Transform (스케일은 1,1,1로 설정)
     */
    inline FTransform ToFTransform(const physx::PxTransform& T)
    {
        return FTransform(
            ToFVector(T.p),
            ToFQuat(T.q),
            FVector::One()  // PhysX는 스케일 미지원, 기본값 1
        );
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // 행렬 변환 (PxMat44 → FMatrix)
    // PhysX Column-major → Mundi Row-major + 좌표계 변환
    // ═══════════════════════════════════════════════════════════════════════════

    /**
     * @brief PxMat44를 FMatrix로 변환
     * @param M PhysX의 4x4 행렬 (Column-major)
     * @return Mundi의 4x4 행렬 (Row-major)
     *
     * @note 축 재배치 행렬 (PhysX → Mundi):
     *       | 0  0 -1 0 |   (PhysX -Z → Mundi X, Forward)
     *       | 1  0  0 0 |   (PhysX X → Mundi Y, Right)
     *       | 0  1  0 0 |   (PhysX Y → Mundi Z, Up)
     *       | 0  0  0 1 |
     */
    inline FMatrix ToFMatrix(const physx::PxMat44& M)
    {
        FMatrix Result;

        // Row 0 (Mundi X축 = PhysX -Z축)
        Result.M[0][0] = -M.column2.z;
        Result.M[0][1] = -M.column2.x;
        Result.M[0][2] = -M.column2.y;
        Result.M[0][3] = 0;

        // Row 1 (Mundi Y축 = PhysX X축)
        Result.M[1][0] = M.column0.z;
        Result.M[1][1] = M.column0.x;
        Result.M[1][2] = M.column0.y;
        Result.M[1][3] = 0;

        // Row 2 (Mundi Z축 = PhysX Y축)
        Result.M[2][0] = M.column1.z;
        Result.M[2][1] = M.column1.x;
        Result.M[2][2] = M.column1.y;
        Result.M[2][3] = 0;

        // Row 3 (Translation: -Z로 인한 부호 변경)
        Result.M[3][0] = -M.column3.z;
        Result.M[3][1] = M.column3.x;
        Result.M[3][2] = M.column3.y;
        Result.M[3][3] = 1;

        return Result;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // 스칼라 변환 (단위계가 동일하므로 변환 불필요)
    // ═══════════════════════════════════════════════════════════════════════════

    inline float ToPhysX(float Value) { return Value; }
    inline float ToMundi(float Value) { return Value; }

} // namespace PhysicsConversion
