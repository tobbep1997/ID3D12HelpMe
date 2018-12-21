#pragma once
#include "Template/IRender.h"

class GeometryPass :
	public IRender
{
private:

	static const UINT BUFFER_SIZE = 1;

	struct CameraBuffer
	{
		CameraBuffer(const DirectX::XMFLOAT4 & cameraPosition = DirectX::XMFLOAT4(1,1,1,1))
		{
			this->CameraPosition = DirectX::XMFLOAT4A(cameraPosition.x, 
				cameraPosition.y,
				cameraPosition.z,
				cameraPosition.w);
		}
		DirectX::XMFLOAT4A CameraPosition;
	};
public:
	GeometryPass(RenderingManager * renderingManager, const Window & window);
	~GeometryPass();
	
	
	HRESULT Init() override;
	HRESULT Update() override;
	HRESULT Draw() override;
	HRESULT Release() override;

private:
	HRESULT _openCommandList() const;
	HRESULT _preInit();
	HRESULT _signalGPU();

	HRESULT _initID3D12RootSignature();
	HRESULT _initID3D12PipelineState();
	HRESULT _initShaders();
	HRESULT _createViewport();
	HRESULT _createDepthStencil();

	HRESULT _createVertexBuffer();
	HRESULT _createIndexBuffer();

	HRESULT _createConstantBuffer();
	

	ID3D12PipelineState * m_pipelineState = nullptr;
	ID3D12RootSignature * m_rootSignature = nullptr;

	D3D12_INPUT_LAYOUT_DESC  m_inputLayoutDesc;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW  m_indexBufferView;

	UINT				  m_vertexBufferSize = 0;
	ID3D12Resource		* m_vertexBuffer		= nullptr;
	ID3D12Resource		* m_vertexHeapBuffer	= nullptr;

	UINT				  m_indexBufferSize = 0;
	ID3D12Resource		* m_indexBuffer			= nullptr;
	ID3D12Resource		* m_indexHeapBuffer		= nullptr;

	ID3D12Resource		* m_depthStencilBuffer  = nullptr;
	ID3D12DescriptorHeap* m_depthStencilDescriptorHeap = nullptr;

	D3D12_VIEWPORT	m_viewport;
	D3D12_RECT		m_rect;

	D3D12_SHADER_BYTECODE m_vertexShader;
	D3D12_SHADER_BYTECODE m_pixelShader;

	D3D12_ROOT_PARAMETER  m_rootParameters[BUFFER_SIZE] {};

	ID3D12DescriptorHeap	* m_constantBufferDescriptorHeap[FRAME_BUFFER_COUNT] = { nullptr };
	ID3D12Resource			* m_constantBuffer[FRAME_BUFFER_COUNT] = { nullptr };

	CameraBuffer m_cameraBuffer {};

	UINT8* m_cameraBufferGPUAddress[FRAME_BUFFER_COUNT] = { nullptr };

	struct Vertex
	{
		Vertex(const DirectX::XMFLOAT4 & position = DirectX::XMFLOAT4(0,0,0,0), 
			const DirectX::XMFLOAT4 & color = DirectX::XMFLOAT4(0,0,0,0))
		{
			this->Position = DirectX::XMFLOAT4(position);
			this->Color = DirectX::XMFLOAT4(color);
		}
		DirectX::XMFLOAT4 Position;
		DirectX::XMFLOAT4 Color;
	};
};

