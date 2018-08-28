
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

#include <shared.h>

#include <curl/curl.h>
#include <json.h>

#include "upgrade.h"

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

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		printf("realloc fail\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

static int http_get_data(struct MemoryStruct *chunk, const char *url)
{
	CURLcode res;
	CURL *curl;
	struct curl_slist *http_headers = NULL;
	curl = curl_easy_init();
	if(curl == NULL){
		printf("get curl is fail!\n");
		return -1;
	}

//	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); 
	http_headers = curl_slist_append(http_headers, "Content-Type:application/json;charset=UTF-8");
	http_headers = curl_slist_append(http_headers, "Expect:");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 60L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 180L);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	res = curl_easy_perform(curl);
	if(res != CURLE_OK) {
		curl_slist_free_all(http_headers);
		curl_easy_cleanup(curl);
		return -2;
	}
	curl_slist_free_all(http_headers);
	curl_easy_cleanup(curl);

printf("response : %s\n", chunk->memory);

	return 0;
}

static int global_http_get_data(struct MemoryStruct *chunk, const char *url)
{
	int ret = -1;

	if(curl_global_init(CURL_GLOBAL_ALL) == CURLE_OK) {
		ret = http_get_data(chunk, url);
		curl_global_cleanup();
	}

	return ret;
}

static int make_back_server_url(char *url)
{
	int firmver_num = nvram_get_int("buildno");

	sprintf(url, "%s?mac=%s&id=%s&ver_num=%d&ver_sub=%s&model=%s&server=%s&ori_server=%s"
			, nvram_safe_get("back_server_url"), nvram_safe_get("et0macaddr"), nvram_safe_get("tinc_id"), firmver_num, nvram_safe_get("extendno")
			, "RTAC1200GP"
			, nvram_safe_get("tinc_cur_server"), nvram_safe_get("tinc_ori_server")
		);

	return 0;
}

static int json_to_response(struct json_object *obj, struct back_server_response *info)
{
	int ret;

	info->server = json_object_object_get_string(obj, "server");
	if(info->server == NULL) return -1;

	info->seconds = json_object_object_get_int(obj, "sleep_time", &ret);
	if(ret != 0) return -2;

	info->action = json_object_object_get_int(obj, "action", &ret);
	if(ret != 0) return -3;

	info->err_code = json_object_object_get_int(obj, "err_code", &ret);
	if(ret != 0) return -4;

	return 0;
}

static int real_do_back_server(struct back_server_response *info)
{
	FILE *f;

	if (!( f = fopen("/tmp/back_server_script.sh", "w"))) {
		return -1;
	}

printf("%s %d: 11111111\n", __FUNCTION__, __LINE__);

	fprintf(f,
		"#!/bin/sh\n"
		"nvram set tinc_cur_server=%s\n"
		"tinc -n gfw set gfw_server.address %s\n"
		"service restart_fasttinc\n"
		, info->server
		, info->server
	);

printf("%s %d: 222222\n", __FUNCTION__, __LINE__);

	fclose(f);

	chmod("/tmp/back_server_script.sh", 0700);
	sleep(1);
	eval("/tmp/back_server_script.sh");

printf("%s %d: 333333333\n", __FUNCTION__, __LINE__);

	return 0;
}

static void do_back_server(struct json_object *response_obj)
{
	struct back_server_response R;

	memset(&R, 0, sizeof(R));
	if(json_to_response(response_obj, &R) != 0) {
		printf("invalid response message from server\n");
		return;
	}

	printf("response sleep_time=%d action=%d err_code=%d\n"
		, R.seconds, R.action, R.err_code
	);

	if((R.seconds > 30) && (R.seconds < 3600)) sleep_seconds = R.seconds;
	if(R.err_code != 0) return;
	if(R.action != 1) return;

printf("%s %d: 444444444\n", __FUNCTION__, __LINE__);

	real_do_back_server(&R);
}

static void check_back_server(void)
{
	int ret;
	char back_server_url[2048];
	struct MemoryStruct M;
	struct json_object *response_obj;

	while (1) {
		sleep(sleep_seconds);
		if(sleep_seconds < 120) sleep_seconds = sleep_seconds + 5;

//1. make back_server_url
		if(make_back_server_url(back_server_url) != 0) continue;
printf("back_server_url=%s\n", back_server_url);

//2. http get response
		M.memory = malloc(1);
		M.size = 0;
		ret = global_http_get_data(&M, back_server_url);
		if(ret != 0) {
			free(M.memory);
			continue;
		}

//3. parse response
		response_obj = json_tokener_parse(M.memory);
		if(response_obj == NULL) {
			free(M.memory);
			continue;
		}

//4. do back_server
		do_back_server(response_obj);

//5. release
		json_object_put(response_obj);
		free(M.memory);
	}

}

int main(int argc, char *argv[])
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
//	signal(SIGCHLD, SIG_IGN);

	if(argc == 1) {
		if (daemon(1, 1) == -1) {
			syslog(LOG_ERR, "daemon: %m");
			return 0;
		}
	}

	sleep(2);

	check_back_server();

	return 0;
}

