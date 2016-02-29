<h1 align="center">
	<br>
	<img width="300" src="https://github.com/okingniko/frog/blob/master/media/logo.jpg" alt="frog">
	<br>
	<br>
</h1>

## Intro
A Fast, Multithreaded, Platform independent HTTP server and client library, implemented in c++11 and Boost.  Easy to add REST resources for your c++ applications.

**Coding Style**: [Google C++ 编程规范](http://zh-google-styleguide.readthedocs.org/en/latest/google-cpp-styleguide/contents/)

## Features
* Thread pool(convenience setting by MainConf.xml file)
* HTTP persistent connection(HTTP/1.1)
* Simple way to add REST resources using regex for path, and anonymous functions(lambda expression)
* Timeouts, if any of Server::timeout_request and Server::timeout_content are >0 (default: Server::timeout_request=5 seconds, and Server::timeout_content=300 seconds)
* Possibility to flush response to clients synchronously (Server::Response::flush).
* Client supports chunked transfer encoding.
* Etc.

## Demo
![frog demo](/media/frog_demo.gif)

## Usage
* Compile
```shell
cd build && cmake .. && make && make install
```
* Run
  1. Run `./frog_sample` as the server
  2. Run `./client_sample` for test, or Use your favorite browser as the Client.

##About REST 
**frog_sample api:**

|  Resource URL | Method | Explanation |
| ------------- | ------ | ----------- |
| /reqinfo      |   GET  | return the request information|
| /id/\<id:int\>  |   GET  | return id value (you may use the id value as the database index)|
| /string       |   POST | return the content of the post form |
| /json         |   POST | Analyse fixed format json, returns the parsed contents |

注：

1. frog_sample 中有default GET方法(当上述API无法匹配时，若请求页面存在，则回送页面)
2. 若想自行设计一套REST api，可参考资料：
  * [RESTful API 设计指南](http://www.ruanyifeng.com/blog/2014/05/restful_api.html)
  * [Flask web 开发](http://book.douban.com/subject/26274202/)



