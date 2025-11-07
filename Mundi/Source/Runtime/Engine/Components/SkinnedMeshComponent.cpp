#include "pch.h"
#include "SkinnedMeshComponent.h"
#include "MeshBatchElement.h"
#include "SceneView.h"

IMPLEMENT_CLASS(USkinnedMeshComponent, UMeshComponent)

void USkinnedMeshComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);
   // SkeletalMesh 에셋 직렬화
}

void USkinnedMeshComponent::CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View)
{
   //  if (!SkeletalMesh || !GetDynamicVertexBuffer() || !SkeletalMesh->GetIndexBuffer())
   //  {
   //     return;
   //  }
   //
   //  const TArray<FGroupInfo>& MeshGroupInfos = SkeletalMesh->GetGroupInfos(); // (USkeletalMesh에 이 함수가 있다고 가정)
   // auto DetermineMaterialAndShader = [&](uint32 SectionIndex) -> TPair<UMaterialInterface*, UShader*>
   //     {
   //        UMaterialInterface* Material = GetMaterial(SectionIndex);
   //        UShader* Shader = nullptr;
   //
   //        if (Material && Material->GetShader())
   //        {
   //           Shader = Material->GetShader();
   //        }
   //        else
   //        {
   //           UE_LOG("USkinnedMeshComponent: 머티리얼/셰이더가 없어 기본값 사용 (Section %u)", SectionIndex);
   //           Material = UResourceManager::GetInstance().GetDefaultMaterial();
   //           if (Material) Shader = Material->GetShader();
   //           if (!Material || !Shader)
   //           {
   //              UE_LOG("USkinnedMeshComponent: 기본 머티리얼 없음.");
   //              return { nullptr, nullptr };
   //           }
   //        }
   //        return { Material, Shader };
   //     };
   //
   //  const bool bHasSections = !MeshGroupInfos.IsEmpty();
   //  const uint32 NumSectionsToProcess = bHasSections ? static_cast<uint32>(MeshGroupInfos.size()) : 1;
   //
   //  for (uint32 SectionIndex = 0; SectionIndex < NumSectionsToProcess; ++SectionIndex)
   //  {
   //     uint32 IndexCount = 0;
   //     uint32 StartIndex = 0;
   //
   //     if (bHasSections)
   //     {
   //        const FGroupInfo& Group = MeshGroupInfos[SectionIndex];
   //        IndexCount = Group.IndexCount;
   //        StartIndex = Group.StartIndex;
   //     }
   //     else
   //     {
   //        IndexCount = SkeletalMesh->GetIndexCount();
   //        StartIndex = 0;
   //     }
   //
   //     if (IndexCount == 0)
   //     {
   //        continue;
   //     }
   //
   //     auto [MaterialToUse, ShaderToUse] = DetermineMaterialAndShader(SectionIndex);
   //     if (!MaterialToUse || !ShaderToUse)
   //     {
   //        continue;
   //     }
   //
   //     // 셰이더 변수(Variant) 가져오기 (StaticMesh와 동일)
   //     FMeshBatchElement BatchElement;
   //     TArray<FShaderMacro> ShaderMacros = View->ViewShaderMacros;
   //     if (0 < MaterialToUse->GetShaderMacros().Num())
   //     {
   //        ShaderMacros.Append(MaterialToUse->GetShaderMacros());
   //     }
   //     FShaderVariant* ShaderVariant = ShaderToUse->GetOrCompileShaderVariant(ShaderMacros);
   //
   //     if (ShaderVariant)
   //     {
   //        BatchElement.VertexShader = ShaderVariant->VertexShader;
   //        BatchElement.PixelShader = ShaderVariant->PixelShader;
   //        BatchElement.InputLayout = ShaderVariant->InputLayout;
   //        // [중요] CPU 스키닝이므로, 셰이더는 스키닝을 할 필요가 없습니다.
   //        // 즉, StaticMesh와 동일한 셰이더/InputLayout을 사용할 수 있습니다.
   //        // (InputLayout이 FSkinnedVertex 구조체와 일치해야 함)
   //     }
   //
   //     // --- [핵심 차이점] BatchElement 채우기 ---
   //     
   //     BatchElement.Material = MaterialToUse;
   //
   //     // [차이점 2] 정점 버퍼 (VB)
   //     // StaticMesh는 에셋(StaticMesh->GetVertexBuffer())에서 가져오지만,
   //     // SkinnedMesh는 컴포넌트(this->GetDynamicVertexBuffer())에서 가져옵니다.
   //     BatchElement.VertexBuffer = GetDynamicVertexBuffer();
   //
   //     // [차이점 3] 인덱스 버퍼 (IB)
   //     // 인덱스 버퍼는 정적이므로 에셋(SkeletalMesh->GetIndexBuffer())에서 가져옵니다.
   //     BatchElement.IndexBuffer = SkeletalMesh->GetIndexBuffer();
   //
   //     // [차이점 4] 정점 스트라이드 (Stride)
   //     // CPU 스키닝이므로, VB는 FSkinnedVertex의 풀 구조체를 가집니다.
   //     // (FSkinnedVertex에 Pos, UV, Normal 등이 모두 포함)
   //     BatchElement.VertexStride = sizeof(FSkinnedVertex);
   //
   //     BatchElement.IndexCount = IndexCount;
   //     BatchElement.StartIndex = StartIndex;
   //     BatchElement.BaseVertexIndex = 0;
   //     BatchElement.WorldMatrix = GetWorldMatrix();
   //     BatchElement.ObjectID = InternalIndex;
   //     BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
   //
   //     OutMeshBatchElements.Add(BatchElement);
   //  }
}
