// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PointJumpTargetComponent.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UPointJumpTargetComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UPointJumpTargetComponent();

	// このポイントをポイントジャンプ対象にするか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Target")
	bool bCanPointJump = true;

	// ポイントごとに到達判定距離を変えたい場合だけ使う。
	// 0以下なら MyCharacter 側の PointJumpArriveDistance を使う。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Target", meta = (ClampMin = "0.0"))
	float ArriveDistanceOverride = 0.0f;

	FVector CustomGetPointJumpLocation() const;
	bool CustomCanPointJump() const;
};