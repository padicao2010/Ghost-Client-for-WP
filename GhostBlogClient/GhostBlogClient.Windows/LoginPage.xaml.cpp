//
// LoginPage.xaml.cpp
// LoginPage 类的实现
//

#include "pch.h"
#include "LoginPage.xaml.h"
#include "PostListPage.xaml.h"
#include "define.h"

using namespace GhostBlogClient;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Popups;
using namespace Windows::Storage;
using namespace concurrency;

// “基本页”项模板在 http://go.microsoft.com/fwlink/?LinkId=234237 上有介绍

LoginPage::LoginPage()
{
	InitializeComponent();
	SetValue(_defaultViewModelProperty, ref new Map<String^,Object^>(std::less<String^>()));
	auto navigationHelper = ref new Common::NavigationHelper(this);
	SetValue(_navigationHelperProperty, navigationHelper);
	navigationHelper->LoadState += ref new Common::LoadStateEventHandler(this, &LoginPage::LoadState);
	navigationHelper->SaveState += ref new Common::SaveStateEventHandler(this, &LoginPage::SaveState);

	ghostClient = ref new GhostRest;
}

DependencyProperty^ LoginPage::_defaultViewModelProperty =
	DependencyProperty::Register("DefaultViewModel",
		TypeName(IObservableMap<String^,Object^>::typeid), TypeName(LoginPage::typeid), nullptr);

/// <summary>
/// 用作普通视图模型。
/// </summary>
IObservableMap<String^, Object^>^ LoginPage::DefaultViewModel::get()
{
	return safe_cast<IObservableMap<String^, Object^>^>(GetValue(_defaultViewModelProperty));
}

DependencyProperty^ LoginPage::_navigationHelperProperty =
	DependencyProperty::Register("NavigationHelper",
		TypeName(Common::NavigationHelper::typeid), TypeName(LoginPage::typeid), nullptr);

/// <summary>
/// 获取 <see cref="NavigationHelper"/> 的实现，该实现旨在
/// 用作普通视图模型。
/// </summary>
Common::NavigationHelper^ LoginPage::NavigationHelper::get()
{
	return safe_cast<Common::NavigationHelper^>(GetValue(_navigationHelperProperty));
}

#pragma region Navigation support

/// 此部分中提供的方法只是用于使
/// NavigationHelper 可响应页面的导航方法。
/// 
/// 应将页面特有的逻辑放入用于
/// <see cref="NavigationHelper::LoadState"/>
/// 和 <see cref="NavigationHelper::SaveState"/> 的事件处理程序中。
/// 除了在会话期间保留的页面状态之外
/// LoadState 方法中还提供导航参数。

void LoginPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedTo(e);
}

void LoginPage::OnNavigatedFrom(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedFrom(e);
}

#pragma endregion

/// <summary>
///使用在导航过程中传递的内容填充页。 在从以前的会话
/// 重新创建页时，也会提供任何已保存状态。
/// </summary>
/// <param name="sender">
/// 事件的来源; 通常为 <see cref="NavigationHelper"/>
/// </param>
/// <param name="e">事件数据，其中既提供在最初请求此页时传递给
/// <see cref="Frame.Navigate(Type, Object)"/> 的导航参数，又提供
/// 此页在以前会话期间保留的状态的
/// 的字典。 首次访问页面时，该状态将为 null。</param>
void LoginPage::LoadState(Object^ sender, Common::LoadStateEventArgs^ e)
{
	(void) sender;	// 未使用的参数
	(void) e;	// 未使用的参数

	ApplicationDataContainer^ localSettings = ApplicationData::Current->LocalSettings;
	auto values = localSettings->Values;

	auto site = safe_cast<String^>(values->Lookup("GhostSite"));
	if (site == nullptr) {
		return;
	}
	LoginSiteInput->Text = site;
	auto email = safe_cast<String^>(values->Lookup("GhostEmail"));
	if (email == nullptr) {
		return;
	}
	LoginEmailInput->Text = email;
	auto accessToken = safe_cast<String^>(values->Lookup("GhostAccessToken"));
	auto refreshToken = safe_cast<String^>(values->Lookup("GhostRefreshToken"));
	if (accessToken == nullptr || refreshToken == nullptr) {
		return;
	}
	ghostClient->Site = site;
	ghostClient->AccessToken = accessToken;
	ghostClient->RefreshToken = refreshToken;

	// 开始登陆
	StartWaiting();
	LoginInfoText->Text = "已经有登陆Token，尝试使用";

	// 整个流程
	create_task(GetBlogAllInfo())
		.then([this](BlogAll^ blog)
	{
		LoginInfoText->Text = "信息获取成功";
		blog->NeedRefresh = false;
		if (this->Frame != nullptr)
		{
			this->Frame->Navigate(TypeName(PostListPage::typeid), blog);
		}
	}).then([this](task<void> prevTask) {
		try
		{
			prevTask.get();
			this->FinishWaiting();
		}
		catch (Exception ^ex)
		{
			if(ghostClient->ErrorMsg->ErrorKind == GHOST_AUTH_ERROR)
			{
				// RefreshToken 后再获取一次
				this->RefreshTokenAndGetBlog();
			}
			else if (ghostClient->ErrorMsg->ErrorKind == GHOST_OTHER_ERROR) {
				LoginInfoText->Text = "已有Token不能使用，请重新登录\n" + ghostClient->ErrorMsg->ToString();
				this->FinishWaiting();
			} else {
				LoginInfoText->Text = ex->ToString();
				this->FinishWaiting();
			}
		}
	});
}

/// <summary>
/// 保留与此页关联的状态，以防挂起应用程序或
/// 从导航缓存中放弃此页。  值必须符合
/// <see cref="SuspensionManager::SessionState"/> 的序列化要求。
/// </summary>
///<param name="sender">事件的来源；通常为 <see cref="NavigationHelper"/></param>
///<param name="e">提供要使用可序列化状态填充的空字典
///的事件数据。</param>
void LoginPage::SaveState(Object^ sender, Common::SaveStateEventArgs^ e){
	(void) sender;	// 未使用的参数
	(void) e; // 未使用的参数

	String^ site = LoginSiteInput->Text;
	String^ email = LoginEmailInput->Text;
	String^ accessToken = ghostClient->AccessToken;
	String^ refreshToken = ghostClient->RefreshToken;

	ApplicationDataContainer^ localSettings = ApplicationData::Current->LocalSettings;
	auto values = localSettings->Values;
	values->Clear();
	if (site->IsEmpty()) {
		return;
	}
	values->Insert("GhostSite", dynamic_cast<PropertyValue^>(PropertyValue::CreateString(site)));
	if (email->IsEmpty()) {
		return;
	}
	values->Insert("GhostEmail", dynamic_cast<PropertyValue^>(PropertyValue::CreateString(email)));
	if (accessToken == nullptr || accessToken->IsEmpty() ||
		refreshToken == nullptr || refreshToken->IsEmpty()) {
		return;
	}
	values->Insert("GhostAccessToken", dynamic_cast<PropertyValue^>(PropertyValue::CreateString(accessToken)));
	values->Insert("GhostRefreshToken", dynamic_cast<PropertyValue^>(PropertyValue::CreateString(refreshToken)));
}

void LoginPage::StartWaiting()
{
	LoginSiteInput->IsEnabled = false;
	LoginEmailInput->IsEnabled = false;
	LoginPasswordInput->IsEnabled = false;

	LoginLoginButton->IsEnabled = false;
}

void LoginPage::FinishWaiting()
{
	LoginSiteInput->IsEnabled = true;
	LoginEmailInput->IsEnabled = true;
	LoginPasswordInput->IsEnabled = true;

	LoginLoginButton->IsEnabled = true;
}

void LoginPage::LoginButtonClick(Object^ sender, RoutedEventArgs^ e) {
	String^ site = LoginSiteInput->Text;
	String^ email = LoginEmailInput->Text;
	String^ passwd = LoginPasswordInput->Password;

	if (site->IsEmpty()) {
		LoginInfoText->Text = "站点不能为空";
		return;
	}
	if (email->IsEmpty()) {
		LoginInfoText->Text = "邮箱不能为空";
		return;
	}
	if (passwd->IsEmpty()) {
		LoginInfoText->Text = "密码不能为空";
		return;
	}

	StartWaiting();
	LoginInfoText->Text = "登录中，请等待";
	
	create_task(ghostClient->Login(site, email, passwd))
		//create_task(restClient->GetUserMe())
		.then([=](Boolean status)
	{
		LoginInfoText->Text = "登录成功，开始获取博客信息";
		return this->GetBlogAllInfo();
	}).then([this](BlogAll^ blog) {
		LoginInfoText->Text = "获取成功，进入下一个页面";
		if (this->Frame != nullptr)
		{
			this->Frame->Navigate(TypeName(PostListPage::typeid), blog);
		}
	}).then([this](task<void> prevTask) {
		try
		{
			prevTask.get();
		}
		catch (Exception ^ex)
		{
			if (ghostClient->ErrorMsg->ErrorKind != GHOST_NONE_ERROR) {
				LoginInfoText->Text = "登录失败\n" + ghostClient->ErrorMsg->ToString();
			}
			else {
				LoginInfoText->Text = "登录失败\n" + ex->ToString();
			}
			OutputDebugString((ex->ToString() + "\n")->Data());
		}
		this->SaveState(nullptr, nullptr);
		this->FinishWaiting();
	});
}

IAsyncOperation<BlogAll^>^ LoginPage::GetBlogAllInfo()
{
	BlogAll^ blog = ref new BlogAll;
	
	return create_async([=] {
		return create_task(ghostClient->GetBlogInfo())
			.then([=](BlogInfo^ resp)
		{
			blog->Blog = resp;
			return ghostClient->GetUserMe();
		}).then([=](UserInfo^ resp)
		{
			blog->User = resp;
			return ghostClient->GetPosts(0, 10);
		}).then([=](PostsPageInfo^ resp)
		{
			blog->Posts = ref new Vector<PostInfo^>;
			for (auto iter = resp->Posts; iter->HasCurrent; iter->MoveNext())
			{
				blog->Posts->Append(iter->Current);
			}
			blog->PostsPerPage = resp->PostsPerPage;
			blog->NumPages = resp->PageCount;
			blog->FetchedPages = resp->PageIndex;

			blog->ShowPost = blog->Posts->GetAt(0);
			blog->GhostClient = ghostClient;
			return blog;
		});
	});
}

void LoginPage::RefreshTokenAndGetBlog()
{
	LoginInfoText->Text = "尝试刷新Token";
	create_task(ghostClient->RefreshAuthToken())
		.then([this](Boolean status)
	{
		LoginInfoText->Text = "刷新Token成功，获取博客信息";
		return this->GetBlogAllInfo();
	}).then([this](BlogAll^ blog)
	{
		LoginInfoText->Text = "信息获取成功";
		this->FinishWaiting();
		blog->NeedRefresh = false;
		if (this->Frame != nullptr)
		{
			this->Frame->Navigate(TypeName(PostListPage::typeid), blog);
		}
	}).then([this](task<void> prevTask) {
		try
		{
			prevTask.get();
			this->FinishWaiting();
		}
		catch (Exception ^ex)
		{
			if (ghostClient->ErrorMsg->ErrorKind == GHOST_AUTH_ERROR)
			{
				// RefreshToken 后再获取一次
				this->RefreshTokenAndGetBlog();
			}
			else if (ghostClient->ErrorMsg->ErrorKind == GHOST_OTHER_ERROR) {
				LoginInfoText->Text = "已有Token不能使用，请重新登录\n" + ghostClient->ErrorMsg->ToString();
				this->FinishWaiting();
			}
			else {
				LoginInfoText->Text = ex->ToString();
			}
		}
	});
}
