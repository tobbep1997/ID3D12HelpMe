﻿#pragma once

class DirectionalLight : 
	public ILight
{
public:
	DirectionalLight(RenderingManager * renderingManager, const Window & window);
	~DirectionalLight();

	void Init() override;
	void Update() override;
	void Release() override;
	const UINT & GetNumRenderTargets() const override;

	Camera * GetCamera();

	void SetDirection(const DirectX::XMFLOAT4 & direction);
	void SetDirection(const float & x, const float & y, const float & z, const float & w = 0.0f);

	void SetPosition(const DirectX::XMFLOAT4& position) override;
	void SetPosition(const float& x, const float& y, const float& z, const float& w = 1.0f) override;

	const UINT& GetType() const override;
private:

	HRESULT _createDirectXContent();

	Camera * m_camera;
};
