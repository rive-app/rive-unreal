#pragma once

namespace rive
{
	class Artboard;
}

namespace UE::Rive::Renderer
{
	class IRiveRenderTarget : public TSharedFromThis<IRiveRenderTarget>
	{
	public:
		virtual ~IRiveRenderTarget() = default;

		virtual void Initialize() = 0;

		virtual void CacheTextureTarget_RenderThread(FRHICommandListImmediate& RHICmdList, const FTexture2DRHIRef& InRHIResource) = 0;

		virtual void AlignArtboard(uint8 Fit, float AlignX, float AlignY, rive::Artboard* InNativeArtBoard) = 0;
		virtual void DrawArtboard(uint8 Fit, float AlignX, float AlignY, rive::Artboard* InNativeArtBoard) = 0;

		virtual uint32 GetWidth() const = 0;
		virtual uint32 GetHeight() const = 0;
	};
}
