# Ghost Client for WP

Ghost 博客客户端，使用 Windows Universal Application 8.1 开发，
主要面向 Windows Phone 用户，和 Windows 平板用户。

## 开发缘由

Ghost 博客有一个很好的网页界面，可是这个网页界面在 WP 手机上的体验却是非常糟糕。

我使用的是 Lumia 638，使用 IE 登陆 Ghost 管理界面，界面的适配很糟糕：

* 底边栏只显示中间部分，两头被挡住，标签不能使用
* 写文章时，输入法很卡，很卡。这可能是输入法的锅，也可能是浏览器的锅。
浏览器中的文本编辑框，在使用中文九宫格输入法输入文字时，总会迅速变得很卡很卡，
使用英文输入法，情况缓解，使用全键盘输入法，情况也会缓解，猜测是因为九宫格输入法，
每个按键输入都会有一些预选项，多次输入到产生候选词汇，会有很多变化，造成很多处理，
导致输入非常卡。

此外，网页版的管理界面，产生的流量也不小。

所以我便有这个需求，在 GitHub 上找了一番，只找到 [ghost-client/ghost-ionic](
https://github.com/ghost-client/ghost-ionic)，是为 Android/IOS 开发的，我用不到。

所以，自己动手，丰衣足食。

Windows 10 for mobile 正式版也快出了，听说能支持 Android 应用，不过离出来还有段时间，
等不下去了。一想到，将来或许要把这个项目 Port 到 Windows 10 上，便很受伤。

## 开发依赖

* Visual Studio 2013 (C++)
* Visual Studio 2015 (C++)

## 当前进度

已实现功能有：

* 登录
* 获取博文列表
* 查看博文，包括 html 浏览（使用已经获取的 html 数据，减少流量消耗）
* 新建，编辑，删除博文，发布博文
* 获取标签，并设置标签

如果有时间，接下来的工作：
* Token 的刷新实现，需要调整
* 使用字符串资源，错误处理信息提示灯
* 界面改良
* 增加标签
* 按标签浏览，编辑
* 图片上传，使用
* ...

## 相关项目

* [ghost-client/ghost-ionic](https://github.com/ghost-client/ghost-ionic)：使用 
[Ionic](http://ionicframework.com/)，可用于 Android/IOS 的 Ghost 博客客户端，
未来可能也能用于 Windows Phone。
* [sindresorhus/github-markdown-css](https://github.com/sindresorhus/github-markdown-css)：
GitHub 的 Markdown CSS 风格，主要用于美化博文浏览。
* [highlight.js](https://highlightjs.org/)：代码高亮，也是美化博文浏览。