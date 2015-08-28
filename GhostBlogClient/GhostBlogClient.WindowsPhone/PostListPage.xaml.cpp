//
// PostListPage.xaml.cpp
// Implementation of the PostListPage class
//

#include "pch.h"
#include "PostListPage.xaml.h"
#include "PostPage.xaml.h"
#include "PostShowPage.xaml.h"
#include "SettingPage.xaml.h"
#include "LoginPage.xaml.h"

#include "define.h"

using namespace GhostBlogClient;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Popups;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Storage;
using namespace concurrency;

// The Basic Page item template is documented at http://go.microsoft.com/fwlink/?LinkID=390556

PostListPage::PostListPage()
{
	InitializeComponent();
	SetValue(_defaultViewModelProperty, ref new Platform::Collections::Map<String^, Object^>(std::less<String^>()));
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
}

DependencyProperty^ PostListPage::_defaultViewModelProperty =
DependencyProperty::Register("DefaultViewModel",
TypeName(IObservableMap<String^, Object^>::typeid), TypeName(PostListPage::typeid), nullptr);

/// <summary>
/// Used as a trivial view model.
/// </summary>
IObservableMap<String^, Object^>^ PostListPage::DefaultViewModel::get()
{
	return safe_cast<IObservableMap<String^, Object^>^>(GetValue(_defaultViewModelProperty));
}

DependencyProperty^ PostListPage::_navigationHelperProperty =
DependencyProperty::Register("NavigationHelper",
TypeName(Common::NavigationHelper::typeid), TypeName(PostListPage::typeid), nullptr);

/// <summary>
/// Gets an implementation of <see cref="NavigationHelper"/> designed to be
/// used as a trivial view model.
/// </summary>
Common::NavigationHelper^ PostListPage::NavigationHelper::get()
{
	return safe_cast<Common::NavigationHelper^>(GetValue(_navigationHelperProperty));
}

#pragma region Navigation support

/// The methods provided in this section are simply used to allow
/// NavigationHelper to respond to the page's navigation methods.
/// 
/// Page specific logic should be placed in event handlers for the  
/// <see cref="NavigationHelper::LoadState"/>
/// and <see cref="NavigationHelper::SaveState"/>.
/// The navigation parameter is available in the LoadState method 
/// in addition to page state preserved during an earlier session.

void PostListPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedTo(e);
}

void PostListPage::OnNavigatedFrom(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedFrom(e);
}

#pragma endregion

/// <summary>
/// Populates the page with content passed during navigation. Any saved state is also
/// provided when recreating a page from a prior session.
/// </summary>
/// <param name="sender">
/// The source of the event; typically <see cref="NavigationHelper"/>
/// </param>
/// <param name="e">Event data that provides both the navigation parameter passed to
/// <see cref="Frame::Navigate(Type, Object)"/> when this page was initially requested and
/// a dictionary of state preserved by this page during an earlier
/// session. The state will be null the first time a page is visited.</param>
void PostListPage::LoadState(Object^ sender, Common::LoadStateEventArgs^ e)
{
	(void) sender;	// Unused parameter
	(void) e;	// Unused parameter

	if (e != nullptr && e->NavigationParameter != nullptr) {
		blogAll = safe_cast<BlogAll^>(e->NavigationParameter);
		ghostClient = blogAll->GhostClient;

		DefaultViewModel->Insert("User", blogAll->User);
		DefaultViewModel->Insert("Posts", blogAll->Posts);
		DefaultViewModel->Insert("Blog", blogAll->Blog);
		DefaultViewModel->Insert("Tags", blogAll->Tags);
		PostsFetchButton->IsEnabled = blogAll->FetchedPages < blogAll->NumPages;
		PostsHubPagination->Header = blogAll->GetPaginationInfo();
		//PostsWebView->NavigateToString(Blog->CurrentPost->Html);
		//PostsFetchButton->IsEnabled = Blog->FetchedPages < Blog->NumPages;
		//itemListView->SelectedIndex = 0;

		//if (Blog->NeedRefresh) {
		//	PostsRefreshButtonClick(nullptr, nullptr);
		//}
	}
}

/// <summary>
/// Preserves state associated with this page in case the application is suspended or the
/// page is discarded from the navigation cache.  Values must conform to the serialization
/// requirements of <see cref="SuspensionManager::SessionState"/>.
/// </summary>
/// <param name="sender">The source of the event; typically <see cref="NavigationHelper"/></param>
/// <param name="e">Event data that provides an empty dictionary to be populated with
/// serializable state.</param>
void PostListPage::SaveState(Object^ sender, Common::SaveStateEventArgs^ e){
	(void) sender;	// Unused parameter
	(void) e; // Unused parameter
}

bool PostListPage::CanGoBack()
{
	return NavigationHelper->CanGoBack();
}

void PostListPage::GoBack()
{
	// 清除 AccessToken 和 RefreshToken
	ApplicationDataContainer^ localSettings = ApplicationData::Current->LocalSettings;
	auto values = localSettings->Values;
	values->Remove("GhostAccessToken");
	values->Remove("GhostRefreshToken");

	NavigationHelper->GoBack();
}

void PostListPage::ItemView_ItemClick(Object^ sender, ItemClickEventArgs ^e)
{
	auto itemId = safe_cast<PostInfo^>(e->ClickedItem);
	blogAll->ShowPost = itemId;

	blogAll->AddNew = false;
	if (this->Frame != nullptr)
	{
		this->Frame->Navigate(TypeName(PostShowPage::typeid), blogAll);
	}
}

void PostListPage::PostsAddButtonClick(Object^ sender, RoutedEventArgs^ e)
{
	blogAll->AddNew = true;
	if (blogAll->EditPost == nullptr) {
		blogAll->EditPost = ref new PostInfo;
	}
	blogAll->EditPost->Title = "";
	blogAll->EditPost->Slug = "";
	blogAll->EditPost->Markdown = "";
	blogAll->EditPost->Status = "draft";
	if (blogAll->EditPost->Tags == nullptr) {
		blogAll->EditPost->Tags = ref new Vector<TagInfo^>;
	}
	else {
		blogAll->EditPost->Tags->Clear();
	}
	if (this->Frame != nullptr)
	{
		this->Frame->Navigate(TypeName(PostPage::typeid), blogAll);
	}
}

void PostListPage::SubmitGetPostsRequest(bool needRefresh, int pageIndex, int postsPerPage)
{
	StartWaiting();
	IAsyncOperation<Boolean>^ bOpe;
	if (needRefresh) {
		bOpe = ghostClient->RefreshAuthToken();
	}
	else {
		bOpe = blogAll->GetBooleanAsyncOperation();
	}
	create_task(bOpe)
		.then([=](Boolean resp)
	{
		return ghostClient->GetPosts(pageIndex, postsPerPage);
	}).then([this](PostsPageInfo^ page)
	{
		for (auto iter = page->Posts; iter->HasCurrent; iter->MoveNext()) {
			blogAll->FixPostTags(iter->Current);
			blogAll->AddPost(iter->Current, false);
		}
		blogAll->FetchedPages = page->PageIndex;
		blogAll->NumPages = page->PageCount;
		PostsFetchButton->IsEnabled = blogAll->FetchedPages < blogAll->NumPages;
		PostsHubPagination->Header = blogAll->GetPaginationInfo();

		this->FinishWaiting();
	}).then([=](task<void> prevTask) {
		try
		{
			prevTask.get();
		}
		catch (Exception ^ex)
		{
			if (ghostClient->ErrorMsg->ErrorKind == GHOST_AUTH_ERROR)
			{
				if (!needRefresh) {
					// RefreshToken 后再获取一次
					this->SubmitGetPostsRequest(true, pageIndex, postsPerPage);
					return;
				}
				else {
					// 需要重新登录
					this->FinishWaiting();
					this->NeedReLogin();
					return;
				}
			}
			String^ errMsg = ghostClient->ErrorMsg->GetErrorMessge(ex);
			auto dialog = ref new MessageDialog(errMsg, "操作失败");
			this->FinishWaiting();
			dialog->ShowAsync();
		}
	});
}

void PostListPage::PostsSettingButtonClick(Object^ sender, RoutedEventArgs^ e)
{
	if (this->Frame != nullptr)
	{
		this->Frame->Navigate(TypeName(SettingPage::typeid), blogAll);
	}
}

void PostListPage::PostsRefreshButtonClick(Object^ sender, RoutedEventArgs^ e)
{
	blogAll->FetchedPages = 0;
	blogAll->RemoveAllPosts();

	SubmitGetPostsRequest(false, 1, blogAll->PostsPerPage);
	blogAll->NeedRefresh = false;
}

void PostListPage::PostsFetchButtonClick(Object^ sender, RoutedEventArgs^ e)
{
	int num = blogAll->Posts->Size;
	int pageIndex = num / blogAll->PostsPerPage + 1;
	SubmitGetPostsRequest(false, pageIndex, blogAll->PostsPerPage);
}

void PostListPage::StartWaiting()
{
	PostsAddButton->IsEnabled = false;
	PostsRefreshButton->IsEnabled = false;
	PostsFetchButton->IsEnabled = false;
	PostsSettingButton->IsEnabled = false;
}

void PostListPage::FinishWaiting()
{
	PostsAddButton->IsEnabled = true;
	PostsRefreshButton->IsEnabled = true;
	PostsFetchButton->IsEnabled = blogAll->FetchedPages < blogAll->NumPages;
	PostsSettingButton->IsEnabled = true;
}

void PostListPage::NeedReLogin()
{
	auto dialog = ref new MessageDialog("操作失败，Token 过时，需要重新登陆");
	create_task(dialog->ShowAsync())
		.then([this](IUICommand^ resp)
	{
		blogAll->OnlyLogin = true;
		if (this->Frame != nullptr)
		{
			this->Frame->Navigate(TypeName(LoginPage::typeid), blogAll);
		}
	})
	.then([=](task<void> prevTask) {
		try
		{
			prevTask.get();
		}
		catch (Exception ^ex)
		{
		}
	});

}