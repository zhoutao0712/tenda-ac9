
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <stdint.h>
#include <syslog.h>
#include <ctype.h>

#include <bcmnvram.h>
#include <shutils.h>

#include <curl/curl.h>
#include <json.h>

#include "http.h"

int sleep_seconds = 1;

static const char *json_object_object_get_string(struct json_object *jso, const char *key)
{
	struct json_object *j;

	j = json_object_object_get(jso, key);
	if(j)
		return json_object_get_string(j);
	else
		return NULL;
}

static int json_object_object_get_int(struct json_object *jso, const char *key, int *ret_cb)
{
	struct json_object *j;

	*ret_cb = 0;
	j = json_object_object_get(jso, key);
	if(!j) *ret_cb = -1; 
	if(j)
		return json_object_get_int(j);
	else 
		return *ret_cb;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
static inline void print_request_head_info(struct evkeyvalq *header)
{
	struct evkeyval *first_node = header->tqh_first;
	while (first_node) {
		printf("key:%s  value:%s\n", first_node->key, first_node->value);
		first_node = first_node->next.tqe_next;
	}
}

static inline void print_uri_parts_info(const struct evhttp_uri * http_uri)
{
	printf("scheme:%s\n", evhttp_uri_get_scheme(http_uri));
	printf("host:%s\n", evhttp_uri_get_host(http_uri));
	printf("path:%s\n", evhttp_uri_get_path(http_uri));
	printf("port:%d\n", evhttp_uri_get_port(http_uri));
	printf("query:%s\n", evhttp_uri_get_query(http_uri));
	printf("userinfo:%s\n", evhttp_uri_get_userinfo(http_uri));
	printf("fragment:%s\n", evhttp_uri_get_fragment(http_uri));
}

static char *nvram_get_dns_url(int n)
{
	char buf[128];

	sprintf(buf, "tinc_dns_url%d", n);

//printf("url=%s\n", nvram_safe_get(buf));

	return nvram_safe_get(buf);
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------
static int json_set_nvram(struct json_object *obj)
{
	const char *name, *value;

	name = json_object_object_get_string(obj, "name");
	value = json_object_object_get_string(obj, "value");

	if( (name == NULL)||(value == NULL) ) return -1;

	if(strcmp(nvram_safe_get(name), value) == 0) return 0;

	nvram_set(name, value);

	return 1;
}


static int do_nvram_url(char *buf)
{
	struct json_object *root_obj;
	struct json_object *nvram_obj;
	struct array_list *nvram_arr;
	int i, ret, need_commit;

	root_obj = json_tokener_parse(buf);
	if(root_obj == NULL) return -1;

	i = json_object_object_get_int(root_obj, "sleep_time", &ret);
	if(ret != 0) {
		ret = -2;
		goto out;
	}

	sleep_seconds = i;

	nvram_obj = json_object_object_get(root_obj, "nvram_list");
	nvram_arr = json_object_get_array(nvram_obj);
	if(nvram_arr == NULL) {
		ret = -3;
		goto out;
	}

printf("%s %d: arr->len=%d\n", __FUNCTION__, __LINE__, nvram_arr->length);

	need_commit = 0;
	for(i = 0; i < nvram_arr->length; i++) {
		if(json_set_nvram(nvram_arr->array[i]) == 1) need_commit = 1;
	}

	if(need_commit) nvram_commit();
	json_object_put(root_obj);

	return 0;

out:
	json_object_put(root_obj);
	return ret;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void http_requset_get_cb(struct evhttp_request *req, void *arg)
{
	struct http_request_get *http_req_get = (struct http_request_get *)arg;

	if((req)&&(req->response_code == HTTP_OK)) {
		struct evbuffer* buf = evhttp_request_get_input_buffer(req);
		size_t len = evbuffer_get_length(buf);

//		printf("print the head info:");
//		print_request_head_info(req->output_headers);

		printf("len:%zu  body size:%zu\n", len, req->body_size);
		char *tmp = malloc(len+1);
		memcpy(tmp, evbuffer_pullup(buf, -1), len);
		tmp[len] = '\0';

		printf("\nprint the body: HTTP BODY:%s\n\n", tmp);

		if(do_nvram_url(tmp) == 0) event_base_loopbreak(http_req_get->base);

		free(tmp);
	}

	return;
}

static struct http_request_get *http_request_new(struct event_base* base, const char *url)
{
	struct http_request_get *http_req_get = calloc(1, sizeof(struct http_request_get));

	if(http_req_get == NULL) {
		return NULL;
	}

	http_req_get->base = base;

	http_req_get->uri = evhttp_uri_parse(url);
	if(http_req_get->uri == NULL) {
		free(http_req_get);
		return NULL;
	}

	print_uri_parts_info(http_req_get->uri);

	if( (evhttp_uri_get_scheme(http_req_get->uri) == NULL) 
		|| (evhttp_uri_get_host(http_req_get->uri) == NULL) ) {

		free(http_req_get);
		return NULL;
	}

	return http_req_get;
}

static int start_url_request(struct http_request_get *http_req)
{
	if (http_req->cn) evhttp_connection_free(http_req->cn);

	int port = evhttp_uri_get_port(http_req->uri);
	const char *host = evhttp_uri_get_host(http_req->uri);
	const char *query = evhttp_uri_get_query(http_req->uri);
	const char *path = evhttp_uri_get_path(http_req->uri);
	char path_query[512];

	if((path) &&(strlen(path) > 0)) {
		if(query) sprintf(path_query, "%s?%s", path, query);
		else sprintf(path_query, "%s", path);
	} else {
		sprintf(path_query, "/");
	}

/*
	if(strcmp(host, "d1.luyouqi2017.com") == 0) {
		host = "104.192.80.22";
	} else if (strcmp(host, "d1.luyouqi2018.com") == 0) {
		host = "104.233.226.102";
	} else if (strcmp(host, "d1.bi051loc.work") == 0) {
		host = "104.192.83.114";
	} else if (strcmp(host, "d1.it018lao.xyz") == 0) {
		host = "104.192.80.22";
	} else if (strcmp(host, "d1.3i0p1lqo.life") == 0) {
		host = "104.233.226.102";
	} else if (strcmp(host, "d2.luyouqi2017.com") == 0) {
		host = "104.192.80.22";
	} else if (strcmp(host, "d2.luyouqi2018.com") == 0) {
		host = "104.233.226.102";
	}
*/

	http_req->cn = evhttp_connection_base_new(http_req->base, NULL, host, (port == -1 ? 80 : port));

/*
	Request will be released by evhttp connection
	See info of evhttp_make_request()
*/

	http_req->req = evhttp_request_new(http_requset_get_cb, http_req);

	evhttp_add_header(http_req->req->output_headers, "Host", evhttp_uri_get_host(http_req->uri));
	evhttp_make_request(http_req->cn, http_req->req, EVHTTP_REQ_GET, path_query);

	return 0;
}

static struct http_request_get *start_http_requset(struct event_base* base, const char *url)
{
	struct http_request_get *http_req_get = http_request_new(base, url);

printf("%s %d: 2222222222222\n", __FUNCTION__, __LINE__);

	if(http_req_get) start_url_request(http_req_get);

printf("%s %d: 33333333333333\n", __FUNCTION__, __LINE__);

	return http_req_get;
}

static void http_request_free(struct http_request_get *http_req_get)
{
	if(http_req_get) {
		if(http_req_get->cn) evhttp_connection_free(http_req_get->cn);
		if(http_req_get->uri) evhttp_uri_free(http_req_get->uri);
		free(http_req_get);
	}

	http_req_get = NULL;
}

static void httpdns(void)
{
	int i;
	struct event_base *base = event_base_new();

	struct http_request_get *http_req_get[10];

/*
	http_req_get[0] = start_http_requset(base, "http://d1.luyouqi2017.com/nvram.php");
	http_req_get[1] = start_http_requset(base, "http://d1.luyouqi2018.com/nvram.php");
	http_req_get[2] = start_http_requset(base, "http://d1.bi051loc.work/nvram.php");
	http_req_get[3] = start_http_requset(base, "http://d1.it018lao.xyz/nvram.php");
	http_req_get[4] = start_http_requset(base, "http://d1.3i0p1lqo.life/nvram.php");
	http_req_get[5] = start_http_requset(base, "http://d2.luyouqi2017.com/nvram.php");
	http_req_get[6] = start_http_requset(base, "http://d2.luyouqi2018.com/nvram.php");
	http_req_get[7] = start_http_requset(base, "http://104.233.226.102/nvram.php");
	http_req_get[8] = start_http_requset(base, "http://104.192.83.114/nvram.php");
	http_req_get[9] = start_http_requset(base, "http://104.192.80.22/nvram.php");
*/

	for(i = 0; i < 10; i++) {
		http_req_get[i] = start_http_requset(base, nvram_get_dns_url(i));
	}

printf("%s %d: 0000000000000\n", __FUNCTION__, __LINE__);

	event_base_dispatch(base);

printf("%s %d: 1111111111111\n", __FUNCTION__, __LINE__);

	for(i = 0; i < 10; i++) {
		http_request_free(http_req_get[i]);
	}

	event_base_free(base);
}

int main(int argc, char *argv[])
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	if(argc == 1) {
		if (daemon(1, 1) == -1) {
			syslog(LOG_ERR, "daemon: %m");
			return 0;
		}
	}

	while (1) {
printf("%s %d: SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS\n", __FUNCTION__, __LINE__);

		sleep(sleep_seconds);
		if(sleep_seconds < 30) sleep_seconds = sleep_seconds + 3;
		httpdns();
	}

	return 0;
}

