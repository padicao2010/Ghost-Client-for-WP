#include "pch.h"
#include "GhostRest.h"
#include "define.h"

using namespace GhostBlogClient;

using namespace Windows::Web::Http;
using namespace Windows::Web;
using namespace Windows::Data::Json;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Platform;
using namespace Platform::Collections;
using namespace concurrency;


String^ ErrorMessage::ToString() 
{
	return Message;
}

String^ ErrorMessage::GetErrorMessge(Exception^ ex)
{
	if (ErrorKind == GHOST_NONE_ERROR) {
		return ex->ToString();
	}
	else {
		return ToString();
	}
}

void BlogInfo::ValueOf(JsonObject^ json) {
	JsonArray^ setArray = json->GetNamedArray("settings");
	for (auto iter = setArray->First(); iter->HasCurrent; iter->MoveNext()) {
		JsonObject^ setObj = iter->Current->GetObject();
		String^ key = setObj->GetNamedString("key");
		if (key->Equals("title")) {
			Title = setObj->GetNamedString("value");
		}
		else if (key->Equals("description")) {
			Description = setObj->GetNamedString("value");
		}
	}
}

void PostsPageInfo::ValueOf(JsonObject^ json) {
	auto ps = ref new Vector<PostInfo^>;
	JsonArray^ postArray = json->GetNamedArray("posts");
	for (auto iter = postArray->First(); iter->HasCurrent; iter->MoveNext()) {
		PostInfo^ post = ref new PostInfo;
		JsonObject^ postJson = iter->Current->GetObject();
		post->ValueOf(postJson);
		ps->Append(post);
	}
	Posts = ps->First();

	auto pageJson = json->GetNamedObject("meta")->GetNamedObject("pagination");
	PageIndex = (int)pageJson->GetNamedNumber("page");
	PageCount = (int)pageJson->GetNamedNumber("pages");
	PostsPerPage = (int)pageJson->GetNamedNumber("limit");
}

void BlogInfo::SetData(BlogInfo^ b) {
	Title = b->Title;
	Description = b->Description;
}

void UserInfo::SetData(UserInfo^ u) {
	Id = u->Id;
	Name = u->Name;
	Slug = u->Slug;
	Email = u->Email;
}

void UserInfo::ValueOf(JsonObject^ json) {
	Id = (int)json->GetNamedNumber("id");
	Name = json->GetNamedString("name");
	Slug = json->GetNamedString("slug");
	Email = json->GetNamedString("email");
}

void PostInfo::ValueOf(JsonObject^ json) {
	Id = json->GetNamedNumber("id");
	Title = json->GetNamedString("title");
	Slug = json->GetNamedString("slug");
	Markdown = json->GetNamedString("markdown");
	Html = json->GetNamedString("html");
	Status = json->GetNamedString("status");

	Tags = TagInfo::ValueOfJsonArray(json->GetNamedArray("tags"));
}

void PostInfo::SetData(PostInfo^ p) {
	Id = p->Id;
	Title = p->Title;
	Slug = p->Slug;
	Markdown = p->Markdown;
	Html = p ->Html;
	Status = p->Status;

	if (Tags == nullptr) {
		Tags = ref new Vector<TagInfo^>();
	}
	else {
		Tags->Clear();
	}
	for (auto iter = p->Tags->First(); iter->HasCurrent; iter->MoveNext()) {
		Tags->Append(iter->Current);
	}
}

JsonObject^ PostInfo::ToJsonObject(boolean add) {
	JsonObject^ postJson = ref new JsonObject;
	if (!add) {
		postJson->Insert("id", JsonValue::CreateNumberValue(Id));
	}
	postJson->Insert("slug", JsonValue::CreateStringValue(Slug));
	postJson->Insert("title", JsonValue::CreateStringValue(Title));
	postJson->Insert("markdown", JsonValue::CreateStringValue(Markdown));
	postJson->Insert("status", JsonValue::CreateStringValue(Status));
	postJson->Insert("tags", TagInfo::GetJsonArrayForTags(Tags->First()));
	JsonArray^ arrayJson = ref new JsonArray;
	arrayJson->Append(postJson);
	JsonObject^ postsJson = ref new JsonObject;
	postsJson->Insert("posts", arrayJson);
	

	return postsJson;
}

GhostRest::GhostRest()
{
	client = ref new HttpClient();
	Site = nullptr;
	AccessToken = nullptr;
	RefreshToken = nullptr;
	ErrorMsg = ref new ErrorMessage;
}

IAsyncOperation<Boolean>^ GhostRest::Login(String ^ site, String ^ email, String ^ passwd)
{
	Site = site;
	Email = email;

	Uri^ siteUri = ref new Uri(site + GHOST_API_PREFIX + AUTH_TOKEN);
	auto params = ref new Map<String^, String^>;
	params->Insert("grant_type", "password");
	params->Insert("username", email);
	params->Insert("password", passwd);
	params->Insert("client_id", "ghost-admin");
	HttpFormUrlEncodedContent^ content = ref new HttpFormUrlEncodedContent(params);

	ErrorMsg->ErrorKind = GHOST_NONE_ERROR;
	return create_async([=] {
		return create_task(client->PostAsync(siteUri, content))
			.then([this](HttpResponseMessage^ response)
		{
			CheckStatusCode(response->StatusCode);
			return response->Content->ReadAsStringAsync();
		}).then([this](String^ response)
		{
			OutputDebugString((response + "\n")->Data());

			JsonObject^ jsonResp = JsonObject::Parse(response);
			CheckResponseJson(jsonResp);

			AccessToken = jsonResp->GetNamedString("access_token");
			RefreshToken = jsonResp->GetNamedString("refresh_token");
			if (AccessToken == nullptr || RefreshToken == nullptr) {
				return false;
			}
			OutputDebugString((AccessToken + "\n")->Data());
			OutputDebugString((RefreshToken + "\n")->Data());

			return true;
		});
	});
}

IAsyncOperation<UserInfo^>^ GhostRest::GetUserMe()
{
	Uri^ uri = ref new Uri(Site + GHOST_API_PREFIX + GET_USERS_ME);
	client->DefaultRequestHeaders->Authorization = ref new Headers::HttpCredentialsHeaderValue("Bearer", AccessToken);

	ErrorMsg->ErrorKind = GHOST_NONE_ERROR;
	return create_async([=] {
		return create_task(client->GetAsync(uri))
			.then([this](HttpResponseMessage^ response) 
		{
			CheckStatusCode(response->StatusCode);
			return response->Content->ReadAsStringAsync();
		}) .then([this](String^ response)
		{
			OutputDebugString((response + "\n")->Data());
			JsonObject^ jsonResp = JsonObject::Parse(response);
			CheckResponseJson(jsonResp);

			UserInfo^ user = ref new UserInfo;
			JsonObject^ userJson = jsonResp->GetNamedArray("users")->GetObjectAt(0);
			user->ValueOf(userJson);
			return user;
		});
	});
}

IAsyncOperation<Boolean>^ GhostRest::RefreshAuthToken()
{
	Uri^ siteUri = ref new Uri(Site + GHOST_API_PREFIX + AUTH_TOKEN);
	auto params = ref new Map<String^, String^>;
	params->Insert("grant_type", "refresh_token");
	params->Insert("refresh_token", RefreshToken);
	params->Insert("client_id", "ghost-admin");
	HttpFormUrlEncodedContent^ content = ref new HttpFormUrlEncodedContent(params);

	ErrorMsg->ErrorKind = GHOST_NONE_ERROR;

	return create_async([=] {
		
		return create_task(client->PostAsync(siteUri, content))
			.then([this](HttpResponseMessage^ response)
		{
			CheckStatusCode(response->StatusCode);
			return response->Content->ReadAsStringAsync();
		}).then([this](String^ response)
		{
			OutputDebugString((response + "\n")->Data());

			JsonObject^ jsonResp = JsonObject::Parse(response);
			CheckResponseJson(jsonResp);
			AccessToken = jsonResp->GetNamedString("access_token");
			if (AccessToken == nullptr) {
				return false;
			}
			OutputDebugString((AccessToken + "\n")->Data());

			return true;
		});
	});
}

IAsyncOperation<PostsPageInfo^>^ GhostRest::GetPosts(int pageId, int postsPerPage)
{
	Uri^ siteUri = ref new Uri(Site + GHOST_API_PREFIX + GET_POSTS + "/?status=all&limit=" 
		+ postsPerPage + "&page=" + pageId + "&include=tags");
	
	ErrorMsg->ErrorKind = GHOST_NONE_ERROR;
	return create_async([=] {

		return create_task(client->GetAsync(siteUri))
			.then([this](HttpResponseMessage^ response)
		{
			CheckStatusCode(response->StatusCode);
			return response->Content->ReadAsStringAsync();
		}).then([this](String^ response)
		{
			OutputDebugString((response + "\n")->Data());

			JsonObject^ jsonResp = JsonObject::Parse(response);
			CheckResponseJson(jsonResp);

			PostsPageInfo^ page = ref new PostsPageInfo;
			page->ValueOf(jsonResp);

			return page;
		});
	});
}

IAsyncOperation<PostInfo^>^ GhostRest::GetPost(int postId)
{
	Uri^ siteUri = ref new Uri(Site + GHOST_API_PREFIX + GET_POSTS + "/" + postId + "/?include=tags&status=all");

	ErrorMsg->ErrorKind = GHOST_NONE_ERROR;
	return create_async([=] {

		return create_task(client->GetAsync(siteUri))
			.then([this](HttpResponseMessage^ response)
		{
			CheckStatusCode(response->StatusCode);
			return response->Content->ReadAsStringAsync();
		}).then([this](String^ response)
		{
			OutputDebugString((response + "\n")->Data());

			JsonObject^ jsonResp = JsonObject::Parse(response);
			CheckResponseJson(jsonResp);
			
			PostInfo^ post = ref new PostInfo;
			JsonObject^ postJson = jsonResp->GetNamedArray("posts")->GetObjectAt(0);
			post->ValueOf(postJson);

			return post;
		});
	});
}

IAsyncOperation<PostInfo^>^ GhostRest::GetPostBySlug(String^ slug)
{
	Uri^ siteUri = ref new Uri(Site + GHOST_API_PREFIX + GET_POSTS + "/slug/" + slug + "/?include=tags");

	ErrorMsg->ErrorKind = GHOST_NONE_ERROR;
	return create_async([=] {

		return create_task(client->GetAsync(siteUri))
			.then([this](HttpResponseMessage^ response)
		{
			CheckStatusCode(response->StatusCode);
			return response->Content->ReadAsStringAsync();
		}).then([this](String^ response)
		{
			OutputDebugString((response + "\n")->Data());

			JsonObject^ jsonResp = JsonObject::Parse(response);
			CheckResponseJson(jsonResp);

			PostInfo^ post = ref new PostInfo;
			JsonObject^ postJson = jsonResp->GetNamedArray("posts")->GetObjectAt(0);
			post->ValueOf(postJson);

			return post;
		});
	});
}

IAsyncOperation<PostInfo^>^ GhostRest::AddPost(PostInfo^ post)
{
	Uri^ siteUri = ref new Uri(Site + GHOST_API_PREFIX + GET_POSTS + "/?include=tags");
	JsonObject^ postJson = post->ToJsonObject(true);

	OutputDebugString((postJson->Stringify() + "\n")->Data());
	HttpStringContent^ content = ref new HttpStringContent(postJson->Stringify(),
		Windows::Storage::Streams::UnicodeEncoding::Utf8, "application/json");

	ErrorMsg->ErrorKind = GHOST_NONE_ERROR;
	return create_async([=] {

		return create_task(client->PostAsync(siteUri, content))
			.then([this](HttpResponseMessage^ response)
		{
			CheckStatusCode(response->StatusCode);
			return response->Content->ReadAsStringAsync();
		}).then([this](String^ response)
		{
			OutputDebugString((response + "\n")->Data());

			JsonObject^ jsonResp = JsonObject::Parse(response);
			CheckResponseJson(jsonResp);

			PostInfo^ post = ref new PostInfo;
			JsonObject^ postJson = jsonResp->GetNamedArray("posts")->GetObjectAt(0);
			post->ValueOf(postJson);

			return post;
		});
	});
}

IAsyncOperation<PostInfo^>^ GhostRest::UpdatePost(PostInfo^ post)
{
	Uri^ siteUri = ref new Uri(Site + GHOST_API_PREFIX + GET_POSTS + "/" + post->Id + "/?include=tags");
	JsonObject^ postJson = post->ToJsonObject(false);
	
	OutputDebugString((postJson->Stringify() + "\n")->Data());
	HttpStringContent^ content = ref new HttpStringContent(postJson->Stringify(), 
		Windows::Storage::Streams::UnicodeEncoding::Utf8, "application/json");

	ErrorMsg->ErrorKind = GHOST_NONE_ERROR;
	return create_async([=] {

		return create_task(client->PutAsync(siteUri, content))
			.then([this](HttpResponseMessage^ response)
		{
			CheckStatusCode(response->StatusCode);
			return response->Content->ReadAsStringAsync();
		}).then([this](String^ response)
		{
			OutputDebugString((response + "\n")->Data());

			JsonObject^ jsonResp = JsonObject::Parse(response);
			CheckResponseJson(jsonResp);

			PostInfo^ post = ref new PostInfo;
			JsonObject^ postJson = jsonResp->GetNamedArray("posts")->GetObjectAt(0);
			post->ValueOf(postJson);

			return post;
		});
	});
}

IAsyncOperation<PostInfo^>^ GhostRest::DeletePost(int postId)
{
	Uri^ siteUri = ref new Uri(Site + GHOST_API_PREFIX + GET_POSTS + "/" + postId + "/?include=tags");

	ErrorMsg->ErrorKind = GHOST_NONE_ERROR;
	return create_async([=] {

		return create_task(client->DeleteAsync(siteUri))
			.then([this](HttpResponseMessage^ response)
		{
			CheckStatusCode(response->StatusCode);
			return response->Content->ReadAsStringAsync();
		}).then([this](String^ response)
		{
			OutputDebugString((response + "\n")->Data());

			JsonObject^ jsonResp = JsonObject::Parse(response);
			CheckResponseJson(jsonResp);

			PostInfo^ post = ref new PostInfo;
			JsonObject^ postJson = jsonResp->GetNamedArray("posts")->GetObjectAt(0);
			post->ValueOf(postJson);

			return post;
		});
	});
}

IAsyncOperation<BlogInfo^>^ GhostRest::GetBlogInfo()
{
	Uri^ siteUri = ref new Uri(Site + GHOST_API_PREFIX + GET_SETTINGS + "/");

	ErrorMsg->ErrorKind = GHOST_NONE_ERROR;
	return create_async([=] {

		return create_task(client->GetAsync(siteUri))
			.then([this](HttpResponseMessage^ response)
		{
			CheckStatusCode(response->StatusCode);
			return response->Content->ReadAsStringAsync();
		}).then([this](String^ response)
		{
			OutputDebugString((response + "\n")->Data());

			JsonObject^ jsonResp = JsonObject::Parse(response);
			CheckResponseJson(jsonResp);

			BlogInfo^ blog = ref new BlogInfo;
			blog->ValueOf(jsonResp);
			return blog;
		});
	});
}

IAsyncOperation<IObservableVector<TagInfo^>^>^ GhostRest::GetAllTags()
{
	Uri^ siteUri = ref new Uri(Site + GHOST_API_PREFIX + GET_TAGS + "/?limit=all");
	return create_async([=] {
		return create_task(client->GetAsync(siteUri))
			.then([this](HttpResponseMessage^ response)
		{
			CheckStatusCode(response->StatusCode);
			return response->Content->ReadAsStringAsync();
		}).then([this](String^ response)
		{
			OutputDebugString((response + "\n")->Data());

			JsonObject^ jsonResp = JsonObject::Parse(response);
			CheckResponseJson(jsonResp);

			auto jsonArray = jsonResp->GetNamedArray("tags");

			auto tags = TagInfo::ValueOfJsonArray(jsonArray);
			return tags;
		});
	});
}

void GhostRest::CheckStatusCode(HttpStatusCode status)
{
	if (status == HttpStatusCode::Unauthorized ||
		status == HttpStatusCode::Forbidden) {
		ErrorMsg->ErrorKind = GHOST_AUTH_ERROR;
	}
	else {
		ErrorMsg->ErrorKind = GHOST_OTHER_ERROR;
	}
}

void GhostRest::CheckResponseJson(JsonObject ^ json)
{
	if (json->HasKey("errors")) {
		auto errorArray = json->GetNamedArray("errors");
		auto errorObj = errorArray->GetAt(0)->GetObject();
		//ErrorMsg->ErrorType = errorObj->GetNamedString("type");
		ErrorMsg->Message = errorObj->GetNamedString("message");
		
		throw ref new COMException(-1);
	}
	else if (json->HasKey("type")) {
		//ErrorMsg->ErrorType = "OtherError";
		ErrorMsg->Message = json->GetNamedString("message");

		throw ref new COMException(-1);
	}

}

void TagInfo::ValueOf(JsonObject^ json)
{
	Id = (int)json->GetNamedNumber("id");
	Name = json->GetNamedString("name");
	Slug = json->GetNamedString("slug");
}

JsonObject^ TagInfo::GetJsonForUpdate()
{
	JsonObject^ json = ref new JsonObject;
	json->Insert("id", JsonValue::CreateNumberValue(Id));
	json->Insert("name", JsonValue::CreateStringValue(Name));
	json->Insert("slug", JsonValue::CreateStringValue(Slug));
	return json;
}

JsonArray^ TagInfo::GetJsonArrayForTags(IIterator<TagInfo^>^ tags)
{
	JsonArray^ jsonArray = ref new JsonArray;

	if (tags != nullptr) {
		for (auto iter = tags; iter->HasCurrent; iter->MoveNext()) {
			auto tag = iter->Current;
			jsonArray->Append(tag->GetJsonForUpdate());
		}
	}
	return jsonArray;
}

IObservableVector<TagInfo^>^ TagInfo::ValueOfJsonArray(JsonArray^ json)
{
	IObservableVector<TagInfo^>^ tags = ref new Vector<TagInfo^>;
	for (auto iter = json->First(); iter->HasCurrent; iter->MoveNext())
	{
		JsonObject^ jsonObj = iter->Current->GetObject();
		TagInfo^ t = ref new TagInfo;
		t->ValueOf(jsonObj);
		tags->Append(t);
	}
	return tags;
}
