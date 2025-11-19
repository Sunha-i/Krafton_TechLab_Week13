#include "pch.h"
#include "DancingCharacter.h"
#include "SkeletalMeshComponent.h"
#include "AudioComponent.h"
#include "FAudioDevice.h"

ADancingCharacter::ADancingCharacter()
{
	if (MeshComponent)
	{
		MeshComponent->SetSkeletalMesh(GDataDir + "/SillyDancing.fbx");
	}

	Sound = UResourceManager::GetInstance().Load<USound>(GDataDir + "/Audio/Die.wav");
}

ADancingCharacter::~ADancingCharacter()
{
}

void ADancingCharacter::BeginPlay()
{
	Super::BeginPlay();

	UAnimSequence* AnimToPlay = UResourceManager::GetInstance().Get<UAnimSequence>(GDataDir + "/SillyDancing_mixamo.com");

	if (AnimToPlay && GetMesh())
	{
		MeshComponent->PlayAnimation(AnimToPlay, true);
		UE_LOG("ADancingCharacter: Started playing animation.");
	}
	else
	{
		UE_LOG("ADancingCharacter: Failedd to find animation to play");
	}
}

void ADancingCharacter::HandleAnimNotify(const FAnimNotifyEvent& NotifyEvent)
{
	Super::HandleAnimNotify(NotifyEvent);

	UE_LOG("ADancingCharacter: Notify Triggered!");
	if (Sound)
	{
		FAudioDevice::PlaySound3D(Sound, GetActorLocation());
	}
}