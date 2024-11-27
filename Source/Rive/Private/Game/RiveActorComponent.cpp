// Copyright Rive, Inc. All rights reserved.

#include "Game/RiveActorComponent.h"

#include "GameFramework/Actor.h"
#include "IRiveRenderer.h"
#include "IRiveRendererModule.h"
#include "Logs/RiveLog.h"
#include "Rive/RiveArtboard.h"
#include "Rive/RiveDescriptor.h"
#include "Rive/RiveFile.h"
#include "Rive/RiveTexture.h"
#include "Stats/RiveStats.h"

class FRiveStateMachine;

constexpr rive::ColorInt Cyan = 0xFF00FFFF;
constexpr rive::ColorInt Magenta = 0xFFFF00FF;
constexpr rive::ColorInt Yellow = 0xFFFFFF00;
constexpr rive::ColorInt Black = 0xFF000000;
constexpr rive::ColorInt Green = 0xFF00FF00;
constexpr rive::ColorInt Blue = 0xFF0000FF;

void DrawDefaultTest(rive::Factory* factory,
                     rive::Renderer* renderer,
                     FIntPoint Size)
{
    constexpr rive::ColorInt colors[] = {Cyan, Magenta, Yellow, Cyan};
    constexpr float stops[] = {0.0, 0.33, 0.66, 1};
    auto gradientShader =
        factory->makeRadialGradient(120, 120, Size.X / 2, colors, stops, 4);

    // Gradient Test
    rive::RawPath FullRect, notFullRect;
    FullRect.addRect(
        {0, 0, static_cast<float>(Size.X), static_cast<float>(Size.Y)});
    notFullRect.addRect({0, 0, 100, static_cast<float>(Size.Y)});
    auto FullRectRenderPath =
        factory->makeRenderPath(FullRect, rive::FillRule::nonZero);
    auto notFullRectRenderPath =
        factory->makeRenderPath(notFullRect, rive::FillRule::nonZero);

    auto FullRectPaint = factory->makeRenderPaint();
    FullRectPaint->style(rive::RenderPaintStyle::fill);
    // FullRectPaint->color(Magenta);
    FullRectPaint->shader(gradientShader);

    auto notFullRectPaint = factory->makeRenderPaint();
    notFullRectPaint->style(rive::RenderPaintStyle::fill);
    notFullRectPaint->color(Yellow);
    renderer->drawPath(FullRectRenderPath.get(), FullRectPaint.get());
    renderer->drawPath(notFullRectRenderPath.get(), notFullRectPaint.get());

    // //Shapes Test
    rive::RawPath rectPath, rectPath2;
    rectPath.addRect({10, 10, 70, 30});
    rectPath2.addRect({80, 10, 140, 30});
    auto renderRectPath =
        factory->makeRenderPath(rectPath, rive::FillRule::nonZero);
    auto renderRectPath2 =
        factory->makeRenderPath(rectPath2, rive::FillRule::nonZero);

    auto fillPaint = factory->makeRenderPaint();
    fillPaint->style(rive::RenderPaintStyle::fill);
    fillPaint->color(Black);

    auto strokePaint = factory->makeRenderPaint();
    strokePaint->style(rive::RenderPaintStyle::stroke);
    strokePaint->thickness(5.0f);
    strokePaint->color(Black);

    renderer->drawPath(renderRectPath.get(), fillPaint.get());
    renderer->drawPath(renderRectPath2.get(), strokePaint.get());

    strokePaint->thickness(8.0f);
    rive::RawPath ovalPath;
    ovalPath.addOval({150, 10, 210, 30});
    auto ovalRenderPath =
        factory->makeRenderPath(ovalPath, rive::FillRule::nonZero);
    renderer->drawPath(ovalRenderPath.get(), strokePaint.get());

    // Line Test
    rive::RawPath linePath1;
    auto linePaint1 = factory->makeRenderPaint();
    linePath1.moveTo(10, 410);
    linePath1.quadTo(556, 364, 428, 420);
    linePath1.quadTo(310, 492, 550, 550);

    linePaint1->style(rive::RenderPaintStyle::stroke);
    linePaint1->cap(rive::StrokeCap::round);
    linePaint1->color(Green);
    linePaint1->thickness(10);

    auto lineRenderPath1 = factory->makeEmptyRenderPath();
    linePath1.addTo(lineRenderPath1.get());
    renderer->drawPath(lineRenderPath1.get(), linePaint1.get());

    // Line Test2
    rive::RawPath linePath2;
    auto linePaint2 = factory->makeRenderPaint();

    linePaint2->style(rive::RenderPaintStyle::stroke);
    linePaint2->cap(rive::StrokeCap::round);
    linePaint2->color(Blue);
    linePaint2->thickness(20);
    linePaint2->join(rive::StrokeJoin::bevel);

    linePath2.moveTo(100, 600);
    linePath2.lineTo(1000, 600);
    linePath2.moveTo(1000, 600);
    linePath2.lineTo(1000, 1000);
    linePath2.moveTo(1000, 1000);
    linePath2.lineTo(100, 1000);

    auto lineRenderPath2 =
        factory->makeRenderPath(linePath2, rive::FillRule::nonZero);
    renderer->drawPath(lineRenderPath2.get(), linePaint2.get());
}

URiveActorComponent::URiveActorComponent() : Size(500, 500)
{
    // Set this component to be initialized when the game starts, and to be
    // ticked every frame.  You can turn these features off to improve
    // performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;
}

void URiveActorComponent::BeginPlay()
{
    Initialize();
    Super::BeginPlay();
}

void URiveActorComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsValidChecked(this))
    {
        return;
    }

    SCOPED_NAMED_EVENT_TEXT(TEXT("URiveActorComponent::TickComponent"),
                            FColor::White);
    DECLARE_SCOPE_CYCLE_COUNTER(TEXT("URiveActorComponent::TickComponent"),
                                STAT_RIVEACTORCOMPONENT_TICK,
                                STATGROUP_Rive);

    if (RiveRenderTarget)
    {
        for (URiveArtboard* Artboard : Artboards)
        {
            RiveRenderTarget->Save();
            Artboard->Tick(DeltaTime);
            RiveRenderTarget->Restore();
        }

        RiveRenderTarget->SubmitAndClear();
    }
}

void URiveActorComponent::Initialize()
{
    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();
    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("RiveRenderer is null, unable to initialize the "
                    "RenderTarget for Rive file '%s'"),
               *GetFullNameSafe(this));
        return;
    }

    RiveRenderer->CallOrRegister_OnInitialized(
        IRiveRenderer::FOnRendererInitialized::FDelegate::CreateUObject(
            this,
            &URiveActorComponent::RiveReady));
}

void URiveActorComponent::RenderRiveTest()
{
    if (!RiveTexture)
    {
        UE_LOG(LogRive, Error, TEXT("RiveRenderTest, RiveTexture not init"));
        return;
    }

    if (!IRiveRendererModule::IsAvailable())
    {
        UE_LOG(LogRive,
               Error,
               TEXT("RiveRenderTest, Rive Renderer Module is either missing or "
                    "not loaded properly."));
        return;
    }

    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();

    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to RiveRenderTest, as we do not have a valid "
                    "renderer."));
        return;
    }

    if (!RiveRenderer->IsInitialized())
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Could not RiveRenderTest,  as the required Rive Renderer "
                    "is not initialized."));
        return;
    }

    RiveRenderTarget->RegisterRenderCommand(
        [Size = this->Size](rive::Factory* factory, rive::Renderer* renderer) {
            DrawDefaultTest(factory, renderer, Size);
        });
}

void URiveActorComponent::ResizeRenderTarget(int32 InSizeX, int32 InSizeY)
{
    if (!RiveTexture)
    {
        return;
    }

    RiveTexture->ResizeRenderTargets(FIntPoint(InSizeX, InSizeY));
}

URiveArtboard* URiveActorComponent::AddArtboard(
    URiveFile* InRiveFile,
    const FString& InArtboardName,
    const FString& InStateMachineName)
{
    if (!IsValid(InRiveFile))
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Can't instantiate an artboard without a valid RiveFile."));
        return nullptr;
    }
    if (!InRiveFile->IsInitialized())
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Can't instantiate an artboard a RiveFile that is not "
                    "initialized!"));
        return nullptr;
    }

    if (!IRiveRendererModule::IsAvailable())
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Could not load rive file as the required Rive Renderer "
                    "Module is either "
                    "missing or not loaded properly."));
        return nullptr;
    }

    IRiveRenderer* RiveRenderer = IRiveRendererModule::Get().GetRenderer();

    if (!RiveRenderer)
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Failed to instantiate the Artboard of Rive file '%s' as "
                    "we do not have a "
                    "valid renderer."),
               *GetFullNameSafe(InRiveFile));
        return nullptr;
    }

    if (!RiveRenderer->IsInitialized())
    {
        UE_LOG(LogRive,
               Error,
               TEXT("Could not load rive file as the required Rive Renderer is "
                    "not initialized."));
        return nullptr;
    }

    URiveArtboard* Artboard = NewObject<URiveArtboard>();
    Artboard->Initialize(InRiveFile,
                         RiveRenderTarget,
                         InArtboardName,
                         InStateMachineName);
    Artboards.Add(Artboard);

    if (RiveAudioEngine != nullptr)
    {
        Artboard->SetAudioEngine(RiveAudioEngine);
    }

    return Artboard;
}

void URiveActorComponent::RemoveArtboard(URiveArtboard* InArtboard)
{
    Artboards.RemoveSingle(InArtboard);
}

URiveArtboard* URiveActorComponent::GetDefaultArtboard() const
{
    return GetArtboardAtIndex(0);
}

URiveArtboard* URiveActorComponent::GetArtboardAtIndex(int32 InIndex) const
{
    if (Artboards.IsEmpty())
    {
        return nullptr;
    }

    if (InIndex >= Artboards.Num())
    {
        UE_LOG(LogRive,
               Warning,
               TEXT("GetArtboardAtIndex with index %d is out of bounds"),
               InIndex);
        return nullptr;
    }

    return Artboards[InIndex];
}

int32 URiveActorComponent::GetArtboardCount() const { return Artboards.Num(); }

void URiveActorComponent::SetAudioEngine(URiveAudioEngine* InRiveAudioEngine)
{
    RiveAudioEngine = InRiveAudioEngine;
    InitializeAudioEngine();
}

#if WITH_EDITOR
void URiveActorComponent::PostEditChangeChainProperty(
    FPropertyChangedChainEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.GetPropertyName();
    const FName ActiveMemberNodeName =
        *PropertyChangedEvent.PropertyChain.GetActiveMemberNode()
             ->GetValue()
             ->GetName();

    if (PropertyName == GET_MEMBER_NAME_CHECKED(FRiveDescriptor, RiveFile) ||
        PropertyName ==
            GET_MEMBER_NAME_CHECKED(FRiveDescriptor, ArtboardIndex) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(FRiveDescriptor, ArtboardName))
    {
        TArray<FString> ArtboardNames = GetArtboardNamesForDropdown();
        if (ArtboardNames.Num() > 0 &&
            DefaultRiveDescriptor.ArtboardIndex == 0 &&
            (DefaultRiveDescriptor.ArtboardName.IsEmpty() ||
             !ArtboardNames.Contains(DefaultRiveDescriptor.ArtboardName)))
        {
            DefaultRiveDescriptor.ArtboardName = ArtboardNames[0];
        }

        TArray<FString> StateMachineNames = GetStateMachineNamesForDropdown();
        if (StateMachineNames.Num() == 1)
        {
            DefaultRiveDescriptor.StateMachineName =
                StateMachineNames[0]; // No state machine, use blank
        }
        else if (DefaultRiveDescriptor.StateMachineName.IsEmpty() ||
                 !StateMachineNames.Contains(
                     DefaultRiveDescriptor.StateMachineName))
        {
            DefaultRiveDescriptor.StateMachineName = StateMachineNames[1];
        }
    }
}
#endif

void URiveActorComponent::OnResourceInitialized_RenderThread(
    FRHICommandListImmediate& RHICmdList,
    FTextureRHIRef& NewResource)
{
    // When the resource change, we need to tell the Render Target otherwise we
    // will keep on drawing on an outdated RT
    if (const TSharedPtr<IRiveRenderTarget> RTarget =
            RiveRenderTarget) // todo: might need a lock
    {
        RTarget->CacheTextureTarget_RenderThread(RHICmdList, NewResource);
    }
}

void URiveActorComponent::OnDefaultArtboardTickRender(float DeltaTime,
                                                      URiveArtboard* InArtboard)
{
    InArtboard->Align(DefaultRiveDescriptor.FitType,
                      DefaultRiveDescriptor.Alignment,
                      DefaultRiveDescriptor.ScaleFactor);
    InArtboard->Draw();
}

TArray<FString> URiveActorComponent::GetArtboardNamesForDropdown() const
{
    TArray<FString> Output;
    if (DefaultRiveDescriptor.RiveFile)
    {
        for (URiveArtboard* Artboard :
             DefaultRiveDescriptor.RiveFile->Artboards)
        {
            Output.Add(Artboard->GetArtboardName());
        }
    }
    return Output;
}

TArray<FString> URiveActorComponent::GetStateMachineNamesForDropdown() const
{
    TArray<FString> Output{""};
    if (DefaultRiveDescriptor.RiveFile)
    {
        for (URiveArtboard* Artboard :
             DefaultRiveDescriptor.RiveFile->Artboards)
        {
            if (Artboard->GetArtboardName().Equals(
                    DefaultRiveDescriptor.ArtboardName))
            {
                Output.Append(Artboard->GetStateMachineNames());
                break;
            }
        }
    }
    return Output;
}

void URiveActorComponent::InitializeAudioEngine()
{
    if (RiveAudioEngine == nullptr)
    {
        if (URiveAudioEngine* AudioEngine =
                GetOwner()->GetComponentByClass<URiveAudioEngine>())
        {
            RiveAudioEngine = AudioEngine;
        }
    }

    if (RiveAudioEngine != nullptr)
    {
        if (RiveAudioEngine->GetNativeAudioEngine() == nullptr)
        {
            if (AudioEngineLambdaHandle.IsValid())
            {
                RiveAudioEngine->OnRiveAudioReady.Remove(
                    AudioEngineLambdaHandle);
                AudioEngineLambdaHandle.Reset();
            }

            TFunction<void()> AudioLambda = [this]() {
                for (URiveArtboard* Artboard : Artboards)
                {
                    Artboard->SetAudioEngine(RiveAudioEngine);
                }
                RiveAudioEngine->OnRiveAudioReady.Remove(
                    AudioEngineLambdaHandle);
            };
            AudioEngineLambdaHandle =
                RiveAudioEngine->OnRiveAudioReady.AddLambda(AudioLambda);
        }
        else
        {
            for (URiveArtboard* Artboard : Artboards)
            {
                Artboard->SetAudioEngine(RiveAudioEngine);
            }
        }
    }
}

void URiveActorComponent::RiveReady(IRiveRenderer* InRiveRenderer)
{
    RiveTexture = NewObject<URiveTexture>();
    // Initialize Rive Render Target Only after we resize the texture
    RiveRenderTarget =
        InRiveRenderer->CreateTextureTarget_GameThread(GetFName(), RiveTexture);
    RiveRenderTarget->SetClearColor(FLinearColor::White);
    RiveTexture->ResizeRenderTargets(FIntPoint(Size.X, Size.Y));
    RiveRenderTarget->Initialize();

    RiveTexture->OnResourceInitializedOnRenderThread.AddUObject(
        this,
        &URiveActorComponent::OnResourceInitialized_RenderThread);

    if (DefaultRiveDescriptor.RiveFile)
    {
        URiveArtboard* Artboard =
            AddArtboard(DefaultRiveDescriptor.RiveFile,
                        DefaultRiveDescriptor.ArtboardName,
                        DefaultRiveDescriptor.StateMachineName);
        Artboard->OnArtboardTick_Render.BindDynamic(
            this,
            &URiveActorComponent::OnDefaultArtboardTickRender);
    }

    InitializeAudioEngine();

    OnRiveReady.Broadcast();
}
