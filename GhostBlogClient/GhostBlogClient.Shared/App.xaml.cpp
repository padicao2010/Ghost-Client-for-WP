//
// App.xaml.cpp
// App 类的实现。
//

#include "pch.h"
#include "LoginPage.xaml.h"

using namespace GhostBlogClient;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml::Media::Animation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// “空白应用程序”模板在 http://go.microsoft.com/fwlink/?LinkId=234227 上有介绍

/// <summary>
/// 初始化单一实例应用程序对象。这是创作的代码的第一行
/// 逻辑上等同于 main() 或 WinMain()。
/// </summary>
App::App()
{
	InitializeComponent();
	Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);
}

/// <summary>
/// 在最终用户正常启动应用程序时调用。将在启动应用程序
/// 当启动应用程序以打开特定的文件或显示搜索结果等操作时，
/// 将使用其他入口点。
/// </summary>
/// <param name="e">有关启动请求和过程的详细信息。</param>
void App::OnLaunched(LaunchActivatedEventArgs^ e)
{
#if _DEBUG
	if (IsDebuggerPresent())
	{
		DebugSettings->EnableFrameRateCounter = true;
	}
#endif

	auto rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

	// 不要在窗口已包含内容时重复应用程序初始化，
	// 只需确保窗口处于活动状态。
	if (rootFrame == nullptr)
	{
		// 创建一个 Frame 以用作导航上下文并将其与
		// SuspensionManager 键关联
		rootFrame = ref new Frame();

		// TODO: 将此值更改为适合您的应用程序的缓存大小。
		rootFrame->CacheSize = 1;

		if (e->PreviousExecutionState == ApplicationExecutionState::Terminated)
		{
			// TODO: 仅当适用时还原保存的会话状态，并安排
			// 还原完成后的最终启动步骤。
		}

		// 将框架放在当前窗口中
		Window::Current->Content = rootFrame;
	}

	if (rootFrame->Content == nullptr)
	{
#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
		// 删除用于启动的旋转门导航。
		if (rootFrame->ContentTransitions != nullptr)
		{
			_transitions = ref new TransitionCollection();
			for (auto transition : rootFrame->ContentTransitions)
			{
				_transitions->Append(transition);
			}
		}

		rootFrame->ContentTransitions = nullptr;
		_firstNavigatedToken = rootFrame->Navigated += ref new NavigatedEventHandler(this, &App::RootFrame_FirstNavigated);
#endif

		// 当导航堆栈尚未还原时，导航到第一页，
		// 并通过将所需信息作为导航参数传入来配置
		// 新页面。
		if (!rootFrame->Navigate(LoginPage::typeid, e->Arguments))
		{
			throw ref new FailureException("Failed to create initial page");
		}
	}

	// 确保当前窗口处于活动状态
	Window::Current->Activate();
}

#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
/// <summary>
/// 启动应用程序后还原内容转换。
/// </summary>
void App::RootFrame_FirstNavigated(Object^ sender, NavigationEventArgs^ e)
{
	auto rootFrame = safe_cast<Frame^>(sender);

	TransitionCollection^ newTransitions;
	if (_transitions == nullptr)
	{
		newTransitions = ref new TransitionCollection();
		newTransitions->Append(ref new NavigationThemeTransition());
	}
	else
	{
		newTransitions = _transitions;
	}

	rootFrame->ContentTransitions = newTransitions;

	rootFrame->Navigated -= _firstNavigatedToken;
}
#endif

/// <summary>
/// 在将要挂起应用程序执行时调用。将保存应用程序状态
/// 将被终止还是恢复的情况下保存应用程序状态，
/// 并让内存内容保持不变。
/// </summary>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
	(void) sender;	// 未使用的参数
	(void) e;		// 未使用的参数

	// TODO: 保存应用程序状态并停止任何后台活动
}