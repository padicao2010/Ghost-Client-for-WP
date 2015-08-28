//
// PostPage.xaml.h
// Declaration of the PostPage class
//

#pragma once

#include "PostPage.g.h"
#include "Common\NavigationHelper.h"
#include "BlogAll.h"

namespace GhostBlogClient
{
	/// <summary>
	/// A basic page that provides characteristics common to most applications.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class PostPage sealed
	{
	public:
		PostPage();

		/// <summary>
		/// Gets the view model for this <see cref="Page"/>. 
		/// This can be changed to a strongly typed view model.
		/// </summary>
		property Windows::Foundation::Collections::IObservableMap<Platform::String^, Platform::Object^>^ DefaultViewModel
		{
			Windows::Foundation::Collections::IObservableMap<Platform::String^, Platform::Object^>^  get();
		}

		/// <summary>
		/// Gets the <see cref="NavigationHelper"/> associated with this <see cref="Page"/>.
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

		void StartWaiting();
		void FinishWaiting();
		void NeedReLogin();
		void SubmitRequest(boolean hasRefresh, PostInfo^ p);

		void PostSubmitButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void TextBox_GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void TextBox_LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		PostInfo^ currentPost;
		GhostRest^ ghostClient;
		BlogAll^ blogAll;
	};

}
