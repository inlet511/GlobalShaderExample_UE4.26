# GlobalShaderExample_UE4.26
UE4.26 Version of GlobalShaderCreattion

因为[这篇文章](https://zhuanlan.zhihu.com/p/36635394) 已经不适用于UE4.26

且[这篇文章](https://zhuanlan.zhihu.com/p/66514192) 中提到的[Repository](https://github.com/blueroseslol/BRPlugins) 中又有过多与创建GlobalShader不相干的东西，所以我在这里参考这两篇文章及代码做了修改，使其适用于UE4.26.

# GlobalShader 4.26 变动

## 整体参考
参考引擎自带插件LensDistortion即可

	Plugins/Compositing/LensDistortion


## 细节变动

### 去掉Serialize函数
彻底删除这个函数，不需要了

	virtual bool Serialize(FArchive& Ar) override  
		
### FGlobalShaderMap
FGlobalShaderMap 替代了原来的

TShareMap\<FGlobalShaderType\>
	
旧代码:

	TShaderMap<FGlobalShaderType>* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel); 
	
新代码:

	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(FeatureLevel);

### GETSAFERHISHADER_VERTEX
这个宏已经不存在了，直接使用XXXShader.GetXXXShader()函数代替
旧代码：

	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);

新代码：

	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();


### GetPixelShader

	SetShaderValue(RHICmdList, GetPixelShader(), SimpleColorVal, MyColor);
GetPixelShader()已经不是GlobalShader中的函数了，要替换为：

	RHICmdList.GetBoundPixelShader()


### SetRenderTarget

这个方法已经废弃，要用RHIBeginRenderPass和RHIEndRenderPass来代替
参考：LensDistortionRendering.cpp
旧代码：

	SetRenderTarget(
		RHICmdList,
		OutputRenderTargetResource->GetRenderTargetTexture(),
		FTextureRHIRef(),
		ESimpleRenderTargetMode::EUninitializedColorAndDepth,
		FExclusiveDepthStencil::DepthNop_StencilNop
	);
	//绘制代码……

新代码：

	// TODO
