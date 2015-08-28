//
// PostShowPage.xaml.cpp
// Implementation of the PostShowPage class
//

#include "pch.h"
#include "PostShowPage.xaml.h"
#include "PostPage.xaml.h"
#include "LoginPage.xaml.h"
#include "define.h"

using namespace GhostBlogClient;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Popups;
using namespace concurrency;

// The Basic Page item template is documented at http://go.microsoft.com/fwlink/?LinkID=390556

PostShowPage::PostShowPage()
{
	InitializeComponent();
	SetValue(_defaultViewModelProperty, ref new Platform::Collections::Map<String^, Object^>(std::less<String^>()));
	auto navigationHelper = ref new Common::NavigationHelper(this);
	SetValue(_navigationHelperProperty, navigationHelper);
	navigationHelper->LoadState += ref new Common::LoadStateEventHandler(this, &PostShowPage::LoadState);
	navigationHelper->SaveState += ref new Common::SaveStateEventHandler(this, &PostShowPage::SaveState);
}

DependencyProperty^ PostShowPage::_defaultViewModelProperty =
DependencyProperty::Register("DefaultViewModel",
TypeName(IObservableMap<String^, Object^>::typeid), TypeName(PostShowPage::typeid), nullptr);

/// <summary>
/// Used as a trivial view model.
/// </summary>
IObservableMap<String^, Object^>^ PostShowPage::DefaultViewModel::get()
{
	return safe_cast<IObservableMap<String^, Object^>^>(GetValue(_defaultViewModelProperty));
}

DependencyProperty^ PostShowPage::_navigationHelperProperty =
DependencyProperty::Register("NavigationHelper",
TypeName(Common::NavigationHelper::typeid), TypeName(PostShowPage::typeid), nullptr);

/// <summary>
/// Gets an implementation of <see cref="NavigationHelper"/> designed to be
/// used as a trivial view model.
/// </summary>
Common::NavigationHelper^ PostShowPage::NavigationHelper::get()
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

void PostShowPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedTo(e);
}

void PostShowPage::OnNavigatedFrom(NavigationEventArgs^ e)
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
void PostShowPage::LoadState(Object^ sender, Common::LoadStateEventArgs^ e)
{
	(void) sender;	// Unused parameter
	(void) e;	// Unused parameter

	if (e != nullptr && e->NavigationParameter != nullptr) {
		blogAll = safe_cast<BlogAll^>(e->NavigationParameter);
		ghostClient = blogAll->GhostClient;
		currentPost = blogAll->ShowPost;

		StartWaiting();
		create_task(blogAll->FormatPostToHtml(currentPost))
			.then([this](String^ html)
		{
			PostShowWebView->NavigateToString(html);
			FinishWaiting();
		});
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
void PostShowPage::SaveState(Object^ sender, Common::SaveStateEventArgs^ e){
	(void) sender;	// Unused parameter
	(void) e; // Unused parameter
}

void PostShowPage::StartWaiting()
{
	PostShowCommandBar->IsEnabled = false;
}

void PostShowPage::FinishWaiting()
{
	PostShowCommandBar->IsEnabled = true;
}

void PostShowPage::PostShowDeleteButtonClick(Object^ sender, RoutedEventArgs^ e)
{
	SubmitDeleteRequest(false, currentPost->Id);
}


void PostShowPage::SubmitDeleteRequest(bool needRefresh, int postId)
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
		return ghostClient->DeletePost(postId);
	}).then([this](PostInfo^ p)
	{
		blogAll->DeletePost(currentPost);
		auto dialog = ref new MessageDialog("删除成功");
		return dialog->ShowAsync();
	}).then([=](IUICommand^ resp) {
		this->FinishWaiting();
		NavigationHelper->GoBack();
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
					this->SubmitDeleteRequest(true, postId);
					return;
				}
				else {
					// 需要重新登录
					this->FinishWaiting();
					this->NeedReLogin();
					return;
				}
			}

			this->FinishWaiting();
			String^ errMsg = ghostClient->ErrorMsg->GetErrorMessge(ex);
			auto dialog = ref new MessageDialog(errMsg, "删除失败");
			dialog->ShowAsync();
		}
	});
}

void PostShowPage::PostShowEditButtonClick(Object^ sender, RoutedEventArgs^ e)
{
	if (blogAll->EditPost == nullptr) {
		blogAll->EditPost = ref new PostInfo;
	}
	blogAll->EditPost->SetData(blogAll->ShowPost);

	this->Frame->Navigate(TypeName(PostPage::typeid), blogAll);
}

void PostShowPage::SubmitRefreshRequest(bool needRefresh, int postId)
{
	IAsyncOperation<Boolean>^ bOpe;
	if (!needRefresh) {
		StartWaiting();
		bOpe = blogAll->GetBooleanAsyncOperation();
	}
	else {
		bOpe = ghostClient->RefreshAuthToken();
	}
	create_task(bOpe)
		.then([=](Boolean resp)
	{
		return ghostClient->GetPost(postId);
	}).then([this](PostInfo^ p)
	{
		blogAll->AddPost(p, false);
		blogAll->ShowPost = p;
		currentPost = p;
		return blogAll->FormatPostToHtml(p);
	}).then([this](String^ html)
	{
		PostShowWebView->NavigateToString(html);
		FinishWaiting();
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
					this->SubmitDeleteRequest(true, postId);
					return;
				}
				else {
					// 需要重新登录
					this->FinishWaiting();
					this->NeedReLogin();
					return;
				}
			}

			this->FinishWaiting();
			String^ errMsg = ghostClient->ErrorMsg->GetErrorMessge(ex);
			auto dialog = ref new MessageDialog(errMsg, "刷新失败");
			dialog->ShowAsync();
		}
	});
}

void PostShowPage::PostShowRefreshButtonClick(Object^ sender, RoutedEventArgs^ e)
{
	SubmitRefreshRequest(false, currentPost->Id);
}

void PostShowPage::NeedReLogin()
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
	}).then([=](task<void> prevTask) {
		try
		{
			prevTask.get();
		}
		catch (Exception ^ex)
		{
		}
	});

}