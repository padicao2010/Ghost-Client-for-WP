//
// PostListPage.xaml.h
// Declaration of the PostListPage class
//

#pragma once

#include "PostListPage.g.h"
#include "Common\NavigationHelper.h"
#include "BlogAll.h"

namespace GhostBlogClient
{
	/// <summary>
	/// A basic page that provides characteristics common to most applications.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class PostListPage sealed
	{
	public:
		PostListPage();

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
		bool CanGoBack();
		void GoBack();

		static Windows::UI::Xaml::DependencyProperty^ _defaultViewModelProperty;
		static Windows::UI::Xaml::DependencyProperty^ _navigationHelperProperty;

		void StartWaiting();
		void FinishWaiting();
		void NeedReLogin();
		void ItemView_ItemClick(Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs ^e);
		void PostsAddButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void PostsSettingButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void PostsFetchButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void PostsRefreshButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		void SubmitGetPostsRequest(bool needRefresh, int pageIndex, int postsPerPage);

		BlogAll^ blogAll;
		GhostRest^ ghostClient;
	};

}
