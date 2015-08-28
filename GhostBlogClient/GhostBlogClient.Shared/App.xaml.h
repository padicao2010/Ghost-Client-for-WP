//
// App.xaml.h
// App 类的声明。
//

#pragma once

#include "App.g.h"

namespace GhostBlogClient
{
	/// <summary>
	/// 提供特定于应用程序的行为，以补充默认的应用程序类。
	/// </summary>
	ref class App sealed
	{
	public:
		App();

		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e) override;

	private:
#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
		Windows::UI::Xaml::Media::Animation::TransitionCollection^ _transitions;
		Windows::Foundation::EventRegistrationToken _firstNavigatedToken;

		void RootFrame_FirstNavigated(Platform::Object^ sender, Windows::UI::Xaml::Navigation::NavigationEventArgs^ e);
#endif

		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
	};
}
