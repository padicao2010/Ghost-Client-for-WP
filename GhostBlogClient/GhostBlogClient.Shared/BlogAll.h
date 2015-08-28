#pragma once

#include "GhostRest.h"

namespace GhostBlogClient {

	public ref class BlogAll sealed : Windows::UI::Xaml::Data::INotifyPropertyChanged
	{
	public:
		property BlogInfo^ Blog;
		property UserInfo^ User;
		property Windows::Foundation::Collections::IObservableVector<PostInfo^>^ Posts;
		property Windows::Foundation::Collections::IObservableVector<TagInfo^>^ Tags;
		property PostInfo^ ShowPost;
		property PostInfo^ EditPost;
		property GhostRest^ GhostClient;
		property int FetchedPages;
		property int PostsPerPage;
		property int NumPages;

		property Platform::Boolean AddNew;
		property Platform::Boolean NeedRefresh;
		property Platform::Boolean OnlyLogin;

		BlogAll();

		virtual event Windows::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

		Windows::Foundation::IAsyncOperation<Platform::Boolean>^ GetBooleanAsyncOperation() {
			return concurrency::create_async([] {
				return true;
			});
		}

		void OnPropertyChanged(Platform::String^ propertyName) {
			PropertyChanged(this, ref new Windows::UI::Xaml::Data::PropertyChangedEventArgs(propertyName));
		}

		Windows::Foundation::IAsyncOperation<BlogAll^>^ GetBlogAllInfo();

		void FixPostTags(PostInfo^ p);
		void FixPostsTags(Windows::Foundation::Collections::IIterator<PostInfo^>^ posts);
		Platform::String^ GetPaginationInfo();

		Platform::Boolean AddPost(PostInfo^ p, Platform::Boolean head);
		void DeletePost(PostInfo^ p);
		void RemoveAllPosts();

		Windows::Foundation::IAsyncOperation<Platform::String^>^ FormatPostToHtml(PostInfo^ post);
	private:

		static Platform::String^ htmlPrefix;
		static Platform::String^ htmlSuffix;
		Windows::Foundation::Collections::IMap<int, TagInfo^>^ TagsMap;
		Windows::Foundation::Collections::IMap<int, PostInfo^>^ PostsMap;
		void UpdateTagsMap();
	};
}

