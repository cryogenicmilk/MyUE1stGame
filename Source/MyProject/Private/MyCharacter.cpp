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

/// =================================================================
/// 1. コンストラクタ & 初期化関数
/// =================================================================
// unityの Awake() に相当する関数（コンストラクタ）
AMyCharacter::AMyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// 初期化
	SetupCharacterRotation();
	SetupCamera();
	SetupJump();
}

// キャラ本体の回転設定
void AMyCharacter::SetupCharacterRotation()
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
void AMyCharacter::SetupCamera()
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
void AMyCharacter::SetupJump()
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

	SetupEnhancedInputMapping(); // 入力の設定を行うためのおまじない関数（自分で新しく定義）
}

// unityの Update() に相当する関数
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// 入力のバインドを行う関数（UEのフレームワークが呼び出す関数）
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComp) return;

	BindLookInput(EnhancedInputComp);
	BindMoveInput(EnhancedInputComp);
	BindJumpInput(EnhancedInputComp);
	BindDashInput(EnhancedInputComp);
}

/// =================================================================
/// 3. 入力バインド（SetupPlayerInputComponent ➔ ヘルパーの順）
/// =================================================================
void AMyCharacter::BindLookInput(UEnhancedInputComponent* EnhancedInputComp)
{
	if (!LookAction) return;
	EnhancedInputComp->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyCharacter::Look);
}

void AMyCharacter::BindMoveInput(UEnhancedInputComponent* EnhancedInputComp)
{
	if (!MoveAction) return;
	EnhancedInputComp->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCharacter::Move);
}

void AMyCharacter::BindJumpInput(UEnhancedInputComponent* EnhancedInputComp)
{
	if (!JumpAction) return;
	EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Started,   this, &AMyCharacter::StartJump);
	EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
}

void AMyCharacter::BindDashInput(UEnhancedInputComponent* EnhancedInputComp)
{
	if (!DashAction) return;
	EnhancedInputComp->BindAction(DashAction, ETriggerEvent::Started,   this, &AMyCharacter::StartDash);
	EnhancedInputComp->BindAction(DashAction, ETriggerEvent::Completed, this, &AMyCharacter::StopDash );
}

// Enhanced Input Mappingの初期設定を行う関数（自分で新しく定義）
void AMyCharacter::SetupEnhancedInputMapping()
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
void AMyCharacter::Look(const FInputActionValue& Value)
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
void AMyCharacter::Move(const FInputActionValue& Value)
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
void AMyCharacter::StartJump()
{
	if (JumpCurrentCount >= JumpMaxCount) return;

	if (JumpCurrentCount > 0)
	{
		PlayDoubleJumpAnimation();
	}

	Jump();
}

// プレイヤーダッシュの処理
void AMyCharacter::StartDash()
{
	bIsDashing = true;
	GetCharacterMovement()->MaxWalkSpeed = DashSpeed;
}
void AMyCharacter::StopDash()
{
	bIsDashing = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}
