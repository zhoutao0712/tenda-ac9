
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/keyvalq_struct.h>

struct http_request_get {
	struct evhttp_uri *uri;
	struct event_base *base;
	struct evhttp_connection *cn;
	struct evhttp_request *req;
};



