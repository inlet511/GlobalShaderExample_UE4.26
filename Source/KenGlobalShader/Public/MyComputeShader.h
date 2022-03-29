#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GlobalShader.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "SceneUtils.h"
#include "SceneInterface.h"
#include "ShaderParameterUtils.h"
#include "Logging/MessageLog.h"
#include "Internationalization/Internationalization.h"
#include "RenderTargetPool.h"


#define LOCTEXT_NAMESPACE "MyComputeShader"

class FCheckerBoardComputeShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCheckerBoardComputeShader, Global)

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{	}

	FCheckerBoardComputeShader() {}

	FCheckerBoardComputeShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		OutputSurface.Bind(Initializer.ParameterMap, TEXT("OutputSurface"));
	}

	void SetParameters(
		FRHICommandList& RHICmdList,
		FTexture2DRHIRef& InOutputSurfaceValue,
		FUnorderedAccessViewRHIRef& UAV
	)
	{
		FRHIComputeShader* ShaderRHI = RHICmdList.GetBoundComputeShader();

		//RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EComputeToCompute, UAV);
		OutputSurface.SetTexture(RHICmdList, ShaderRHI, InOutputSurfaceValue, UAV);
	}

	void UnsetParameters(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef& UAV)
	{
		//RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToCompute, UAV);
		//OutputSurface.UnsetUAV(RHICmdList, RHICmdList.GetBoundComputeShader());
	}

private:

	LAYOUT_FIELD(FRWShaderParameter, OutputSurface);
};
IMPLEMENT_SHADER_TYPE(, FCheckerBoardComputeShader, TEXT("/GlobalShaderPlugin/CheckerBoard.usf"), TEXT("MainCS"), SF_Compute);

#undef LOCTEXT_NAMESPACE