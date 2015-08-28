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
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace concurrency;
using namespace Windows::Storage;

// “基本页”项模板在 http://go.microsoft.com/fwlink/?LinkID=390556 上有介绍

LoginPage::LoginPage()
{
	InitializeComponent();
	SetValue(_defaultViewModelProperty, ref new Platform::Collections::Map<String^, Object^>(std::less<String^>()));
	auto navigationHelper = ref new Common::NavigationHelper(this);
	SetValue(_navigationHelperProperty, navigationHelper);
	navigationHelper->LoadState += ref new Common::LoadStateEventHandler(this, &LoginPage::LoadState);
	navigationHelper->SaveState += ref new Common::SaveStateEventHandler(this, &LoginPage::SaveState);
}

DependencyProperty^ LoginPage::_defaultViewModelProperty =
DependencyProperty::Register("DefaultViewModel",
TypeName(IObservableMap<String^, Object^>::typeid), TypeName(LoginPage::typeid), nullptr);

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
/// 使用在导航过程中传递的内容填充页。在从以前的会话
/// 重新创建页时，也会提供任何已保存状态。
/// </summary>
/// <param name="sender">
/// 事件的来源; 通常为 <see cref="NavigationHelper"/>
/// </param>
/// <param name="e">事件数据，其中既提供在最初请求此页时传递给
/// <see cref="Frame::Navigate(Type, Object)"/> 的导航参数，又提供
/// 此页在以前会话期间保留的状态的
/// 的字典。首次访问页面时，该状态将为 null。</param>
void LoginPage::LoadState(Object^ sender, Common::LoadStateEventArgs^ e)
{
	(void) sender;	// 未使用的参数
	(void) e;	// 未使用的参数

	if (e != nullptr && e->NavigationParameter != nullptr) {
		// 由 PostListPage 导航进来时
		blogAll = safe_cast<BlogAll^>(e->NavigationParameter);
		ghostClient = blogAll->GhostClient;
		LoginSiteInput->IsEnabled = false;
		LoginEmailInput->IsEnabled = false;
		LoginSiteInput->Text = ghostClient->Site;
		LoginEmailInput->Text = ghostClient->Email;
	}
	else {
		ghostClient = ref new GhostRest;
		blogAll = ref new BlogAll;
		blogAll->GhostClient = ghostClient;
		LoadSettings();
		LoginSiteInput->Text = ghostClient->Site;
		LoginEmailInput->Text = ghostClient->Email;
		if (!ghostClient->Site->IsEmpty() && !ghostClient->AccessToken->IsEmpty() &&
			!ghostClient->RefreshToken->IsEmpty()) {
			GetBlogAllInfoWithToken(false);
		}
	}
}

/// <summary>
/// 保留与此页关联的状态，以防挂起应用程序或
/// 从导航缓存中放弃此页。值必须符合
/// <see cref="SuspensionManager::SessionState"/> 的序列化要求。
/// </summary>
/// <param name="sender">事件的来源；通常为 <see cref="NavigationHelper"/></param>
/// <param name="e">提供要使用可序列化状态填充的空字典
/// 的事件数据。</param>
void LoginPage::SaveState(Object^ sender, Common::SaveStateEventArgs^ e){
	(void) sender;	// 未使用的参数
	(void) e; // 未使用的参数

	SaveSettings();

	if (blogAll->OnlyLogin) {
		blogAll->OnlyLogin = false;
	}
}

void LoginPage::LoadSettings()
{
	ApplicationDataContainer^ localSettings = ApplicationData::Current->LocalSettings;
	auto values = localSettings->Values;

	auto site = safe_cast<String^>(values->Lookup("GhostSite"));
	if (site == nullptr) {
		site = "";
	}
	auto email = safe_cast<String^>(values->Lookup("GhostEmail"));
	if (email == nullptr) {
		email = "";
	}
	auto accessToken = safe_cast<String^>(values->Lookup("GhostAccessToken"));
	auto refreshToken = safe_cast<String^>(values->Lookup("GhostRefreshToken"));
	if (accessToken == nullptr) {
		accessToken = "";
	}
	if (refreshToken == nullptr) {
		refreshToken = "";
	}

	ghostClient->Site = site;
	ghostClient->Email = email;
	ghostClient->AccessToken = accessToken;
	ghostClient->RefreshToken = refreshToken;
}

void LoginPage::SaveSettings()
{
	String^ site = ghostClient->Site;
	String^ email = ghostClient->Email;
	String^ accessToken = ghostClient->AccessToken;
	String^ refreshToken = ghostClient->RefreshToken;

	ApplicationDataContainer^ localSettings = ApplicationData::Current->LocalSettings;
	auto values = localSettings->Values;
	values->Insert("GhostSite", dynamic_cast<PropertyValue^>(PropertyValue::CreateString(site)));
	values->Insert("GhostEmail", dynamic_cast<PropertyValue^>(PropertyValue::CreateString(email)));
	values->Insert("GhostAccessToken", dynamic_cast<PropertyValue^>(PropertyValue::CreateString(accessToken)));
	values->Insert("GhostRefreshToken", dynamic_cast<PropertyValue^>(PropertyValue::CreateString(refreshToken)));
}

void LoginPage::StartWaiting()
{
	if (!blogAll->OnlyLogin) {
		LoginSiteInput->IsEnabled = false;
		LoginEmailInput->IsEnabled = false;
	}
	LoginPasswordInput->IsEnabled = false;
	LoginLoginButton->IsEnabled = false;
}

void LoginPage::FinishWaiting()
{
	if (!blogAll->OnlyLogin) {
		LoginSiteInput->IsEnabled = true;
		LoginEmailInput->IsEnabled = true;
	}
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

	if (blogAll->OnlyLogin) {
		Login(site, email, passwd);
	}
	else {
		LoginAndGetAllBlogInfo(site, email, passwd);
	}
}

void LoginPage::Login(String^ site, String^ email, String^ passwd)
{
	StartWaiting();
	LoginInfoText->Text = "登录中，请等待";

	create_task(ghostClient->Login(site, email, passwd))
		//create_task(restClient->GetUserMe())
		.then([=](Boolean status)
	{
		LoginInfoText->Text = "登录成功，返回上一个页面，重新操作";
		blogAll->OnlyLogin = false;
		this->SaveSettings();
		this->FinishWaiting();

		this->NavigationHelper->GoBack();
	}).then([this](task<void> prevTask) {
		try
		{
			prevTask.get();
		}
		catch (Exception ^ex)
		{
			LoginInfoText->Text = "登录失败\n" + ghostClient->ErrorMsg->GetErrorMessge(ex);
			this->SaveSettings();
			this->FinishWaiting();
		}
	});
}

void LoginPage::LoginAndGetAllBlogInfo(String^ site, String^ email, String^ passwd)
{
	StartWaiting();
	LoginInfoText->Text = "登录中，请等待";

	create_task(ghostClient->Login(site, email, passwd))
		//create_task(restClient->GetUserMe())
		.then([=](Boolean status)
	{
		LoginInfoText->Text = "登录成功，开始获取博客信息";
		return blogAll->GetBlogAllInfo();
	}).then([this](BlogAll^ blog) {
		LoginInfoText->Text = "获取成功，进入下一个页面";

		this->FinishWaiting();
		this->SaveSettings();

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
			LoginInfoText->Text = "登录失败\n" + ghostClient->ErrorMsg->GetErrorMessge(ex);
			this->FinishWaiting();
			this->SaveSettings();
		}
	});
}

void LoginPage::GetBlogAllInfoWithToken(Boolean refreshToken)
{
	StartWaiting();
	IAsyncOperation<Boolean>^ bOpe;
	if (refreshToken) {
		bOpe = ghostClient->RefreshAuthToken();
		LoginInfoText->Text = "尝试使用现有 Token";
	}
	else {
		bOpe = blogAll->GetBooleanAsyncOperation();
		LoginInfoText->Text = "尝试刷新 Token";
	}

	create_task(bOpe)
		.then([this](Boolean status)
	{
		LoginInfoText->Text = "使用 Token 获取博客信息";
		return blogAll->GetBlogAllInfo();
	}).then([this](BlogAll^ blog)
	{
		LoginInfoText->Text = "博客信息获取成功";
		this->FinishWaiting();
		blog->NeedRefresh = false;

		this->FinishWaiting();
		this->SaveSettings();

		if (this->Frame != nullptr)
		{
			this->Frame->Navigate(TypeName(PostListPage::typeid), blog);
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
				if (!refreshToken) {
					// RefreshToken 后再获取一次
					this->GetBlogAllInfoWithToken(true);
					return;
				}
			}
			LoginInfoText->Text = ghostClient->ErrorMsg->GetErrorMessge(ex);
			this->SaveSettings();
			this->FinishWaiting();
		}
	});
}