#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PointJumpActionComponent.generated.h"

class ACharacter;
class UCameraComponent;
class UPointJumpTargetComponent;

enum class EPointJumpResult : uint8
{
	Normal,
	Perfect
};

UENUM(BlueprintType)
enum class EPointJumpState : uint8
{
	None,
	Startup,
	Pulling,
	LandingSnap,
	Landing,
	Launch,
	AfterLaunch
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UPointJumpActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPointJumpActionComponent();

	// ポイントジャンプ入力の入口。
	// 未実行なら開始し、実行中ならジャンプ入力を先行入力として保存する。
	void CustomTryStartPointJump();

	// Startup～Landing中の移動入力を、射出方向として保存する。
	void CustomSetPointJumpMoveDirection(const FVector& MoveDirection);

	UFUNCTION(BlueprintCallable, Category = "PointJump|Animation")
	void CustomOnPointJumpLandingAnimationFinished();

	UFUNCTION(BlueprintCallable, Category = "PointJump|Animation")
	void CustomOnPointJumpLaunchAnimationFinished();

	bool CustomIsPointJumpActive() const;
	bool CustomIsLanding() const;
protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

private:
	/// =================================================================
	/// 入力処理
	/// =================================================================
	void CustomBufferPointJumpInput();
	bool CustomCanBufferPointJumpInput() const;
	bool CustomCanBufferMoveDirection() const;

	/// =================================================================
	/// ステートマシン更新
	/// =================================================================
	void CustomUpdatePointJump(float DeltaTime);
	void CustomUpdateStartup(float DeltaTime);
	void CustomUpdatePulling(float DeltaTime);
	void CustomUpdateLandingSnap(float DeltaTime);
	void CustomUpdateLanding(float DeltaTime);
	void CustomUpdateLaunch();
	void CustomUpdateAfterLaunch();

	/// =================================================================
	/// 状態遷移
	/// =================================================================
	void CustomBeginPointJump(UPointJumpTargetComponent* TargetComponent);
	void CustomEnterPulling();
	void CustomEnterLandingSnap();
	void CustomEnterLanding();
	void CustomEnterLaunch();
	void CustomFinishPointJump(EPointJumpResult Result);
	void CustomResetPointJump();

	/// =================================================================
	/// ターゲット検索・判定
	/// =================================================================
	bool CustomTryFindPointJumpTarget(UPointJumpTargetComponent*& OutTargetComponent) const;
	bool CustomIsValidPointJumpTarget(
		const UPointJumpTargetComponent* TargetComponent,
		const FVector& CameraLocation,
		const FVector& CameraForward
	) const;
	bool CustomHasLineOfSightToTarget(
		const UPointJumpTargetComponent* TargetComponent,
		const FVector& CameraLocation
	) const;

	/// =================================================================
	/// タイミング判定
	/// =================================================================
	EPointJumpResult CustomJudgePointJumpInputTiming() const;
	float CustomGetPointJumpRemainingTime() const;

	/// =================================================================
	/// 射出計算
	/// =================================================================
	FVector CustomGetPointJumpLaunchDirection() const;
	float CustomGetPointJumpForwardPower(EPointJumpResult Result) const;
	float CustomGetPointJumpUpPower(EPointJumpResult Result) const;

	/// =================================================================
	/// 移動制御
	/// =================================================================
	float CustomGetLandingSnapStartDistance() const;
	void CustomSetPointJumpMovementEnabled(bool bEnabled);

private:
	/// =================================================================
	/// 参照
	/// =================================================================
	UPROPERTY()
	ACharacter* OwnerCharacter = nullptr;

	UPROPERTY()
	UCameraComponent* OwnerCamera = nullptr;

	UPROPERTY()
	UPointJumpTargetComponent* CurrentPointJumpTarget = nullptr;

	/// =================================================================
	/// 検索設定
	/// =================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Search",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpSearchRadius = 2500.0f;

	// カメラ前方との一致度。1に近いほど画面中央付近だけを対象にする。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Search",
		meta = (AllowPrivateAccess = "true", ClampMin = "-1.0", ClampMax = "1.0"))
	float PointJumpMinViewDot = 0.55f;

	/// =================================================================
	/// 吸着移動設定
	/// =================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Pulling",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpPullSpeed = 2800.0f;

	// Startup終了時、この距離以内ならPullingを飛ばしてLandingSnapへ入る。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Transition",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PullingSkipDistance = 250.0f;

	// Pulling中、この距離以内に入ったらLandingSnapへ切り替える。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Transition",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DefaultLandingSnapStartDistance = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|LandingSnap",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LandingSnapSpeed = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|LandingSnap",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LandingSnapCompleteDistance = 5.0f;

	/// =================================================================
	/// タイミング設定
	/// =================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpPerfectWindow = 0.12f;
	
	// Landingに入ってからの経過時間
	float PointJumpLandingElapsedTime = 0.0f;
	// Landing後もPerfectとして受け付ける時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Timing",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpAfterLandingPerfectWindow = 0.12f;

	/// =================================================================
	/// 射出設定
	/// =================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpNormalForwardPower = 1100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpPerfectForwardPower = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch",
		meta = (AllowPrivateAccess = "true"))
	float PointJumpNormalUpPower = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Launch",
		meta = (AllowPrivateAccess = "true"))
	float PointJumpPerfectUpPower = 850.0f;

	/// =================================================================
	/// アニメーション・状態設定
	/// =================================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PointJump|Animation",
		meta = (AllowPrivateAccess = "true"))
	EPointJumpState PointJumpState = EPointJumpState::None;

	// アニメーションか手触りかの調整
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PointJump|Animation",
		meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PointJumpStartupDuration = 0.3f;

	/// =================================================================
	/// 実行中データ
	/// =================================================================
	FVector PointJumpTargetLocation = FVector::ZeroVector;
	FVector BufferedMoveDirection = FVector::ZeroVector;

	float PointJumpStateTimer = 0.f;

	bool bHasBufferedPointJumpInput = false;
	EPointJumpResult BufferedPointJumpResult = EPointJumpResult::Normal;
	bool bHasExecutedLaunch = false;
};
