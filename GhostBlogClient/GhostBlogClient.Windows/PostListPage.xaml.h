//
// PostListPage.xaml.h
// PostListPage 类的声明
//

#pragma once

#include "PostListPage.g.h"
#include "Common\NavigationHelper.h"
#include "GhostRest.h"
#include "BlogAll.h"

namespace GhostBlogClient
{
	/// <summary>
	/// 显示组标题、组内各项的列表以及当前选定项的
	/// 详细信息的页。
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class PostListPage sealed
	{
	public:
		PostListPage();

		property BlogAll^ Blog;

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
		void SaveState(Object^ sender, Common::SaveStateEventArgs^ e);
		bool CanGoBack();
		void GoBack();

		void PostsAddButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void PostsDeleteButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void PostsEditButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void PostsSettingsButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void PostsFetchButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void PostsRefreshButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		void SubmitDeleteRequest(bool needRefresh, int postId);
		void SubmitGetPostsRequest(bool needRefresh, int pageIndex, int postsPerPage);

		GhostRest^ ghostClient;

#pragma region Logical page navigation

		// 设计了拆分页，以便 Window 具有足够的空间同时显示
		// 列表和详细信息，一次将仅显示一个窗格。
		//
		// 这完全通过一个可表示两个逻辑页的单一物理页
		// 实现。    使用下面的代码可以实现此目标，且用户不会察觉到
		// 区别。

		void Window_SizeChanged(Platform::Object^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ e);
		void ItemListView_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e);
		bool UsingLogicalPageNavigation();
		void InvalidateVisualState();
		Platform::String^ DetermineVisualState();

#pragma endregion

		static Windows::UI::Xaml::DependencyProperty^ _defaultViewModelProperty;
		static Windows::UI::Xaml::DependencyProperty^ _navigationHelperProperty;
		static const int MinimumWidthForSupportingTwoPanes = 768;
	};
}
