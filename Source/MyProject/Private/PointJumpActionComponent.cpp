#include "PointJumpActionComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "PointJumpTargetComponent.h"
#include "UObject/UObjectIterator.h"

UPointJumpActionComponent::UPointJumpActionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

/// =================================================================
/// ライフサイクル
/// =================================================================
void UPointJumpActionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Error,
			TEXT("PointJumpActionComponent must be attached to ACharacter."));
		SetComponentTickEnabled(false);
		return;
	}

	OwnerCamera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
	if (!OwnerCamera)
	{
		UE_LOG(LogTemp, Error,
			TEXT("PointJumpActionComponent could not find a UCameraComponent on %s."),
			*OwnerCharacter->GetName());
	}
}

void UPointJumpActionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	CustomUpdatePointJump(DeltaTime);
}

/// =================================================================
/// 外部入力
/// =================================================================
void UPointJumpActionComponent::CustomTryStartPointJump()
{
	if (!OwnerCharacter) return;

	// ポイントジャンプ中の再入力は、着地点で行うジャンプの先行入力として扱う。
	if (PointJumpState != EPointJumpState::None)
	{
		if (CustomCanBufferPointJumpInput())
		{
			CustomBufferPointJumpInput();
		}
		return;
	}

	UPointJumpTargetComponent* TargetComponent = nullptr;
	if (!CustomTryFindPointJumpTarget(TargetComponent)) return;

	CustomBeginPointJump(TargetComponent);
}

void UPointJumpActionComponent::CustomSetPointJumpMoveDirection(const FVector& MoveDirection)
{
	if (!CustomCanBufferMoveDirection()) return;
	if (MoveDirection.IsNearlyZero()) return;

	BufferedMoveDirection = MoveDirection.GetSafeNormal2D();
}

bool UPointJumpActionComponent::CustomCanBufferPointJumpInput() const
{
	return PointJumpState == EPointJumpState::Startup
		|| PointJumpState == EPointJumpState::Pulling
		|| PointJumpState == EPointJumpState::LandingSnap
		|| PointJumpState == EPointJumpState::Landing;
}

bool UPointJumpActionComponent::CustomCanBufferMoveDirection() const
{
	return CustomCanBufferPointJumpInput();
}

bool UPointJumpActionComponent::CustomIsPointJumpActive() const
{
	return PointJumpState != EPointJumpState::None;
}

/// =================================================================
/// ステートマシン更新
/// =================================================================
void UPointJumpActionComponent::CustomUpdatePointJump(float DeltaTime)
{
	if (!OwnerCharacter) return;

	switch (PointJumpState)
	{
	case EPointJumpState::None:
		break;

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
		CustomUpdateLanding();
		break;

	case EPointJumpState::Launch:
		CustomUpdateLaunch();
		break;

	case EPointJumpState::AfterLaunch:
		CustomUpdateAfterLaunch();
		break;

	default:
		break;
	}
}

void UPointJumpActionComponent::CustomUpdateStartup(float DeltaTime)
{
	PointJumpStateTimer -= DeltaTime;
	if (PointJumpStateTimer > 0.0f) return;

	const float DistanceToTarget = FVector::Dist(
		OwnerCharacter->GetActorLocation(),
		PointJumpTargetLocation
	);

	if (DistanceToTarget <= PullingSkipDistance)
	{
		CustomEnterLandingSnap();
		return;
	}

	CustomEnterPulling();
}

void UPointJumpActionComponent::CustomUpdatePulling(float DeltaTime)
{
	const FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	const FVector NextLocation = FMath::VInterpConstantTo(
		CurrentLocation,
		PointJumpTargetLocation,
		DeltaTime,
		PointJumpPullSpeed
	);

	FHitResult SweepHit;
	OwnerCharacter->SetActorLocation(NextLocation, true, &SweepHit);

	// SweepによってNextLocationまで進めない場合があるため、移動後の実位置で判定する。
	const float DistanceSquared = FVector::DistSquared(
		OwnerCharacter->GetActorLocation(),
		PointJumpTargetLocation
	);

	if (DistanceSquared <= FMath::Square(CustomGetLandingSnapStartDistance()))
	{
		CustomEnterLandingSnap();
	}
}

void UPointJumpActionComponent::CustomUpdateLandingSnap(float DeltaTime)
{
	const FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	const FVector NextLocation = FMath::VInterpConstantTo(
		CurrentLocation,
		PointJumpTargetLocation,
		DeltaTime,
		LandingSnapSpeed
	);

	// 最後の吸着はポイント位置を優先するためSweepなし。
	OwnerCharacter->SetActorLocation(NextLocation, false);

	const bool bHasReachedLandingPoint =
		FVector::DistSquared(OwnerCharacter->GetActorLocation(), PointJumpTargetLocation)
		<= FMath::Square(LandingSnapCompleteDistance);

	if (!bHasReachedLandingPoint) return;

	OwnerCharacter->SetActorLocation(PointJumpTargetLocation, false);
	CustomEnterLanding();
}

void UPointJumpActionComponent::CustomUpdateLanding()
{
	if (!bHasBufferedPointJumpInput) return;
	CustomEnterLaunch();
}

void UPointJumpActionComponent::CustomUpdateLaunch()
{
	if (bHasExecutedLaunch) return;
	bHasExecutedLaunch = true;

	const EPointJumpResult LaunchResult = bHasBufferedPointJumpInput
		? BufferedPointJumpResult
		: EPointJumpResult::Normal;

	CustomFinishPointJump(LaunchResult);
}

void UPointJumpActionComponent::CustomUpdateAfterLaunch()
{
	const UCharacterMovementComponent* MovementComponent =
		OwnerCharacter->GetCharacterMovement();

	// Launch終了後に地面へ戻ったら、次のポイントジャンプを受け付けられる状態へ戻す。
	if (MovementComponent && MovementComponent->IsMovingOnGround())
	{
		CustomResetPointJump();
	}
}

/// =================================================================
/// 状態遷移
/// =================================================================
void UPointJumpActionComponent::CustomBeginPointJump(
	UPointJumpTargetComponent* TargetComponent
)
{
	if (!OwnerCharacter || !TargetComponent) return;

	CurrentPointJumpTarget = TargetComponent;
	PointJumpState = EPointJumpState::Startup;
	PointJumpStateTimer = PointJumpStartupDuration;

	bHasBufferedPointJumpInput = false;
	BufferedPointJumpResult = EPointJumpResult::Normal;
	bHasExecutedLaunch = false;
	BufferedMoveDirection = FVector::ZeroVector;

	const FVector TargetLocation = TargetComponent->CustomGetPointJumpLocation();
	const UCapsuleComponent* CapsuleComponent = OwnerCharacter->GetCapsuleComponent();
	const float CapsuleHalfHeight = CapsuleComponent
		? CapsuleComponent->GetScaledCapsuleHalfHeight()
		: 0.0f;

	PointJumpTargetLocation =
		TargetLocation + FVector::UpVector * CapsuleHalfHeight;

	CustomSetPointJumpMovementEnabled(false);
}

void UPointJumpActionComponent::CustomEnterPulling()
{
	PointJumpState = EPointJumpState::Pulling;
}

void UPointJumpActionComponent::CustomEnterLandingSnap()
{
	PointJumpState = EPointJumpState::LandingSnap;

	if (UCharacterMovementComponent* MovementComponent =
		OwnerCharacter->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}
}

void UPointJumpActionComponent::CustomEnterLanding()
{
	PointJumpState = EPointJumpState::Landing;

	if (UCharacterMovementComponent* MovementComponent =
		OwnerCharacter->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->SetMovementMode(MOVE_Walking);
	}
}

void UPointJumpActionComponent::CustomEnterLaunch()
{
	PointJumpState = EPointJumpState::Launch;
	bHasExecutedLaunch = false;
}

void UPointJumpActionComponent::CustomFinishPointJump(EPointJumpResult Result)
{
	if (!OwnerCharacter) return;
	if (PointJumpState == EPointJumpState::None) return;

	CurrentPointJumpTarget = nullptr;
	CustomSetPointJumpMovementEnabled(true);

	const FVector LaunchVelocity =
		CustomGetPointJumpLaunchDirection() * CustomGetPointJumpForwardPower(Result)
		+ FVector::UpVector * CustomGetPointJumpUpPower(Result);

	OwnerCharacter->LaunchCharacter(LaunchVelocity, true, true);
}

void UPointJumpActionComponent::CustomResetPointJump()
{
	PointJumpState = EPointJumpState::None;
	PointJumpStateTimer = 0.0f;
	PointJumpTargetLocation = FVector::ZeroVector;
	BufferedMoveDirection = FVector::ZeroVector;

	CurrentPointJumpTarget = nullptr;
	bHasBufferedPointJumpInput = false;
	BufferedPointJumpResult = EPointJumpResult::Normal;
	bHasExecutedLaunch = false;
}

/// =================================================================
/// アニメーション通知
/// =================================================================
void UPointJumpActionComponent::CustomOnPointJumpLandingAnimationFinished()
{
	if (PointJumpState != EPointJumpState::Landing) return;

	CustomResetPointJump();

	if (UCharacterMovementComponent* MovementComponent =
		OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr)
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
	}
}

void UPointJumpActionComponent::CustomOnPointJumpLaunchAnimationFinished()
{
	if (PointJumpState != EPointJumpState::Launch) return;
	PointJumpState = EPointJumpState::AfterLaunch;
}

/// =================================================================
/// タイミング判定
/// =================================================================
void UPointJumpActionComponent::CustomBufferPointJumpInput()
{
	bHasBufferedPointJumpInput = true;
	BufferedPointJumpResult = CustomJudgePointJumpInputTiming();

	UE_LOG(LogTemp, Log, TEXT("PointJump input buffered: %s"),
		BufferedPointJumpResult == EPointJumpResult::Perfect
		? TEXT("Perfect")
		: TEXT("Normal"));
}

EPointJumpResult UPointJumpActionComponent::CustomJudgePointJumpInputTiming() const
{
	return CustomGetPointJumpRemainingTime() <= PointJumpPerfectWindow
		? EPointJumpResult::Perfect
		: EPointJumpResult::Normal;
}

float UPointJumpActionComponent::CustomGetPointJumpRemainingTime() const
{
	if (!OwnerCharacter) return TNumericLimits<float>::Max();
	if (PointJumpPullSpeed <= KINDA_SMALL_NUMBER)
	{
		return TNumericLimits<float>::Max();
	}

	const float RemainingDistance = FVector::Dist(
		OwnerCharacter->GetActorLocation(),
		PointJumpTargetLocation
	);

	return RemainingDistance / PointJumpPullSpeed;
}

/// =================================================================
/// ターゲット検索・判定
/// =================================================================
bool UPointJumpActionComponent::CustomTryFindPointJumpTarget(
	UPointJumpTargetComponent*& OutTargetComponent
) const
{
	OutTargetComponent = nullptr;

	if (!OwnerCharacter || !OwnerCamera || !GetWorld()) return false;

	const FVector CameraLocation = OwnerCamera->GetComponentLocation();
	const FVector CameraForward = OwnerCamera->GetForwardVector();

	UPointJumpTargetComponent* BestTarget = nullptr;
	float BestViewDot = PointJumpMinViewDot;

	for (TObjectIterator<UPointJumpTargetComponent> It; It; ++It)
	{
		UPointJumpTargetComponent* TargetComponent = *It;
		if (!TargetComponent) continue;
		if (TargetComponent->GetWorld() != GetWorld()) continue;
		if (!CustomIsValidPointJumpTarget(
			TargetComponent,
			CameraLocation,
			CameraForward
		))
		{
			continue;
		}

		const FVector TargetLocation =
			TargetComponent->CustomGetPointJumpLocation();
		const FVector ToTarget =
			(TargetLocation - CameraLocation).GetSafeNormal();
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

bool UPointJumpActionComponent::CustomIsValidPointJumpTarget(
	const UPointJumpTargetComponent* TargetComponent,
	const FVector& CameraLocation,
	const FVector& CameraForward
) const
{
	if (!TargetComponent || !OwnerCharacter) return false;
	if (!TargetComponent->CustomCanPointJump()) return false;

	const AActor* TargetOwnerActor = TargetComponent->GetOwner();
	if (!TargetOwnerActor || TargetOwnerActor == OwnerCharacter) return false;

	const FVector TargetLocation = TargetComponent->CustomGetPointJumpLocation();
	const float Distance = FVector::Dist(
		OwnerCharacter->GetActorLocation(),
		TargetLocation
	);
	if (Distance > PointJumpSearchRadius) return false;

	const FVector ToTarget = (TargetLocation - CameraLocation).GetSafeNormal();
	if (ToTarget.IsNearlyZero()) return false;

	const float ViewDot = FVector::DotProduct(CameraForward, ToTarget);
	return ViewDot >= PointJumpMinViewDot;
}

/// =================================================================
/// 射出計算
/// =================================================================
FVector UPointJumpActionComponent::CustomGetPointJumpLaunchDirection() const
{
	if (!BufferedMoveDirection.IsNearlyZero())
	{
		return BufferedMoveDirection.GetSafeNormal2D();
	}

	if (!OwnerCharacter) return FVector::ForwardVector;

	const FVector LastMovementInput = OwnerCharacter->GetLastMovementInputVector();
	if (!LastMovementInput.IsNearlyZero())
	{
		return LastMovementInput.GetSafeNormal2D();
	}

	const AController* Controller = OwnerCharacter->GetController();
	if (!Controller)
	{
		return OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	return FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X).GetSafeNormal2D();
}

float UPointJumpActionComponent::CustomGetPointJumpForwardPower(
	EPointJumpResult Result
) const
{
	switch (Result)
	{
	case EPointJumpResult::Perfect:
		return PointJumpPerfectForwardPower;
	case EPointJumpResult::Normal:
	default:
		return PointJumpNormalForwardPower;
	}
}

float UPointJumpActionComponent::CustomGetPointJumpUpPower(
	EPointJumpResult Result
) const
{
	switch (Result)
	{
	case EPointJumpResult::Perfect:
		return PointJumpPerfectUpPower;
	case EPointJumpResult::Normal:
	default:
		return PointJumpNormalUpPower;
	}
}

/// =================================================================
/// 移動制御
/// =================================================================
float UPointJumpActionComponent::CustomGetLandingSnapStartDistance() const
{
	if (CurrentPointJumpTarget
		&& CurrentPointJumpTarget->ArriveDistanceOverride > 0.0f)
	{
		return CurrentPointJumpTarget->ArriveDistanceOverride;
	}

	return DefaultLandingSnapStartDistance;
}

void UPointJumpActionComponent::CustomSetPointJumpMovementEnabled(bool bEnabled)
{
	if (!OwnerCharacter) return;

	UCharacterMovementComponent* MovementComponent =
		OwnerCharacter->GetCharacterMovement();
	if (!MovementComponent) return;

	MovementComponent->StopMovementImmediately();

	if (bEnabled)
	{
		MovementComponent->SetMovementMode(MOVE_Falling);
		return;
	}

	MovementComponent->SetMovementMode(MOVE_Flying);
}
