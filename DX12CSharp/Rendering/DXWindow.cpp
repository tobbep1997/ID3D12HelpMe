#include "DXWindow.h"

namespace ID3D12
{
#include "../Converter.h"
	   
	DXWindow::DXWindow()
		: IManagedObject(Window::GetPointerInstance(), true)
	{

	}

	void DXWindow::Create(void* hInstance, String ^ windowName, unsigned width, unsigned height, bool fullScreen)
	{
		p_instance->Create(hInstance, std::string(Converter::SystemStringToCharArr(windowName)), width, height, fullScreen);
	}

	bool DXWindow::Updating()
	{
		return p_instance->Updating();
	}

	void DXWindow::Release()
	{
		p_instance->DeletePointerInstance();
	}
}
