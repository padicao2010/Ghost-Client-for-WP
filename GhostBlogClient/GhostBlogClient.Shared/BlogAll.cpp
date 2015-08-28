#include "pch.h"
#include "BlogAll.h"

using namespace GhostBlogClient;
using namespace concurrency;
using namespace Windows::Foundation;
using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;
using namespace Platform;

String^ BlogAll::htmlPrefix = "<!doctype html><html><head><meta charset=\"utf-8\">"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, minimal-ui\">"
"<link rel=\"stylesheet\" href=\"ms-appx-web:///Markdown/github-markdown.min.css\">"
"<link rel=\"stylesheet\" href=\"ms-appx-web:///Markdown/highlight-default.min.css\">"
"<script src=\"ms-appx-web:///Markdown/highlight.min.js\"></script><script>hljs.initHighlightingOnLoad();</script>"
"<style>body{min-width:200px;max-width:790px;margin:0 auto;padding:30px}</style></head><body>"
"<article class=\"markdown-body\">";
String^ BlogAll::htmlSuffix = "</article></body></html>";


BlogAll::BlogAll()
{
	TagsMap = ref new Map<int, TagInfo^>;
	PostsMap = ref new Map<int, PostInfo^>;
}

IAsyncOperation<BlogAll^>^ BlogAll::GetBlogAllInfo()
{
	return create_async([=] {
		return create_task(GhostClient->GetBlogInfo())
			.then([this](BlogInfo^ resp)
		{
			this->Blog = resp;
			return GhostClient->GetUserMe();
		}).then([this](UserInfo^ resp)
		{
			this->User = resp;
			return GhostClient->GetAllTags();
		}).then([this](IObservableVector<TagInfo^>^ tags)
		{
			this->Tags = tags;
			UpdateTagsMap();
			return GhostClient->GetPosts(0, 10);
		}).then([this](PostsPageInfo^ resp)
		{
			this->Posts = ref new Vector<PostInfo^>;
			for (auto iter = resp->Posts; iter->HasCurrent; iter->MoveNext())
			{
				this->FixPostTags(iter->Current);
				this->AddPost(iter->Current, false);
			}
			this->PostsPerPage = resp->PostsPerPage;
			this->NumPages = resp->PageCount;
			this->FetchedPages = resp->PageIndex;
			return this;
		});
	});
}

void BlogAll::UpdateTagsMap()
{
	TagsMap->Clear();
	for (auto iter = Tags->First(); iter->HasCurrent; iter->MoveNext())
	{
		TagsMap->Insert(iter->Current->Id, iter->Current);
	}
}
void BlogAll::FixPostTags(PostInfo^ p)
{
	Vector<TagInfo^>^ newTags = ref new Vector<TagInfo^>;
	for (auto iter = p->Tags->First(); iter->HasCurrent; iter->MoveNext())
	{
		auto tag = TagsMap->Lookup(iter->Current->Id);
		newTags->Append(tag);
	}
	p->Tags = newTags;
}

void BlogAll::FixPostsTags(IIterator<PostInfo^>^ posts)
{
	for (auto iter = posts; iter->HasCurrent; iter->MoveNext())
	{
		FixPostTags(iter->Current);
	}
}

IAsyncOperation<String^>^ BlogAll::FormatPostToHtml(PostInfo^ post)
{
	return create_async([=] {
		// 标题
		String^ html = htmlPrefix + "<h1>" + post->Title + "</h1>";

		// 元数据信息
		html = html + "<hr />";
		// 链接
		html = html + "<p>Permanant link : <a href=\"" + GhostClient->Site + "/" + post->Slug + "\">" + post->Slug + "</a>";
		// 标签
		html = html + "<p>Tags : ";
		for (auto iter = post->Tags->First(); iter->HasCurrent; iter->MoveNext()) {
			html = html + "<a href=\"" + GhostClient->Site + "/tag/" + iter->Current->Slug + "\">" + iter->Current->Name + "</a>, ";
		}
		html = html + "</p>";
		// 状态
		html = html + "<p>State : " + post->Status + "</p>";
		html = html + "<hr />";

		// 正文
		html = html + post->Html + htmlSuffix;

		return html;
	});
}

String^ BlogAll::GetPaginationInfo()
{
	String^ result = "博文列表： 已下载 ";
	result = result + this->Posts->Size.ToString();
	result = result + "，" + this->FetchedPages.ToString() + "/" + this->NumPages.ToString() + " （x " + this->PostsPerPage.ToString() + "）";
	return result;
}

Boolean BlogAll::AddPost(PostInfo^ p, Boolean head)
{
	if (PostsMap->HasKey(p->Id)) {
		auto old = PostsMap->Lookup(p->Id);
		unsigned int index = 0;
		Posts->IndexOf(old, &index);
		Posts->RemoveAt(index);
		if (head) {
			Posts->InsertAt(0, p);
		}
		else {
			Posts->InsertAt(index, p);
		}
		PostsMap->Insert(p->Id, p);
		return true;
	}
	else {
		PostsMap->Insert(p->Id, p);
		if (head) {
			Posts->InsertAt(0, p);
		}
		else {
			Posts->Append(p);
		}
		return false;
	}
}

void BlogAll::DeletePost(PostInfo^ p)
{
	PostsMap->Remove(p->Id);
	unsigned int index = 0;
	Posts->IndexOf(p, &index);
	Posts->RemoveAt(index);
}

void BlogAll::RemoveAllPosts()
{
	PostsMap->Clear();
	Posts->Clear();
}