# include "httpserver.hpp"

HTTPServer::HTTPServer(IObserver* observer): socket_observer_(observer) {}

HTTPServer::~HTTPServer() {
    delete socket_observer_;
}

void            HTTPServer::listen(
    t_socket_domain sdomain,
    t_socket_type stype,
    t_port port
) {
    Channel *ch = new Channel(this, sdomain, stype, port);
    channels[ch->get_id()] = ch;
    socket_observer_->reserve_set(ch, SHMT_READ);
}

void            HTTPServer::run() {
    socket_observer_->loop();
}

ResponseHTTP*   HTTPServer::route(RequestHTTP* request) {
    ResponseHTTP*   res = new ResponseHTTP(
        request->get_http_version(),
        HTTP::STATUS_OK
    );
    res->feed_body(
        request->get_body_begin(),
        request->get_body_end()
    );
    res->render();
    DSOUT() << res->get_message_text() << std::endl;
    return res;
}
