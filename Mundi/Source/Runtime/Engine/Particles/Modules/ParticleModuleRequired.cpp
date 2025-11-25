#include "pch.h"
#include "ParticleModuleRequired.h"
#include "ParticleDefinitions.h"
#include "JsonSerializer.h"
#include "ResourceManager.h"
#include "Material.h"

// 언리얼 엔진 호환: 렌더 스레드용 데이터로 변환
FParticleRequiredModule UParticleModuleRequired::ToRenderThreadData() const
{
	FParticleRequiredModule RenderData;
	RenderData.Material = Material;
	RenderData.EmitterName = EmitterName;
	RenderData.ScreenAlignment = ScreenAlignment;
	RenderData.bOrientZAxisTowardCamera = bOrientZAxisTowardCamera;
	return RenderData;
}

void UParticleModuleRequired::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);
	// UPROPERTY 속성은 자동으로 직렬화됨 (Material 제외 - 수동 처리)

	if (bInIsLoading)
	{
		// === Material 로드 (Shader + Texture 경로 기반) ===
		FString ShaderPath;
		FJsonSerializer::ReadString(InOutHandle, "MaterialShader", ShaderPath);

		if (!ShaderPath.empty())
		{
			// Shader 경로로 Material 로드/생성
			Material = UResourceManager::GetInstance().Load<UMaterial>(ShaderPath);

			// 텍스처 경로 읽기 및 설정
			FString TexturePath;
			FJsonSerializer::ReadString(InOutHandle, "MaterialTexture", TexturePath);

			if (Material && !TexturePath.empty())
			{
				UMaterial* MatPtr = Cast<UMaterial>(Material);
				if (MatPtr)
				{
					FMaterialInfo MatInfo;
					MatInfo.DiffuseTextureFileName = TexturePath;
					MatPtr->SetMaterialInfo(MatInfo);
					MatPtr->ResolveTextures();
				}
			}
		}
	}
	else
	{
		// === Material 저장 (Shader + Texture 경로 별도 저장) ===
		if (Material)
		{
			UMaterial* MatPtr = Cast<UMaterial>(Material);
			if (MatPtr)
			{
				// Shader 경로 저장
				UShader* Shader = MatPtr->GetShader();
				if (Shader)
				{
					InOutHandle["MaterialShader"] = Shader->GetFilePath().c_str();
				}
				else
				{
					InOutHandle["MaterialShader"] = "";
				}

				// Texture 경로 저장
				const FMaterialInfo& MatInfo = MatPtr->GetMaterialInfo();
				InOutHandle["MaterialTexture"] = MatInfo.DiffuseTextureFileName.c_str();
			}
			else
			{
				InOutHandle["MaterialShader"] = "";
				InOutHandle["MaterialTexture"] = "";
			}
		}
		else
		{
			InOutHandle["MaterialShader"] = "";
			InOutHandle["MaterialTexture"] = "";
		}
	}
}
