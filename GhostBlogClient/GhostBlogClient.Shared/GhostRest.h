#pragma once

namespace GhostBlogClient {

	public ref class ErrorMessage sealed
	{
	public:
		property int ErrorKind;
		//property Platform::String^ ErrorType;
		property Platform::String^ Message;

		Platform::String^ ToString() override;

		Platform::String^ GetErrorMessge(Platform::Exception^ ex);
	};

	[Windows::UI::Xaml::Data::Bindable]
	public ref class BlogInfo sealed
	{
	public:
		property Platform::String^ Title;
		property Platform::String^ Description;

		void ValueOf(Windows::Data::Json::JsonObject^ json);
		void SetData(BlogInfo^ b);
	};

	[Windows::UI::Xaml::Data::Bindable]
	public ref class TagInfo sealed
	{
	public:
		property int Id;
		property Platform::String^ Name;
		property Platform::String^ Slug;

		void ValueOf(Windows::Data::Json::JsonObject^ json);
		Windows::Data::Json::JsonObject^ GetJsonForUpdate();

		static Windows::Data::Json::JsonArray^ GetJsonArrayForTags(Windows::Foundation::Collections::IIterator<TagInfo^>^ tags);
		static Windows::Foundation::Collections::IObservableVector<TagInfo^>^ ValueOfJsonArray(Windows::Data::Json::JsonArray^ json);
	};

	[Windows::UI::Xaml::Data::Bindable]
	public ref class PostInfo sealed
	{
	public:
		property int Id;
		property Platform::String^ Title;
		property Platform::String^ Slug;
		property Platform::String^ Markdown;
		property Platform::String^ Html;
		property Platform::String^ Status;
		property Windows::Foundation::Collections::IObservableVector<TagInfo^>^ Tags;

		void ValueOf(Windows::Data::Json::JsonObject^ json);
		Windows::Data::Json::JsonObject^ ToJsonObject(boolean add);
		void SetData(PostInfo^ b);
	};

	[Windows::UI::Xaml::Data::Bindable]
	public ref class UserInfo sealed
	{
	public:
		property int Id;
		property Platform::String^ Name;
		property Platform::String^ Slug;
		property Platform::String^ Email;

		void ValueOf(Windows::Data::Json::JsonObject^ json);
		void SetData(UserInfo^ b);
	};

	public ref class PostsPageInfo sealed
	{
	public:
		property Windows::Foundation::Collections::IIterator<PostInfo^>^ Posts;
		property int PageCount;
		property int PageIndex;
		property int PostsPerPage;

		void ValueOf(Windows::Data::Json::JsonObject^ json);
	};

	public ref class GhostRest sealed
	{
	public:
		GhostRest();
		property Platform::String^ Site;
		property Platform::String^ Email;
		property Platform::String^ AccessToken{
			Platform::String^ get() { return m_AccessToken; }
			void set(Platform::String^ s) {
				if (s == nullptr) {
					m_AccessToken = "";
				}
				else {
					m_AccessToken = s;
				}
				client->DefaultRequestHeaders->Authorization = 
						ref new Windows::Web::Http::Headers::HttpCredentialsHeaderValue("Bearer", m_AccessToken);
			}
		};
		property Platform::String^ RefreshToken;

		property ErrorMessage^ ErrorMsg;

		// 登陆，返回 true 表示登陆成功
		// 返回false， 表示登陆失败，密码错误或邮箱不存在
		// 抛出异常，表示联网失败
		Windows::Foundation::IAsyncOperation<Platform::Boolean>^ Login(
			Platform::String^ site,
			Platform::String^ email,
			Platform::String^ passwd);

		Windows::Foundation::IAsyncOperation<Platform::Boolean>^ RefreshAuthToken();

		Windows::Foundation::IAsyncOperation<UserInfo^>^ GetUserMe();

		Windows::Foundation::IAsyncOperation<PostsPageInfo^>^ GetPosts(int pageIndex, int postsPerPage);

		Windows::Foundation::IAsyncOperation<PostInfo^>^ GetPost(int postid);

		Windows::Foundation::IAsyncOperation<PostInfo^>^ GetPostBySlug(Platform::String^ slug);

		Windows::Foundation::IAsyncOperation<PostInfo^>^ UpdatePost(PostInfo^ post);

		Windows::Foundation::IAsyncOperation<PostInfo^>^ DeletePost(int postid);

		Windows::Foundation::IAsyncOperation<PostInfo^>^ AddPost(PostInfo^ post);

		Windows::Foundation::IAsyncOperation<BlogInfo^>^ GetBlogInfo();

		Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IObservableVector<TagInfo^>^>^ GetAllTags();
	private:
		Windows::Web::Http::HttpClient^ client;

		void CheckStatusCode(Windows::Web::Http::HttpStatusCode status);
		void CheckResponseJson(Windows::Data::Json::JsonObject^ json);

		Platform::String^ m_AccessToken;
	};
}
