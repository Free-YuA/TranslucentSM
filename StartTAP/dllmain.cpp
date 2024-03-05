// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Windows::UI::Xaml::Media;

DWORD dwRes = 0, dwSize = sizeof(DWORD), dwOpacity = 0, dwLuminosity = 0, dwHide = 0;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#pragma region Helpers
template <typename T>
T convert_from_abi(com_ptr<::IInspectable> from)
{
	T to{ nullptr }; // `T` is a projected type.

	winrt::check_hresult(from->QueryInterface(winrt::guid_of<T>(),
		winrt::put_abi(to)));

	return to;
}

DependencyObject FindDescendantByName(DependencyObject root, hstring name)
{
	if (root == nullptr)
	{
		return nullptr;
	}

	int count = VisualTreeHelper::GetChildrenCount(root);
	for (int i = 0; i < count; i++)
	{
		DependencyObject child = VisualTreeHelper::GetChild(root, i);
		if (child == nullptr)
		{
			continue;
		}

		hstring childName = child.GetValue(FrameworkElement::NameProperty()).as<hstring>();
		if (childName == name)
		{
			return child;
		}

		DependencyObject result = FindDescendantByName(child, name);
		if (result != nullptr)
		{
			return result;
		}
	}

	return nullptr;
}
#pragma endregion

struct StartTAP : winrt::implements<StartTAP, IObjectWithSite>
{
	HRESULT STDMETHODCALLTYPE SetSite(IUnknown* pUnkSite) noexcept override
	{
		site.copy_from(pUnkSite);
		com_ptr<IXamlDiagnostics> diag = site.as<IXamlDiagnostics>();
		com_ptr<::IInspectable> dispatcherPtr;
		diag->GetDispatcher(dispatcherPtr.put());
		CoreDispatcher dispatcher = convert_from_abi<CoreDispatcher>(dispatcherPtr);
		RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"TintOpacity", RRF_RT_DWORD, NULL, &dwOpacity, &dwSize);
		RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"TintLuminosityOpacity", RRF_RT_DWORD, NULL, &dwLuminosity, &dwSize);
		RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"HideSearch", RRF_RT_DWORD, NULL, &dwHide, &dwSize);

		dispatcher.RunAsync(CoreDispatcherPriority::Normal, []()
			{
				// get the current window view
				auto content = Window::Current().Content();

				// Search for AcrylicBorder name
				static auto acrylicBorder = FindDescendantByName(content, L"AcrylicBorder").as<Border>();
				static auto srchBox = FindDescendantByName(content, L"StartMenuSearchBox").as<Control>();

				// apply the things
				if (acrylicBorder != nullptr)
				{
					if (dwOpacity > 100) dwOpacity = 100;
					if (dwLuminosity > 100) dwLuminosity = 100;
					acrylicBorder.Background().as<AcrylicBrush>().TintOpacity(double(dwOpacity) / 100);
					acrylicBorder.Background().as<AcrylicBrush>().TintLuminosityOpacity(double(dwLuminosity) / 100);
					if (dwHide == 1 && srchBox != nullptr)
					{
						srchBox.Visibility(Visibility::Collapsed);
					}
				}

				Button bt;
				auto f = FontIcon();
				f.Glyph(L"\uE104");
				f.FontFamily(Media::FontFamily(L"Segoe Fluent Icons"));
				f.FontSize(16);
				bt.BorderThickness(Thickness{ 0 });
				bt.Background(SolidColorBrush(Colors::Transparent()));
				auto stackPanel = StackPanel();

				TextBlock tbx;
				tbx.FontFamily(Windows::UI::Xaml::Media::FontFamily(L"Segoe UI Variable"));
				tbx.FontSize(13.0);
				winrt::hstring hs = L"TintOpacity";
				tbx.Text(hs);
				stackPanel.Children().Append(tbx);

				Slider slider;
				slider.Width(140);
				slider.Value(acrylicBorder.Background().as<AcrylicBrush>().TintOpacity() * 100);
				stackPanel.Children().Append(slider);

				TextBlock tbx1;
				tbx1.FontFamily(Windows::UI::Xaml::Media::FontFamily(L"Segoe UI Variable"));
				tbx1.FontSize(13.0);
				winrt::hstring hs1 = L"TintLuminosityOpacity";
				tbx1.Text(hs1);
				stackPanel.Children().Append(tbx1);

				Slider slider2;
				slider2.Width(140);
				slider2.Value(acrylicBorder.Background().as<AcrylicBrush>().TintLuminosityOpacity().Value() * 100);
				stackPanel.Children().Append(slider2);

				static HKEY subKey = nullptr;
				DWORD dwSize = sizeof(DWORD), dwInstalled = 0;
				RegCreateKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\TranslucentSM", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &subKey, NULL);

				slider.ValueChanged([](IInspectable const& sender, RoutedEventArgs const&) {
					auto sliderControl = sender.as<Slider>();
					double sliderValue = sliderControl.Value();
					DWORD val = sliderValue;
					acrylicBorder.Background().as<AcrylicBrush>().TintOpacity(double(sliderValue) / 100);
					RegSetValueEx(subKey, TEXT("TintOpacity"), 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
					});

				slider2.ValueChanged([](IInspectable const& sender, RoutedEventArgs const&) {
					auto sliderControl = sender.as<Slider>();
					double sliderValue = sliderControl.Value();
					DWORD val = sliderValue;
					acrylicBorder.Background().as<AcrylicBrush>().TintLuminosityOpacity(double(sliderValue) / 100);
					RegSetValueEx(subKey, TEXT("TintLuminosityOpacity"), 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
					});

				// add only on w11
				if (srchBox != nullptr)
				{
					auto checkBox = CheckBox();
					checkBox.Content(box_value(L"Hide search box"));
					stackPanel.Children().Append(checkBox);
					if (dwHide == 1)
					{
						checkBox.IsChecked(true);
					}
					checkBox.Checked([](IInspectable const& sender, RoutedEventArgs const&) {
						DWORD ser = 1;
						RegSetValueEx(subKey, TEXT("HideSearch"), 0, REG_DWORD, (const BYTE*)&ser, sizeof(ser));
						srchBox.Visibility(Visibility::Collapsed);
						});

					checkBox.Unchecked([](IInspectable const& sender, RoutedEventArgs const&) {
						DWORD ser = 0;
						RegSetValueEx(subKey, TEXT("HideSearch"), 0, REG_DWORD, (const BYTE*)&ser, sizeof(ser));
						srchBox.Visibility(Visibility::Visible);
						});
				}

				auto nvds = FindDescendantByName(content, L"RootPanel");
				auto nvpane = FindDescendantByName(content, L"NavigationPanePlacesListView");
				auto rootpanel = FindDescendantByName(nvpane, L"Root");
				if (rootpanel != nullptr)
				{
					auto grid = VisualTreeHelper::GetChild(rootpanel, 0).as<Grid>();
					if (grid != nullptr)
					{
						ToolTipService::SetToolTip(bt, box_value(L"TranslucentSM settings"));

						bt.Content(winrt::box_value(f));
						bt.Margin({ -40,0,0,0 });
						bt.Padding({ 11.2,11.2,11.2,11.2 });
						bt.Width(38);
						bt.BorderBrush(SolidColorBrush(Colors::Transparent()));
						grid.Children().Append(bt);


						Flyout flyout;
						flyout.Content(stackPanel);
						bt.Flyout(flyout);
					}
				}
				else if (nvds != nullptr)
				{
					auto nvGrid = nvds.as<Grid>();

					// w10 start menu
					RevealBorderBrush rv;
					rv.TargetTheme(ApplicationTheme::Dark);

					RevealBorderBrush rvb;
					rvb.TargetTheme(ApplicationTheme::Dark);
					rvb.Color(ColorHelper::FromArgb(128, 255, 255, 255));
					rvb.Opacity(0.4);
					
					// doesnt seem to work on windows 11
					bt.Resources().Insert(winrt::box_value(L"ButtonBackgroundPointerOver"), rvb);
					bt.Resources().Insert(winrt::box_value(L"ButtonBackgroundPressed"), rvb);

					f.HorizontalAlignment(HorizontalAlignment::Left);
					f.Margin({ 11.2,11.2,11.2,11.2 });

					auto mainpanel = StackPanel();
					mainpanel.Margin({ 0,-94,0,0 });
					mainpanel.Height(45);
					mainpanel.BorderThickness({ 1,1,1,1 });
					mainpanel.BorderBrush(rv);
					Grid::SetRow(mainpanel, 3);
					Canvas::SetZIndex(mainpanel, 10);
					mainpanel.Children().Append(bt);

					auto textBlock = TextBlock();
					textBlock.Text(L"TranslucentSM settings");
					textBlock.Margin({ 5,13,0,0 });
					textBlock.HorizontalAlignment(HorizontalAlignment::Left);

					auto panel = StackPanel();
					panel.Margin({ -4,-4,0,0 });
					panel.Children().Append(f);
					panel.Children().Append(textBlock);
					panel.Orientation(Orientation::Horizontal);
					panel.Height(45);
					panel.Width(256);
					panel.HorizontalAlignment(HorizontalAlignment::Left);

					bt.Height(45);
					bt.Width(256);
					bt.Content(panel);

					slider.Width(230);
					slider2.Width(230);

					// flyout
					Flyout flyout;
					flyout.Content(stackPanel);
					bt.Flyout(flyout);

					nvGrid.Children().InsertAt(0, mainpanel);
				}

				// for search idk
				static auto appBorder = FindDescendantByName(content, L"AppBorder").as<Border>();
				if (appBorder != nullptr)
				{
					/// aaaa need a better way
					appBorder.Background().as<AcrylicBrush>().TintOpacity(double(dwOpacity) / 100);
					appBorder.Background().as<AcrylicBrush>().TintLuminosityOpacity(double(dwLuminosity) / 100);
				}

			});
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void** ppvSite) noexcept override
	{
		return site.as(riid, ppvSite);
	}

private:
	winrt::com_ptr<IUnknown> site;
};
struct TAPFactory : winrt::implements<TAPFactory, IClassFactory>
{
	HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) override try
	{
		*ppvObject = nullptr;

		if (pUnkOuter)
		{
			return CLASS_E_NOAGGREGATION;
		}

		return winrt::make<StartTAP>().as(riid, ppvObject);
	}
	catch (...)
	{
		return winrt::to_hresult();
	}

	HRESULT STDMETHODCALLTYPE LockServer(BOOL) noexcept override
	{
		return S_OK;
	}
};
_Use_decl_annotations_ STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) try
{
	*ppv = nullptr;

	// TapFactory
	// {36162BD3-3531-4131-9B8B-7FB1A991EF51}
	static constexpr GUID tapFactory =
	{ 0x36162bd3, 0x3531, 0x4131, { 0x9b, 0x8b, 0x7f, 0xb1, 0xa9, 0x91, 0xef, 0x51 } };

	// {F3454DD1-B68F-4196-8571-2260F107A47B}
	static const GUID shellExt =
	{ 0xf3454dd1, 0xb68f, 0x4196, { 0x85, 0x71, 0x22, 0x60, 0xf1, 0x7, 0xa4, 0x7b } };

	if (rclsid == tapFactory)
	{
		return winrt::make<TAPFactory>().as(riid, ppv);
	}
	else if (rclsid == shellExt)
	{

	}
	else
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}
}
catch (...)
{
	return winrt::to_hresult();
}
_Use_decl_annotations_ STDAPI DllCanUnloadNow(void)
{
	if (winrt::get_module_lock())
	{
		return S_FALSE;
	}
	else
	{
		return S_OK;
	}
}
