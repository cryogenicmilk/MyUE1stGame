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
// 自作
#include "PointJumpActionComponent.h"

/// =================================================================
/// コンストラクタ
/// =================================================================
// unityの Awake() に相当する関数（コンストラクタ）
AMyCharacter::AMyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	PointJumpActionComponent = 
		CreateDefaultSubobject<UPointJumpActionComponent>(TEXT("PointJumpActionComponent"));

	// 初期化
	CustomCreateCameraComponents();
}

/// =================================================================
/// コンポーネント生成
/// =================================================================
void AMyCharacter::CustomCreateCameraComponents()
{
	// カメラアームの作成
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom")); // USpringArmComponentがあるお陰で、カメラが壁に衝突しても、カメラがキャラクターに近づくようになる
	CameraBoom->SetupAttachment(RootComponent); // キャラクターのルートコンポーネントにアタッチ
	CameraBoom->bUsePawnControlRotation = true; // コントローラーの回転をカメラアームに適用

	// カメラ作成
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera")); // プレイヤーが見る画面そのもの
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // カメラアームの先端にアタッチ
	FollowCamera->bUsePawnControlRotation = false; // カメラはコントローラーの回転をしない
}

/// =================================================================
/// ライフサイクル
/// =================================================================
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();

	CustomApplyCharacterRotationSettings();
	CustomApplyMovementSettings();
	CustomSetupEnhancedInputMapping();
}

/// =================================================================
/// キャラクター設定
/// =================================================================
void AMyCharacter::CustomApplyCharacterRotationSettings()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent) return;

	MovementComponent->bOrientRotationToMovement = true;
	MovementComponent->RotationRate = FRotator(0.0f, CharacterYawSpeed, 0.0f);
}

void AMyCharacter::CustomApplyMovementSettings()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent) return;

	MovementComponent->MaxWalkSpeed = WalkSpeed;
	MovementComponent->JumpZVelocity = JumpVelocity;
	JumpMaxCount = MaxJumpCount;

	// 元々CustomCreateCameraComponents()に書いていたが、
	// 調整値はBlueprintで変更した値を反映したいので、BeginPlay()で設定するように変更
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = CameraArmLength;
	}
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
/// 入力セットアップ
/// =================================================================
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComponent) return;

	CustomBindLookInput(EnhancedInputComponent);
	CustomBindMoveInput(EnhancedInputComponent);
	CustomBindDashInput(EnhancedInputComponent);
	CustomBindJumpInput(EnhancedInputComponent);
	CustomBindPointJumpInput(EnhancedInputComponent);
}

/// =================================================================
/// 入力バインド
/// =================================================================
void AMyCharacter::CustomBindLookInput(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (!LookAction) return;

	EnhancedInputComponent->BindAction(
		LookAction,
		ETriggerEvent::Triggered,
		this,
		&AMyCharacter::CustomHandleLookInput
	);
}

void AMyCharacter::CustomBindMoveInput(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (!MoveAction) return;

	EnhancedInputComponent->BindAction(
		MoveAction,
		ETriggerEvent::Triggered,
		this,
		&AMyCharacter::CustomHandleMoveInput
	);
}

void AMyCharacter::CustomBindDashInput(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (!DashAction) return;

	EnhancedInputComponent->BindAction(
		DashAction,
		ETriggerEvent::Started,
		this,
		&AMyCharacter::CustomHandleDashStarted
	);

	EnhancedInputComponent->BindAction(
		DashAction,
		ETriggerEvent::Completed,
		this,
		&AMyCharacter::CustomHandleDashEnded
	);

	// 入力が別処理によって中断された場合も歩行速度へ戻す。
	EnhancedInputComponent->BindAction(
		DashAction,
		ETriggerEvent::Canceled,
		this,
		&AMyCharacter::CustomHandleDashEnded
	);
}

void AMyCharacter::CustomBindJumpInput(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (!JumpAction) return;

	EnhancedInputComponent->BindAction(
		JumpAction,
		ETriggerEvent::Started,
		this,
		&AMyCharacter::CustomHandleJumpInput
	);
}

void AMyCharacter::CustomBindPointJumpInput(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (!PointJumpAction) return;

	EnhancedInputComponent->BindAction(
		PointJumpAction,
		ETriggerEvent::Started,
		this,
		&AMyCharacter::CustomHandlePointJumpInput
	);
}

/// =================================================================
/// 入力処理
/// =================================================================
void AMyCharacter::CustomHandleLookInput(const FInputActionValue& Value)
{
	if (!Controller) return;

	const FVector2D LookInput = Value.Get<FVector2D>();

	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(-LookInput.Y);
}

void AMyCharacter::CustomHandleMoveInput(const FInputActionValue& Value)
{
	if (!Controller) return;

	const FVector2D MoveInput = Value.Get<FVector2D>();

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

	const FVector ForwardDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	const FVector RightDirection =
		FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	const FVector WorldMoveDirection =
		ForwardDirection * MoveInput.Y
		+ RightDirection * MoveInput.X;

	// ポイントジャンプ中の射出方向候補をコンポーネントへ渡す。
	if (PointJumpActionComponent)
	{
		PointJumpActionComponent->CustomSetPointJumpMoveDirection(WorldMoveDirection);
	}

	AddMovementInput(ForwardDirection, MoveInput.Y);
	AddMovementInput(RightDirection, MoveInput.X);
}

void AMyCharacter::CustomHandleDashStarted()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent) return;

	MovementComponent->MaxWalkSpeed = DashSpeed;
}

void AMyCharacter::CustomHandleDashEnded()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent) return;

	MovementComponent->MaxWalkSpeed = WalkSpeed;
}

void AMyCharacter::CustomHandleJumpInput()
{
	// ポイントジャンプ中なら
	//Qでポイントジャンプができるので制限する予定？（QでアクションならQでアクションを終わらせるよな？でもポイント"ジャンプ"だからspaceでやった方がよくね？）
	if (PointJumpActionComponent)
	{
		if(PointJumpActionComponent->CustomIsPointJumpActive())
		{
			PointJumpActionComponent->CustomTryStartPointJump();
			return;
		}
	}

	// 通常ジャンプの処理
	if (JumpCurrentCount >= JumpMaxCount) return;

	if (JumpCurrentCount > 0)
	{
		CustomPerformAirJump();
		return;
	}

	CustomPerformGroundJump();
}

void AMyCharacter::CustomHandlePointJumpInput()
{
	if (!PointJumpActionComponent) return;

	PointJumpActionComponent->CustomTryStartPointJump();
}

/// =================================================================
/// 通常ジャンプ
/// =================================================================
void AMyCharacter::CustomPerformGroundJump()
{
	Jump();
}

void AMyCharacter::CustomPerformAirJump()
{
	PlayDoubleJumpAnimation();
	Jump();
}

// 今は使ってない。今後エフェクトやアニメーションを入れるときに使うかも
//void AMyCharacter::Landed(const FHitResult& Hit)
//{
//	Super::Landed(Hit);
//}
