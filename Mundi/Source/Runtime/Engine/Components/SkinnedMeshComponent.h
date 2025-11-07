#pragma once
#include "MeshComponent.h"

class USkinnedMeshComponent : public UMeshComponent
{
public:
    DECLARE_CLASS(USkinnedMeshComponent, UMeshComponent)

    USkinnedMeshComponent() = default;
    ~USkinnedMeshComponent() override = default;

    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
    
// Mesh Component Section
public:
    void CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View) override;
    // virtual void UpdateDynamicVertexBuffer();

// Skeletal Section
public:
    /**
     * @brief 렌더링할 스켈레탈 메시 에셋 설정
     * @param InSkeletalMesh 새 스켈레탈 메시 에셋
     */
    // virtual void SetSkeletalMesh(USkeletalMesh* InSkeletalMesh);

    /**
     * @brief 계산이 완료된 최종 정점을 반환 (정점 버퍼의 데이터로 사용)
     */
    // const TArray<FNormalVertex>& GetDynamicNormalVertices() const { return SkinnedVertices; }

    /**
     * @brief 원본 T-Pose 정점 버퍼를 반환합니다.
     * 렌더러는 이 버퍼를 UV, Normal, Tangent 등 정적 데이터 소스로 사용합니다.
     */
    // const TArray<FSkinnedVertex>& GetReferenceVertices() const;

    /**
     * @brief 이 컴포넌트의 USkeletalMesh 에셋을 반환
     */
    // USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }

protected:
    // USkeletalMesh* SkeletalMesh;

    /**
    * CPU 스키닝 최종 결과물
    */
    // TArray<FNormalVertex> SkinnedVertices;
};
