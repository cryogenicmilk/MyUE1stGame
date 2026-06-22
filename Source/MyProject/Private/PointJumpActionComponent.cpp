// Fill out your copyright notice in the Description page of Project Settings.


#include "PointJumpActionComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "PointJumpTargetComponent.h"
#include "UObject/UObjectIterator.h"

// Sets default values
UPointJumpActionComponent::UPointJumpActionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void UPointJumpActionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
}

// Called every frame
void UPointJumpActionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CustomUpdatePointJump(DeltaTime);
}

void UPointJumpActionComponent::CustomUpdatePointJump(float DeltaTime)
{
	if (!OwnerCharacter) return;

	switch (PointJumpState)
	{
	case EPointJumpState::None:
		return;

	case EPointJumpState::Startup:
		CustomUpdateStartup(DeltaTime);
		break;

	case EPointJumpState::Pulling:
		CustomUpdatePulling(DeltaTime);
		break;

	case EPointJumpState::LandingSnap:
		CustomUpdateLandingSnap(DeltaTime);
		break;

	case EPointJumpState::Landing:
		CustomUpdateLanding(DeltaTime);
		break;

	case EPointJumpState::Launch:
		CustomUpdateLaunch();
		break;

	case EPointJumpState::AfterLaunch:
		break;
	}
}

// アニメーション
void UPointJumpActionComponent::CustomUpdateStartup(float deltatime)
{
	PointJumpStateTimer -= deltatime;

	if (PointJumpStateTimer > 0.0f) return;

	// ポイント位置が指定した距離より小さかったら
	const float DistanceToTarget = FVector::Dist(OwnerCharacter->GetActorLocation(), PointJumpTargetLocation);
	if (DistanceToTarget <= PullingSkipDistance)
	{
		CustomEnterLandingSnap();
		return;
	}
	// else
	PointJumpState = EPointJumpState::Pulling;
}

void UPointJumpActionComponent::CustomUpdatePulling(float deltatime)
{
	if (CustomGetPointJumpRemainingTime() <= PointJumpPerfectWindow)
	{
		bIsPointJumpingEnableJump = true;
	}

	const FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	const FVector NextLocation = FMath::VInterpConstantTo(
		CurrentLocation, PointJumpTargetLocation, deltatime,
		PullSpeed
	);

	FHitResult SweepHit; // 壁に引っかかるなど置いた位置に行かないことがあるから、移動後実際の位置を使う
	OwnerCharacter->SetActorLocation(NextLocation, true, &SweepHit);

	if (FVector::DistSquared(NextLocation, PointJumpTargetLocation)
		<= FMath::Square(CustomGetCurrentPointJumpArriveDistance()))
	{
		CustomEnterLandingSnap();
		return;
	}
}

// 一回だけ enter LandingSnap 今は薄いけど、状態遷移の入口として残す価値はある
void UPointJumpActionComponent::CustomEnterLandingSnap()
{
	PointJumpState = EPointJumpState::LandingSnap;

	UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement();
	if (MovementComp)
	{
		MovementComp->StopMovementImmediately();
	}
}

void UPointJumpActionComponent::CustomUpdateLandingSnap(float deltatime)
{
	const FVector CurrentLocation = OwnerCharacter->GetActorLocation();

	const FVector NextLocation = FMath::VInterpConstantTo(
		CurrentLocation, PointJumpTargetLocation, deltatime,
		LandingSnapSpeed
	);

	OwnerCharacter->SetActorLocation(NextLocation, false);

	const bool bArrivedLandingPoint =
		FVector::DistSquared(OwnerCharacter->GetActorLocation(), PointJumpTargetLocation)
		<= FMath::Square(LandingSnapCompleteDistance);

	if (!bArrivedLandingPoint) return;

	OwnerCharacter->SetActorLocation(PointJumpTargetLocation, false);
	CustomEnterLanding();
}

// Landing
void UPointJumpActionComponent::CustomEnterLanding()
{
	PointJumpState = EPointJumpState::Landing;
	
	UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement();
	if (MovementComp)
	{
		MovementComp->StopMovementImmediately();
		MovementComp->SetMovementMode(MOVE_Walking);
	}
}

void UPointJumpActionComponent::CustomUpdateLanding(float DeltaTime)
{
	if (!bIsBufferedPointJump) return;

	CustomEnterLaunch();
}

// landing後launch可能にする
void UPointJumpActionComponent::CustomEnterLaunch()
{
	PointJumpState = EPointJumpState::Launch;
	bHasExecutedPointJumpLaunch = false;
}

void UPointJumpActionComponent::CustomUpdateLaunch()
{
	if (bHasExecutedPointJumpLaunch) return;
	bHasExecutedPointJumpLaunch = true;

	CustomFinishPointJump(bIsBufferedPointJump ? BufferedPointJumpResult : EPointJumpResult::Normal);
}

// 位置変える方針
void UPointJumpActionComponent::CustomOnPointJumpLandingAnimationFinished()
{
	if (PointJumpState != EPointJumpState::Landing) return;

	CustomResetPointJump();

	if (UCharacterMovementComponent* MovementComp = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr)
	{
		MovementComp->SetMovementMode(MOVE_Walking);
	}
}

void UPointJumpActionComponent::CustomOnPointJumpLaunchAnimationFinished()
{
	if (PointJumpState != EPointJumpState::Launch) return;

	PointJumpState = EPointJumpState::AfterLaunch;
}

void UPointJumpActionComponent::CustomResetPointJump()
{
	PointJumpState = EPointJumpState::None;
	PointJumpStateTimer = 0.0f;
	PointJumpTargetLocation = FVector::ZeroVector;

	CurrentPointJumpTarget = nullptr;

	bIsBufferedPointJump = false;
	BufferedPointJumpResult = EPointJumpResult::Normal;
	bHasExecutedPointJumpLaunch = false;

	bHasPointJumpMoveInput = false;
	PointJumpBufferedMoveDirection = FVector::ZeroVector;
}


/// =================================================================
/// 入力実行関数
/// =================================================================


/// =================================================================
/// タイミング判定
/// =================================================================
// まず距離でタイミング
float UPointJumpActionComponent::CustomGetPointJumpRemainingTime() const
{
	if (PullSpeed <= 0.0f) return 0.0f;

	const float RemainingDistance = FVector::Dist(OwnerCharacter->GetActorLocation(), PointJumpTargetLocation);
	return RemainingDistance / PullSpeed;
}
// 
void UPointJumpActionComponent::CustomBufferPointJumpInput()
{
	const EPointJumpResult Result = CustomJudgePointJumpInputTiming();

	switch (Result) {
	case EPointJumpResult::Perfect:UE_LOG(LogTemp, Warning, TEXT("PERFECT")); break;
	case EPointJumpResult::Normal: UE_LOG(LogTemp, Warning, TEXT("NORMAL" )); break;
	}

	bIsBufferedPointJump = true;
	BufferedPointJumpResult = Result;
	return;
}

EPointJumpResult UPointJumpActionComponent::CustomJudgePointJumpInputTiming() const
{
	const float Timing = CustomGetPointJumpRemainingTime();

	if (Timing <= PointJumpPerfectWindow)
	{
		return EPointJumpResult::Perfect;
	}
	// else
	return EPointJumpResult::Normal;
}

float UPointJumpActionComponent::CustomGetPointJumpForwardPower(EPointJumpResult Result) const
{
	switch (Result)
	{
	case EPointJumpResult::Perfect: return PointJumpPerfectForwardPower;
	case EPointJumpResult::Normal:  return PointJumpNormalForwardPower;
	default:                        return PointJumpNormalForwardPower;
	}
}
float UPointJumpActionComponent::CustomGetPointJumpUpPower(EPointJumpResult Result) const
{
	switch (Result)
	{
	case EPointJumpResult::Perfect: return PointJumpPerfectUpPower;
	case EPointJumpResult::Normal:  return PointJumpNormalUpPower;
	default:                        return PointJumpNormalUpPower;
	}
}

FVector UPointJumpActionComponent::CustomGetPointJumpLaunchDirection() const
{
	if (!PointJumpBufferedMoveDirection.IsNearlyZero())
	{
		return PointJumpBufferedMoveDirection.GetSafeNormal2D();
	}

	if (!OwnerCharacter) return FVector::ForwardVector;

	const FVector LastMovementInput = OwnerCharacter->GetLastMovementInputVector();
	if (!LastMovementInput.IsNearlyZero())
	{
		return LastMovementInput.GetSafeNormal2D();
	}

	const AController* Controller = OwnerCharacter->GetController();
	if (!Controller) return OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	return FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X).GetSafeNormal2D();
}
/// =================================================================
/// サーチターゲット
/// =================================================================

bool AMyCharacter::CustomTryFindPointJumpTarget(UPointJumpTargetComponent*& OutTargetComponent) const
{
	OutTargetComponent = nullptr;

	if (!FollowCamera) return false;
	if (!GetWorld()) return false;

	const FVector CameraLocation = FollowCamera->GetComponentLocation();
	const FVector CameraForward = FollowCamera->GetForwardVector();

	UPointJumpTargetComponent* BestTarget = nullptr;
	float BestViewDot = PointJumpMinViewDot;

	for (TObjectIterator<UPointJumpTargetComponent> It; It; ++It)
	{
		UPointJumpTargetComponent* TargetComponent = *It;
		if (!TargetComponent) continue;
		if (TargetComponent->GetWorld() != GetWorld()) continue;
		if (!CustomIsValidPointJumpTarget(TargetComponent, CameraLocation, CameraForward)) continue;

		const FVector TargetLocation = TargetComponent->CustomGetPointJumpLocation();
		const FVector ToTarget = (TargetLocation - CameraLocation).GetSafeNormal();
		const float ViewDot = FVector::DotProduct(CameraForward, ToTarget);

		if (ViewDot > BestViewDot)
		{
			BestViewDot = ViewDot;
			BestTarget = TargetComponent;
		}
	}

	OutTargetComponent = BestTarget;
	return OutTargetComponent != nullptr;
}

bool AMyCharacter::CustomIsValidPointJumpTarget(const UPointJumpTargetComponent* TargetComponent, const FVector& CameraLocation, const FVector& CameraForward) const
{
	if (!TargetComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("TargetComponent is null"));
		return false;
	}

	if (!TargetComponent->CustomCanPointJump())
	{
		UE_LOG(LogTemp, Warning, TEXT("Target disabled"));
		return false;
	}

	const AActor* OwnerActor = TargetComponent->GetOwner();
	if (!OwnerActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("Owner is null"));
		return false;
	}

	if (OwnerActor == this)
	{
		UE_LOG(LogTemp, Warning, TEXT("Owner is player"));
		return false;
	}

	const FVector TargetLocation = TargetComponent->CustomGetPointJumpLocation();

	const float Distance = FVector::Dist(GetActorLocation(), TargetLocation);
	if (Distance > PointJumpSearchRadius)
	{
		UE_LOG(LogTemp, Warning, TEXT("Too far: %f"), Distance);
		return false;
	}

	const FVector ToTarget = (TargetLocation - CameraLocation).GetSafeNormal();
	const float ViewDot = FVector::DotProduct(CameraForward, ToTarget);
	if (ViewDot < PointJumpMinViewDot)
	{
		UE_LOG(LogTemp, Warning, TEXT("Out of view: %f"), ViewDot);
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Valid Target: %s"), *OwnerActor->GetName());
	return true;
}
//開始
void AMyCharacter::CustomBeginPointJump(UPointJumpTargetComponent* TargetComponent)
{
	if (!TargetComponent) return;

	PointJumpState = EPointJumpState::Start;
	PointJumpStateTimer = PointJumpStartTime;

	bIsPointJumpingEnableJump = false;
	bIsBufferedPointJump = false;
	BufferedPointJumpResult = EPointJumpResult::Normal;

	bHasPointJumpMoveInput = false;
	PointJumpBufferedMoveDirection = FVector::ZeroVector;

	CurrentPointJumpTarget = TargetComponent;

	const FVector TargetLocation = TargetComponent->CustomGetPointJumpLocation();
	const float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	PointJumpTargetLocation = TargetLocation + FVector::UpVector * CapsuleHalfHeight;

	CustomStopDash();
	CustomSetPointJumpMovementEnabled(false);
}

void AMyCharacter::CustomFinishPointJump(EPointJumpResult Result)
{
	if (PointJumpState == EPointJumpState::None) return;

	CurrentPointJumpTarget = nullptr;
	CustomSetPointJumpMovementEnabled(true);

	const FVector LaunchVelocity = CustomGetPointJumpLaunchDirection() * CustomGetPointJumpForwardPower(Result)
		+ FVector::UpVector * CustomGetPointJumpUpPower(Result);

	LaunchCharacter(LaunchVelocity, true, true);
}

float AMyCharacter::CustomGetCurrentPointJumpArriveDistance() const
{
	if (CurrentPointJumpTarget && CurrentPointJumpTarget->ArriveDistanceOverride > 0.0f)
	{
		return CurrentPointJumpTarget->ArriveDistanceOverride;
	}

	return PointJumpArriveDistance;
}

void AMyCharacter::CustomSetPointJumpMovementEnabled(bool bEnabled)
{
	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	if (!MovementComp) return;

	if (bEnabled)
	{
		MovementComp->SetMovementMode(MOVE_Falling);
		MovementComp->Velocity = FVector::ZeroVector;
		return;
	}

	MovementComp->StopMovementImmediately();
	MovementComp->SetMovementMode(MOVE_Flying);
}