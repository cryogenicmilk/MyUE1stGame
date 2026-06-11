// Fill out your copyright notice in the Description page of Project Settings.


#include "PointJumpTargetComponent.h"

// Sets default values for this component's properties
UPointJumpTargetComponent::UPointJumpTargetComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// エディタ上で位置を調整しやすいように SceneComponent にしている。
	// ActorComponent だと Transform を持てないため、ポイント位置を置きづらい。
	SetHiddenInGame(true);
}

FVector UPointJumpTargetComponent::GetPointJumpLocation() const
{
	return GetComponentLocation();
}

bool UPointJumpTargetComponent::CanPointJump() const
{
	return bCanPointJump;
}

