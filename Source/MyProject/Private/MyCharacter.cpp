// Fill out your copyright notice in the Description page of Project Settings.

//Self include first
#include "MyCharacter.h"

// .hで前方宣言したクラスの実体をここでインクルード
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PointJumpTargetComponent.h"
#include "UObject/UObjectIterator.h"

/// =================================================================
/// 1. コンストラクタ & 初期化関数
/// =================================================================
// unityの Awake() に相当する関数（コンストラクタ）
AMyCharacter::AMyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 初期化
	CustomSetupCharacterRotation();
	CustomSetupCamera();
	CustomSetupJump();
}

// キャラ本体の回転設定
void AMyCharacter::CustomSetupCharacterRotation()
{
	// デフォルトだと、キャラクターはコントローラーの回転に合わせて回転する設定になっているため、これらを無効にする
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 移動方向にキャラを向ける
	GetCharacterMovement()->bOrientRotationToMovement = true; // これを有効にすると、キャラクターは移動方向に向くようになる
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 700.0f, 0.0f); // 回転速度

}

// カメラの初期設定を行う関数（自分で新しく定義）
void AMyCharacter::CustomSetupCamera()
{
	// カメラアームの作成
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom")); // USpringArmComponentがあるお陰で、カメラが壁に衝突しても、カメラがキャラクターに近づくようになる
	CameraBoom->SetupAttachment(RootComponent); // キャラクターのルートコンポーネントにアタッチ
	CameraBoom->TargetArmLength = 300.0f;		// カメラとキャラクターの距離
	CameraBoom->bUsePawnControlRotation = true; // コントローラーの回転をカメラアームに適用

	// カメラ作成
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera")); // プレイヤーが見る画面そのもの
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // カメラアームの先端にアタッチ
	FollowCamera->bUsePawnControlRotation = false; // カメラはコントローラーの回転をしない
}

// ジャンプ設定
void AMyCharacter::CustomSetupJump()
{
	GetCharacterMovement()->JumpZVelocity = 600.f; // ジャンプの高さ
	JumpMaxCount = 2; // 二段ジャンプを可能にする
}

/// =================================================================
/// 2. ライフサイクル関数
/// =================================================================
// unityの Start() に相当する関数
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();

	CustomSetupEnhancedInputMapping(); // 入力の設定を行うためのおまじない関数
}

// unityの Update() に相当する関数
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CustomUpdatePointJump(DeltaTime);
}

void AMyCharacter::CustomUpdatePointJump(float DeltaTime)
{
	if (!bIsPointJumping) return;

	const FVector CurrentLocation = GetActorLocation();
	const FVector NextLocation =
		FMath::VInterpConstantTo(
			CurrentLocation,
			PointJumpTargetLocation,
			DeltaTime,
			PointJumpPullSpeed
		);
	SetActorLocation(NextLocation, true);

	if (FVector::DistSquared(NextLocation, PointJumpTargetLocation) <= FMath::Square(CustomGetCurrentPointJumpArriveDistance()))
	{
		CustomFinishPointJump(EPointJumpResult::Normal);
	}
}

// 入力のバインドを行う関数（UEのフレームワークが呼び出す関数）
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComp) return;

	CustomBindLookInput(EnhancedInputComp);
	CustomBindMoveInput(EnhancedInputComp);
	CustomBindJumpInput(EnhancedInputComp);
	CustomBindDashInput(EnhancedInputComp);
	CustomBindPointJumpInput(EnhancedInputComp);
}

/// =================================================================
/// 3. 入力バインド（SetupPlayerInputComponent ➔ ヘルパーの順）
/// =================================================================
void AMyCharacter::CustomBindLookInput(UEnhancedInputComponent* EnhancedInputComp)
{
	if (!LookAction) return;
	EnhancedInputComp->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyCharacter::CustomLook);
}

void AMyCharacter::CustomBindMoveInput(UEnhancedInputComponent* EnhancedInputComp)
{
	if (!MoveAction) return;
	EnhancedInputComp->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::CustomMove);
}

void AMyCharacter::CustomBindJumpInput(UEnhancedInputComponent* EnhancedInputComp)
{
	if (!JumpAction) return;
	EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Started, this, &AMyCharacter::CustomOnJumpActionStarted);
}

void AMyCharacter::CustomBindDashInput(UEnhancedInputComponent* EnhancedInputComp)
{
	if (!DashAction) return;
	EnhancedInputComp->BindAction(DashAction, ETriggerEvent::Started,   this, &AMyCharacter::CustomStartDash);
	EnhancedInputComp->BindAction(DashAction, ETriggerEvent::Completed, this, &AMyCharacter::CustomStopDash );
}

void AMyCharacter::CustomBindPointJumpInput(UEnhancedInputComponent* EnhancedInputComp)
{
	if (!PointJumpAction) return;
	EnhancedInputComp->BindAction(PointJumpAction, ETriggerEvent::Started, this, &AMyCharacter::CustomStartPointJump);
}

// Enhanced Input Mappingの初期設定を行う関数
void AMyCharacter::CustomSetupEnhancedInputMapping()
{
	// 1. 人間プレイヤーかチェック
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;

	// 2. ローカルプレイヤーかチェック
	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer) return;

	// 3. 入力サブシステム（管理人）がいるかチェック
	auto* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem) return;

	// 4. エディタ側でアセットが割り当てられているかチェック
	if (!DefaultMappingContext) return;

	// --- すべてのセキュリティを突破！ 本題の処理 ---
	Subsystem->AddMappingContext(DefaultMappingContext, 0);
}

/// =================================================================
/// 4. 入力実行関数
/// =================================================================
// カメラ回転入力の処理
void AMyCharacter::CustomLook(const FInputActionValue& Value)
{
	//IA_Lookの入力は FVector2D 型なので、Value.Get<FVector2D>() で入力値を取得する
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X); // X = 水平方向の回転
		AddControllerPitchInput(-LookAxisVector.Y); // Y = 垂直方向の回転（上下反転を防ぐためマイナス）
		// CameraBoom->bUsePawnControlRotation = true; になっているため、コントローラーの回転がカメラアームに適用され、カメラがキャラクターを追従して回転するようになる
	}
}

// 移動入力の処理
void AMyCharacter::CustomMove(const FInputActionValue& Value)
{
	//IA_Moveの入力は FVector2D 型なので、Value.Get<FVector2D>() で入力値を取得する
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator ControlRotation = Controller->GetControlRotation(); // 1. まず「今、カメラ（コントローラー）はどっちを向いてる？」と確認します（ControlRotation）
		const FRotator YawRotation(0, ControlRotation.Yaw, 0); // 2. 上下を見上げている角度（Pitch）などは邪魔なので、水平方向の向き（Yaw）だけを抜き出します（YawRotation）
		// 3. その向きを基準にして「前（Forward）」と「右（Right）」のベクトルを計算し、スティックの傾き具合（MovementVector）を掛け合わせてキャラクターを動かしています。

		// キャラクターの前方向と右方向を取得する
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y); // Y = 前後移動 
		AddMovementInput(RightDirection, MovementVector.X); // X = 左右移動
	}
}

// ジャンプの処理
void AMyCharacter::CustomOnJumpActionStarted()
{
	if (bIsPointJumping)
	{
		CustomExecutePointJumpTimingInput();
	}
	else
	{
		CustomExecuteNormalJump();
	}
}

void AMyCharacter::CustomExecutePointJumpTimingInput()
{
	const EPointJumpResult Result = CustomJudgePointJumpInputTiming();

	switch (Result) // デバッログ
	{
	case EPointJumpResult::Perfect:UE_LOG(LogTemp, Warning, TEXT("PERFECT")); break;
	case EPointJumpResult::Good:   UE_LOG(LogTemp, Warning, TEXT("GOOD"   )); break;
	case EPointJumpResult::Normal: UE_LOG(LogTemp, Warning, TEXT("NORMAL" )); break;
	}

	CustomFinishPointJump(Result);
	return;
}
void AMyCharacter::CustomExecuteNormalJump()
{
	if (JumpCurrentCount >= JumpMaxCount) return;

	if (JumpCurrentCount > 0)
	{
		PlayDoubleJumpAnimation();
	}

	Jump();
}

// プレイヤーダッシュの処理
void AMyCharacter::CustomStartDash()
{
	bIsDashing = true;
	GetCharacterMovement()->MaxWalkSpeed = DashSpeed;
}
void AMyCharacter::CustomStopDash()
{
	bIsDashing = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

// ポイントジャンプの処理
void AMyCharacter::CustomStartPointJump()
{
	UE_LOG(LogTemp, Warning, TEXT("PointJump Pressed"));

	if (bIsPointJumping) return;

	UPointJumpTargetComponent* TargetComponent = nullptr;
	if (!CustomTryFindPointJumpTarget(TargetComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("Target Not Found"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Target Found"));

	CustomBeginPointJump(TargetComponent);
}


/// =================================================================
/// ポイントジャンプ判定
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

void AMyCharacter::CustomBeginPointJump(UPointJumpTargetComponent* TargetComponent)
{
	if (!TargetComponent) return;

	bIsPointJumping = true;
	CurrentPointJumpTarget = TargetComponent;
	PointJumpTargetLocation = TargetComponent->CustomGetPointJumpLocation();

	CustomStopDash();
	CustomSetPointJumpMovementEnabled(false);
}

void AMyCharacter::CustomFinishPointJump(EPointJumpResult Result)
{
	if (!bIsPointJumping) return;

	bIsPointJumping = false;
	CurrentPointJumpTarget = nullptr;
	CustomSetPointJumpMovementEnabled(true);

	const FVector LanchVelocity = CustomGetPointJumpLaunchDirection() * CustomGetPointJumpForwardPower(Result)
		+ FVector::UpVector * CustomGetPointJumpUpPower(Result);

	LaunchCharacter(LanchVelocity, true, true);
}
// ポイント判定
EPointJumpResult AMyCharacter::CustomJudgePointJumpInputTiming() const
{
	const float Timing = CustomGetPointJumpRemainingTime();

	if (Timing <= PointJumpPerfectWindow)
	{
		return EPointJumpResult::Perfect;
	}
	if (Timing <= PointJumpGoodWindow)
	{
		return EPointJumpResult::Good;
	}
	return EPointJumpResult::Normal;
}

// ポイント判定結果
float AMyCharacter::CustomGetPointJumpForwardPower(EPointJumpResult Result) const
{
	switch (Result)
	{
	case EPointJumpResult::Perfect: return PointJumpPerfectForwardPower;
	case EPointJumpResult::Good:    return PointJumpGoodForwardPower;
	case EPointJumpResult::Normal:  return PointJumpNormalForwardPower;
	default: return PointJumpNormalForwardPower;
	}
}
float AMyCharacter::CustomGetPointJumpUpPower(EPointJumpResult Result) const
{
	switch (Result)
	{
	case EPointJumpResult::Perfect:return PointJumpPerfectUpPower;
	case EPointJumpResult::Good:   return PointJumpGoodUpPower;
	case EPointJumpResult::Normal: return PointJumpNormalUpPower;
	default:return PointJumpNormalUpPower;
	}
}

// ポイント結果後
FVector AMyCharacter::CustomGetPointJumpLaunchDirection() const
{
	if (!Controller) return GetActorForwardVector();

	const FVector LastMovementInput = GetLastMovementInputVector();
	if (!LastMovementInput.IsNearlyZero())
	{
		return LastMovementInput.GetSafeNormal2D();
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	return FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X).GetSafeNormal2D();
}

float AMyCharacter::CustomGetPointJumpRemainingTime() const
{
	if (PointJumpPullSpeed <= 0.0f) return 0.0f;

	const float RemainingDistance = FVector::Dist(GetActorLocation(), PointJumpTargetLocation);
	return RemainingDistance / PointJumpPullSpeed;
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
