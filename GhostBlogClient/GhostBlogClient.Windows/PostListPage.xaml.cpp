//
// PostListPage.xaml.cpp
// PostListPage 类的实现
//

#include "pch.h"
#include "PostListPage.xaml.h"
#include "PostPage.xaml.h"
#include "SettingPage.xaml.h"
#include "define.h"

using namespace GhostBlogClient;
using namespace GhostBlogClient::Common;

using namespace Platform;
using namespace Platform::Collections;
using namespace concurrency;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Storage;

// “拆分页”项模板在 http://go.microsoft.com/fwlink/?LinkId=234234 上提供

PostListPage::PostListPage()
{
	InitializeComponent();
	SetValue(_defaultViewModelProperty, ref new Map<String^,Object^>(std::less<String^>()));
	auto navigationHelper = ref new Common::NavigationHelper(this,
		ref new Common::RelayCommand(
		[this](Object^) -> bool
	{
		return CanGoBack();
	},
		[this](Object^) -> void
	{
		GoBack();
	}
	)
		);
	SetValue(_navigationHelperProperty, navigationHelper);
	navigationHelper->LoadState += ref new Common::LoadStateEventHandler(this, &PostListPage::LoadState);
	navigationHelper->SaveState += ref new Common::SaveStateEventHandler(this, &PostListPage::SaveState);

	itemListView->SelectionChanged += ref new SelectionChangedEventHandler(this, &PostListPage::ItemListView_SelectionChanged);
	Window::Current->SizeChanged += ref new WindowSizeChangedEventHandler (this, &PostListPage::Window_SizeChanged);
	InvalidateVisualState();
}

DependencyProperty^ PostListPage::_defaultViewModelProperty =
	DependencyProperty::Register("DefaultViewModel",
		TypeName(IObservableMap<String^,Object^>::typeid), TypeName(PostListPage::typeid), nullptr);

/// <summary>
/// 用作普通视图模型。
/// </summary>
IObservableMap<String^, Object^>^ PostListPage::DefaultViewModel::get()
{
	return safe_cast<IObservableMap<String^, Object^>^>(GetValue(_defaultViewModelProperty));
}

DependencyProperty^ PostListPage::_navigationHelperProperty =
	DependencyProperty::Register("NavigationHelper",
		TypeName(Common::NavigationHelper::typeid), TypeName(PostListPage::typeid), nullptr);

/// <summary>
/// 获取 <see cref="NavigationHelper"/> 的实现，该实现旨在
/// 用作普通视图模型。
/// </summary>
Common::NavigationHelper^ PostListPage::NavigationHelper::get()
{
	//	return _navigationHelper;
	return safe_cast<Common::NavigationHelper^>(GetValue(_navigationHelperProperty));
}

#pragma region Page state management

/// <summary>
/// 使用在导航过程中传递的内容填充页。  在从以前的会话
/// 重新创建页时，也会提供任何已保存状态。
/// </summary>
/// <param name="navigationParameter">最初请求此页时传递给
/// <see cref="Frame::Navigate(Type, Object)"/> 的参数值。
/// </param>
/// <param name="pageState">此页面在之前的会话期间保留的状态
/// 字典。首次访问页面时为 null。</param>
void PostListPage::LoadState(Platform::Object^ sender, Common::LoadStateEventArgs^ e)
{
	// TODO:  使用 DefaultViewModel->Insert("Group", <value>) 设置可绑定组
	// TODO:  使用 DefaultViewModel->Insert("Items", <value>) 设置可绑定项集合

	if (e != nullptr && e->NavigationParameter != nullptr) {
		Blog = safe_cast<BlogAll^>(e->NavigationParameter);
		ghostClient = Blog->GhostClient;

		DefaultViewModel->Insert("User", Blog->User);
		DefaultViewModel->Insert("Posts", Blog->Posts);
		DefaultViewModel->Insert("Blog", Blog->Blog);
		//PostsWebView->NavigateToString(Blog->CurrentPost->Html);
		PostsFetchButton->IsEnabled = Blog->FetchedPages < Blog->NumPages;
		itemListView->SelectedIndex = 0;

		if (Blog->NeedRefresh) {
			PostsRefreshButtonClick(nullptr, nullptr);
		}
	}

	if (e->PageState == nullptr)
	{
		// 当这是新页时，除非正在使用逻辑页导航，
		// 否则会自动选择第一项(请参见下面的逻辑页导航 #region。)
		if (!UsingLogicalPageNavigation() && itemsViewSource->View != nullptr)
		{
			itemsViewSource->View->MoveCurrentToFirst();
		}
	}
	else
	{
		// 还原与此页关联的以前保存的状态
		if (e->PageState->HasKey("SelectedItem") && itemsViewSource->View != nullptr)
		{
			// TODO:  使用选定项调用 itemsViewSource->View->MoveCurrentTo()，
			//       该选定项由值 pageState->Lookup("SelectedItem")指定
		}
	}
}

/// <summary>
/// 保留与此页关联的状态，以防挂起应用程序或
/// 从导航缓存中放弃此页。  值必须符合
/// <see cref="SuspensionManager::SessionState"/> 的序列化要求。
/// </summary>
///<param name="sender">事件的来源；通常为 <see cref="NavigationHelper"/></param>
///<param name="e">提供要使用可序列化状态填充的空字典
///的事件数据。</param>
void PostListPage::SaveState(Platform::Object^ sender, Common::SaveStateEventArgs^ e)
{
	if (itemsViewSource->View != nullptr)
	{
		auto selectedItem = itemsViewSource->View->CurrentItem;
		// TODO:  派生一个可序列化的导航参数，并将其传递给
		//       pageState->Insert("SelectedItem", <value>)
	}
}

#pragma endregion

#pragma region Logical page navigation

// 视觉状态管理通常直接反映四种应用程序视图状态(全
// 屏横向与纵向以及对齐和填充视图。)设计拆分页的目的在于使
// 对齐和纵向视图状态均有两个不同的子状态:  显示
// 项列表或详细信息之一，但不同时显示。
//
// 这完全通过一个可表示两个逻辑页的单一物理页实现。
// 使用下面的代码可以实现此目标，且用户不会察觉到区别。

/// <summary>
/// 在确定该页是应用作一个逻辑页还是两个逻辑页时进行调用。
/// </summary>
/// <returns>当前视图状态为纵向或对齐时为 true，否则为 false
/// 。</returns>
bool PostListPage::CanGoBack()
{
	if (UsingLogicalPageNavigation() && itemListView->SelectedItem != nullptr)
	{
		return true;
	}
	else
	{
		return NavigationHelper->CanGoBack();
	}
}

void PostListPage::GoBack()
{
	if (UsingLogicalPageNavigation() && itemListView->SelectedItem != nullptr)
	{
		// 如果逻辑页导航起作用且存在选定项，则当前将显示
		// 选定项的详细信息。    清除选择后将返回到
		// 项列表。    从用户的角度来看，这是一个逻辑后向
		// 导航。
		itemListView->SelectedItem = nullptr;
	}
	else
	{
		// 清除 AccessToken 和 RefreshToken
		ApplicationDataContainer^ localSettings = ApplicationData::Current->LocalSettings;
		auto values = localSettings->Values;
		values->Remove("GhostAccessToken");
		values->Remove("GhostRefreshToken");

		NavigationHelper->GoBack();
	}
}

/// <summary>
/// 在 Window 改变大小时调用
/// </summary>
/// <param name="sender">当前的 Window</param>
/// <param name="e">描述 Window 新大小的事件数据</param>
void PostListPage::Window_SizeChanged(Platform::Object^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ e)
{
	InvalidateVisualState();
}

/// <summary>
/// 在选择了列表中的某项后调用。
/// </summary>
/// <param name="sender">显示所选项的 GridView。</param>
/// <param name="e">描述如何更改了所选内容的事件数据。</param>
void PostListPage::ItemListView_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
	if (e->AddedItems->Size == 1) {
		Blog->ShowPost = safe_cast<PostInfo^>(e->AddedItems->GetAt(0));
		//PostsWebView->NavigateToString(Blog->CurrentPost->Html);
	}
	if (UsingLogicalPageNavigation())
	{
		InvalidateVisualState();
	}
}

/// <summary>
/// 在确定该页是应用作一个逻辑页还是两个逻辑页时进行调用。
/// </summary>
/// <returns>如果窗口应显示充当一个逻辑页，则为 True，
/// 。</returns>
bool PostListPage::UsingLogicalPageNavigation()
{
	return Windows::UI::Xaml::Window::Current->Bounds.Width < MinimumWidthForSupportingTwoPanes;
}

void PostListPage::InvalidateVisualState()
{
	auto visualState = DetermineVisualState();
	Windows::UI::Xaml::VisualStateManager::GoToState(this, visualState, false);
	NavigationHelper->GoBackCommand->RaiseCanExecuteChanged();
}

/// <summary>
/// 在确定对应于应用程序视图状态的可视状态的名称时进行
/// 视图状态。
/// </summary>
/// <returns>所需的可视状态的名称。  此名称与视图状态的名称相同，
/// 但在纵向和对齐视图中存在选定项时例外，在纵向和对齐视图中，
/// 此附加逻辑页通过添加 _Detail 后缀表示。</returns>
Platform::String^ PostListPage::DetermineVisualState()
{
	if (!UsingLogicalPageNavigation())
		return "PrimaryView";

	// 在视图状态更改时更新后退按钮的启用状态
	auto logicalPageBack = UsingLogicalPageNavigation() && itemListView->SelectedItem != nullptr;

	return logicalPageBack ? "SinglePane_Detail" : "SinglePane";
}

#pragma endregion

#pragma region Navigation support

/// 此部分中提供的方法只是用于使
/// NavigationHelper 可响应页面的导航方法。
/// 
/// 应将页面特有的逻辑放入用于
/// <see cref="NavigationHelper::LoadState"/>
/// 和 <see cref="NavigationHelper::SaveState"/> 的事件处理程序中。
/// 除了在会话期间保留的页面状态之外
/// LoadState 方法中还提供导航参数。

void PostListPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedTo(e);
}

void PostListPage::OnNavigatedFrom(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedFrom(e);

}
#pragma endregion

void PostListPage::PostsAddButtonClick(Object^ sender, RoutedEventArgs^ e)
{
	Blog->AddNew = true;
	if (this->Frame != nullptr)
	{
		this->Frame->Navigate(TypeName(PostPage::typeid), Blog);
	}
}

void PostListPage::PostsEditButtonClick(Object^ sender, RoutedEventArgs^ e)
{
	Blog->AddNew = false;
	if (this->Frame != nullptr)
	{
		this->Frame->Navigate(TypeName(PostPage::typeid), Blog);
	}
}

void PostListPage::SubmitDeleteRequest(bool needRefresh, int postId)
{
	IAsyncOperation<Boolean>^ bOpe;
	if (needRefresh) {
		bOpe = ghostClient->RefreshAuthToken();
	}
	else {
		bOpe = Blog->GetBooleanAsyncOperation();
	}
	create_task(bOpe)
		.then([=](Boolean resp)
	{
		return ghostClient->DeletePost(postId);
	}).then([this](PostInfo^ p)
	{
		unsigned int index = 0;
		auto posts = Blog->Posts;
		posts->IndexOf(Blog->ShowPost, &index);
		posts->RemoveAt(index);
		if (index > 0) {
			index--;
		}
		if (posts->Size == 0) {
			index = -1;
			Blog->ShowPost = nullptr;
		}
		else {
			Blog->ShowPost = posts->GetAt(index);
		}
		this->itemListView->SelectedIndex = index;
	}).then([=](task<void> prevTask) {
		try
		{
			prevTask.get();
		}
		catch (Exception ^ex)
		{
			if (ghostClient->ErrorMsg->ErrorKind == GHOST_AUTH_ERROR)
			{
				// RefreshToken 后再获取一次
				OutputDebugString((ghostClient->ErrorMsg->ToString() + "\n")->Data());
				if (!needRefresh) {
					this->SubmitDeleteRequest(true, postId);
				}
			}
			else if (ghostClient->ErrorMsg->ErrorKind == GHOST_OTHER_ERROR) {
				OutputDebugString((ghostClient->ErrorMsg->ToString() + "\n")->Data());
			}
			else {
				OutputDebugString((ex->ToString() + "\n")->Data());
			}
		}
	});
}
void PostListPage::SubmitGetPostsRequest(bool needRefresh, int pageIndex, int postsPerPage)
{
	IAsyncOperation<Boolean>^ bOpe;
	if (needRefresh) {
		bOpe = ghostClient->RefreshAuthToken();
	}
	else {
		bOpe = Blog->GetBooleanAsyncOperation();
	}
	create_task(bOpe)
		.then([=](Boolean resp)
	{
		return ghostClient->GetPosts(pageIndex, postsPerPage);
	}).then([this](PostsPageInfo^ page)
	{
		for (auto iter = page->Posts; iter->HasCurrent; iter->MoveNext()) {
			Blog->Posts->Append(iter->Current);
		}
		Blog->FetchedPages = page->PageIndex;
		Blog->NumPages = page->PageCount;
		PostsFetchButton->IsEnabled = Blog->FetchedPages < Blog->NumPages;
		if (Blog->ShowPost == nullptr) {
			Blog->ShowPost = Blog->Posts->GetAt(0);
			this->itemListView->SelectedIndex = 0;
		}
	}).then([=](task<void> prevTask) {
		try
		{
			prevTask.get();
		}
		catch (Exception ^ex)
		{
			if (ghostClient->ErrorMsg->ErrorKind == GHOST_AUTH_ERROR)
			{
				// RefreshToken 后再获取一次
				OutputDebugString((ghostClient->ErrorMsg->ToString() + "\n")->Data());
				if (!needRefresh) {
					this->SubmitGetPostsRequest(true, pageIndex, postsPerPage);
				}
			}
			else if (ghostClient->ErrorMsg->ErrorKind == GHOST_OTHER_ERROR) {
				OutputDebugString((ghostClient->ErrorMsg->ToString() + "\n")->Data());
			}
			else {
				OutputDebugString((ex->ToString() + "\n")->Data());
			}
		}
	});
}

void PostListPage::PostsDeleteButtonClick(Object^ sender, RoutedEventArgs^ e)
{
	SubmitDeleteRequest(false, Blog->ShowPost->Id);
}

void PostListPage::PostsSettingsButtonClick(Object^ sender, RoutedEventArgs^ e)
{
	if (this->Frame != nullptr)
	{
		this->Frame->Navigate(TypeName(SettingPage::typeid), Blog);
	}
}

void PostListPage::PostsRefreshButtonClick(Object^ sender, RoutedEventArgs^ e)
{
	Blog->FetchedPages = 0;
	Blog->Posts->Clear();
	Blog->ShowPost = nullptr;

	SubmitGetPostsRequest(false, 1, Blog->PostsPerPage);
	Blog->NeedRefresh = false;
}

void PostListPage::PostsFetchButtonClick(Object^ sender, RoutedEventArgs^ e)
{
	SubmitGetPostsRequest(false, Blog->FetchedPages + 1, Blog->PostsPerPage);
}
