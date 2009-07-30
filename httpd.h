/* Copyright 2009 by Yasuhiro Matsumoto
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _HTTPD_H_
#define _HTTPD_H_

#define HTTPD_VERSION 0x0100

#ifdef _WIN32
#pragma warning(disable:4018 4503 4530 4786)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <sys/types.h>
#ifdef _WIN32
#include <winsock2.h>
#include <process.h>
#include <direct.h>
#include <io.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/uio.h>
#include <limits.h>
#include <unistd.h>
#endif
#include <sstream>

#ifndef _WIN32
#include <pthread.h>
#endif

#include "utils.h"

namespace tthttpd {

class server {
public:
	typedef struct {
		std::string name;
		unsigned long size;
		bool isdir;
		struct tm date;
	} ListInfo;
	typedef struct {
		int msgsock;
		server *httpd;
		tstring address;
	} HttpdInfo;
	typedef std::map<tstring, tstring> BasicAuths;
	typedef struct {
		std::vector<tstring> accept_list;
	} AcceptAuth;
	typedef std::map<tstring, AcceptAuth> AcceptAuths;
	typedef std::vector<tstring> AcceptIPs;

	typedef void (*LoggerFunc)(const HttpdInfo* httpd_info, const tstring& request);
	typedef std::map<tstring, tstring> MimeTypes;
	typedef std::vector<tstring> DefaultPages;
	typedef std::map<tstring, tstring> RequestAliases;
	typedef std::map<tstring, tstring> RequestEnvironments;

private:
#ifdef _WIN32
	HANDLE thread;
#else
	pthread_t thread;
#endif

public:
	int sock;
	std::string hostname;
	std::string hostaddr;
	std::string root;
	std::string fs_charset;
	unsigned short port;
	BasicAuths basic_auths;
	AcceptAuths accept_auths;
	AcceptIPs accept_ips;
	MimeTypes mime_types;
	DefaultPages default_pages;
	RequestAliases request_aliases;
	RequestEnvironments request_environments;
	LoggerFunc loggerfunc;
	int verbose_mode;

	void initialize() {
		sock = -1;
		port = 80;
		fs_charset = "utf-8";
		thread = 0;
		loggerfunc = NULL;
		mime_types[_T("gif")] = _T("image/gif");
		mime_types[_T("jpg")] = _T("image/jpeg");
		mime_types[_T("png")] = _T("image/png");
		mime_types[_T("htm")] = _T("text/html");
		mime_types[_T("html")] = _T("text/html");
		mime_types[_T("txt")] = _T("text/plain");
		mime_types[_T("xml")] = _T("text/xml");
		mime_types[_T("js")] = _T("application/x-javascript");
		mime_types[_T("css")] = _T("text/css");
		default_pages.push_back(_T("index.html"));
		default_pages.push_back(_T("index.php"));
		default_pages.push_back(_T("index.cgi"));
		verbose_mode = 0;
	};

	server() {
		initialize();
	}
	server(unsigned short _port) {
		initialize();
		port = _port;
	}
	server(unsigned short _port, tstring _target) {
		initialize();
		port = _port;
	}
	~server() {
		stop();
	}
	bool start();
	bool stop();
	bool wait();
	void set_fs_charset(tstring _fs_charset) {
		fs_charset = tstring2string(_fs_charset);
	}
	tstring get_fs_charset() {
		return string2tstring(fs_charset);
	}
	void setAuthentication(BasicAuths _basic_auths) {
		basic_auths = _basic_auths;
	}
	void bindRoot(tstring _root) {
		root = get_realpath(tstring2string(_root));
	}
	static std::string get_realpath(std::string path) {
#ifdef _WIN32
		char fullpath[_MAX_PATH] = {0};
		char* filepart = NULL;
		if (GetFullPathNameA(path.c_str(), _MAX_PATH, fullpath, &filepart))
			path = fullpath;
#else
		char fullpath[PATH_MAX] = {0};
		if (realpath((char*)path.c_str(), fullpath))
			path = fullpath;
#endif
		std::replace(path.begin(), path.end(), '\\', '/');
		size_t end_pos = path.find_last_of("?#");
		if (end_pos != std::string::npos) path.resize(end_pos);

		std::vector<std::string> path_sep = split_string(path, "/");
		std::vector<std::string>::iterator it;
		while(true) {
			it = std::find(path_sep.begin(), path_sep.end(), "..");
			if (it == path_sep.end()) break;
			if (it == path_sep.begin()) {
				continue;
			}
			path_sep.erase(it-1);
			path_sep.erase(it-1);
		}
		std::string path_real;
		for(it = path_sep.begin(); it != path_sep.end(); it++) {
			path_real += *it;
			if (it+1 != path_sep.end())
				path_real += "/";
		}
		if (path[path.size()-1] == '/')
			path_real += "/";
		return path_real;
	}
};

}

#endif /* _HTTPD_H_ */