//
// LoginPage.xaml.h
// LoginPage 类的声明
//

#pragma once

#include "LoginPage.g.h"
#include "Common\NavigationHelper.h"
#include "GhostRest.h"
#include "BlogAll.h"

namespace GhostBlogClient
{
	/// <summary>
	/// 基本页，提供大多数应用程序通用的特性。
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class LoginPage sealed
	{
	public:
		LoginPage();

		/// <summary>
		/// 可将其更改为强类型视图模型。
		/// </summary>
		property Windows::Foundation::Collections::IObservableMap<Platform::String^, Platform::Object^>^ DefaultViewModel
		{
			Windows::Foundation::Collections::IObservableMap<Platform::String^, Platform::Object^>^  get();
		}

		/// <summary>
		/// NavigationHelper 在每页上用于协助导航和
		/// 进程生命期管理
		/// </summary>
		property Common::NavigationHelper^ NavigationHelper
		{
			Common::NavigationHelper^ get();
		}

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
		virtual void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		void LoadState(Platform::Object^ sender, Common::LoadStateEventArgs^ e);
		void SaveState(Platform::Object^ sender, Common::SaveStateEventArgs^ e);

		static Windows::UI::Xaml::DependencyProperty^ _defaultViewModelProperty;
		static Windows::UI::Xaml::DependencyProperty^ _navigationHelperProperty;

		void LoginButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void StartWaiting();
		void FinishWaiting();

		Windows::Foundation::IAsyncOperation<BlogAll^>^ GetBlogAllInfo();
		void RefreshTokenAndGetBlog();

		GhostRest^ ghostClient;
	};
}
