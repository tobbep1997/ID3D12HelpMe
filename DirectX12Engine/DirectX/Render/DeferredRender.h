#pragma once
#include "Template/IRender.h"

class X12RenderTargetView;
class X12ConstantBuffer;

class DeferredRender : public IRender
{
private:
	static const UINT ROOT_PARAMETERS = 7;
	struct LightBuffer
	{
		DirectX::XMFLOAT4A	CameraPosition;
		DirectX::XMUINT4	Type[256];
		DirectX::XMFLOAT4A	Position[256];
		DirectX::XMFLOAT4A	Color[256];
		DirectX::XMFLOAT4A	Vector[256];
	};

	struct ShadowLightBuffer
	{
		DirectX::XMINT4 values;
		DirectX::XMFLOAT4X4A ViewProjection;
	};

	struct ShadowMap
	{
		ID3D12DescriptorHeap * Map;
		DirectX::XMFLOAT4X4A ViewProjection;
	};

public:
	DeferredRender(RenderingManager * renderingManager, const Window & window);
	~DeferredRender();


	HRESULT Init() override;
	void Update(const Camera& camera) override;
	void Draw() override;
	void Clear() override;
	void Release() override;

	void SetRenderTarget(X12RenderTargetView ** renderTarget, const UINT & size);
	void AddShadowMap(ID3D12DescriptorHeap * map, DirectX::XMFLOAT4X4A ViewProjection) const;

private:

	HRESULT _preInit();
	HRESULT _initID3D12RootSignature();
	HRESULT _initShaders();
	HRESULT _initID3D12PipelineState();
	HRESULT _createViewport();

	HRESULT _createQuadBuffer();
	HRESULT _createQuadIndexBuffer();

	D3D12_VERTEX_BUFFER_VIEW		m_vertexBufferView{};
	ID3D12Resource *				m_vertexBuffer = nullptr;
	ID3D12Resource *				m_vertexHeapBuffer = nullptr;

	D3D12_VIEWPORT	m_viewport{};
	D3D12_RECT		m_rect{};

	ID3D12PipelineState * m_pipelineState = nullptr;
	ID3D12RootSignature * m_rootSignature = nullptr;
	D3D12_ROOT_PARAMETER  m_rootParameters[ROOT_PARAMETERS]{};

	D3D12_SHADER_BYTECODE m_vertexShader{};
	D3D12_SHADER_BYTECODE m_pixelShader{};
	D3D12_INPUT_LAYOUT_DESC  m_inputLayoutDesc;

	UINT m_renderTargetSize = 0;
	X12RenderTargetView ** m_geometryRenderTargetView = nullptr;
	X12ConstantBuffer * m_lightBuffer = nullptr;
	X12ConstantBuffer * m_shadowBuffer = nullptr;

	LightBuffer m_lightValues{};
	ShadowLightBuffer m_shadowValues{};

	std::vector<ShadowMap*>* m_shadowMaps = nullptr;


	struct Vertex
	{
		Vertex()
		{
			this->Position = { 0,0,0,0 };
			this->Uv = { 0,0,0,0 };
		}
		Vertex(const DirectX::XMFLOAT4 & position, const DirectX::XMFLOAT4 & uv)
		{
			this->Position = position;
			this->Uv = uv;
		}
		DirectX::XMFLOAT4 Position;
		DirectX::XMFLOAT4 Uv;
	};
	UINT m_vertexBufferSize;

	Vertex m_vertexList[4];
};
