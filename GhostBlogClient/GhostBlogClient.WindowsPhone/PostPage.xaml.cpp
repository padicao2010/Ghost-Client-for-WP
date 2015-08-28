//
// PostPage.xaml.cpp
// Implementation of the PostPage class
//

#include "pch.h"
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

PostPage::PostPage()
{
	InitializeComponent();
	SetValue(_defaultViewModelProperty, ref new Platform::Collections::Map<String^, Object^>(std::less<String^>()));
	auto navigationHelper = ref new Common::NavigationHelper(this);
	SetValue(_navigationHelperProperty, navigationHelper);
	navigationHelper->LoadState += ref new Common::LoadStateEventHandler(this, &PostPage::LoadState);
	navigationHelper->SaveState += ref new Common::SaveStateEventHandler(this, &PostPage::SaveState);
}

DependencyProperty^ PostPage::_defaultViewModelProperty =
DependencyProperty::Register("DefaultViewModel",
TypeName(IObservableMap<String^, Object^>::typeid), TypeName(PostPage::typeid), nullptr);

/// <summary>
/// Used as a trivial view model.
/// </summary>
IObservableMap<String^, Object^>^ PostPage::DefaultViewModel::get()
{
	return safe_cast<IObservableMap<String^, Object^>^>(GetValue(_defaultViewModelProperty));
}

DependencyProperty^ PostPage::_navigationHelperProperty =
DependencyProperty::Register("NavigationHelper",
TypeName(Common::NavigationHelper::typeid), TypeName(PostPage::typeid), nullptr);

/// <summary>
/// Gets an implementation of <see cref="NavigationHelper"/> designed to be
/// used as a trivial view model.
/// </summary>
Common::NavigationHelper^ PostPage::NavigationHelper::get()
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

void PostPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedTo(e);
}

void PostPage::OnNavigatedFrom(NavigationEventArgs^ e)
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
void PostPage::LoadState(Object^ sender, Common::LoadStateEventArgs^ e)
{
	(void) sender;	// Unused parameter
	(void) e;	// Unused parameter

	if (e != nullptr && e->NavigationParameter != nullptr) {
		blogAll = safe_cast<BlogAll^>(e->NavigationParameter);
		ghostClient = blogAll->GhostClient;

		currentPost = blogAll->EditPost;

		if (blogAll->AddNew) {
			PostHeaderText->Text = "Ghost 博客客户端 -- 添加博文";
		}
		else {
			PostHeaderText->Text = "Ghost 博客客户端 -- 编辑博文";
		}

		DefaultViewModel->Insert("CurrentPost", currentPost);
		DefaultViewModel->Insert("Tags", blogAll->Tags);

		for (auto iter = currentPost->Tags->First(); iter->HasCurrent; iter->MoveNext())
		{
			PostTagsInput->SelectedItems->Append(iter->Current);
		}

		PostPublishInput->IsChecked = currentPost->Status->Equals("published");
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
void PostPage::SaveState(Object^ sender, Common::SaveStateEventArgs^ e){
	(void) sender;	// Unused parameter
	(void) e; // Unused parameter
}

void PostPage::SubmitRequest(boolean needRefresh, PostInfo^ cur) {
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
		IAsyncOperation<PostInfo^>^ pOpe;
		if (blogAll->AddNew) {
			pOpe = ghostClient->AddPost(cur);
		}
		else {
			pOpe = ghostClient->UpdatePost(cur);
		}
		return pOpe;
	}).then([this](PostInfo^ p)
	{
		blogAll->FixPostTags(p);
		if (blogAll->AddNew) {
			blogAll->AddPost(p, true);
		}
		else {
			blogAll->AddPost(p, true);
			blogAll->ShowPost = p;
		}
		
		auto dialog = ref new MessageDialog("操作成功");
		return dialog->ShowAsync();
	}).then([=](IUICommand^ resp) {
		this->FinishWaiting();
		if (blogAll->AddNew) {
			NavigationHelper->GoBack();
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
				if (!needRefresh) {
					SubmitRequest(true, currentPost);
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
			auto dialog = ref new MessageDialog(errMsg, "操作失败");
			dialog->ShowAsync();
		}
	});
}

void PostPage::PostSubmitButtonClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	currentPost->Markdown = PostMarkdownInput->Text;
	currentPost->Slug = PostSlugInput->Text;
	currentPost->Title = PostTitleInput->Text;
	if (PostPublishInput->IsChecked->Equals(true)) {
		currentPost->Status = "published";
	}
	else {
		currentPost->Status = "draft";
	}
	currentPost->Tags->Clear();
	for (auto iter = PostTagsInput->SelectedItems->First(); iter->HasCurrent; iter->MoveNext())
	{
		TagInfo^ tag = safe_cast<TagInfo^>(iter->Current);
		currentPost->Tags->Append(tag);
	}

	SubmitRequest(false, currentPost);
}

void PostPage::StartWaiting()
{
	PostTitleInput->IsEnabled = false;
	PostMarkdownInput->IsEnabled = false;
	PostSlugInput->IsEnabled = false;
	PostPublishInput->IsEnabled = false;
	PostSubmitButton->IsEnabled = false;
}

void PostPage::FinishWaiting()
{
	PostTitleInput->IsEnabled = true;
	PostMarkdownInput->IsEnabled = true;
	PostSlugInput->IsEnabled = true;
	PostPublishInput->IsEnabled = true;
	PostSubmitButton->IsEnabled = true;
}

void PostPage::NeedReLogin()
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

void PostPage::TextBox_GotFocus(Object^ sender, RoutedEventArgs^ e)
{
	TextBox^ box = safe_cast<TextBox^>(sender);
	box->Foreground = this->Background;
}

void PostPage::TextBox_LostFocus(Object^ sender, RoutedEventArgs^ e)
{
	TextBox^ box = safe_cast<TextBox^>(sender);
	box->Foreground = this->Foreground;
}