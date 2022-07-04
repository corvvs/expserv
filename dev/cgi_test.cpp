#include "cgi.hpp"

int main(int argc, char **argv, char **envp) {
    (void)argc;
    (void)**argv;
    HTTP::byte_string content   = HTTP::strfy("UFO仮面ヤキソバン！！！！");
    CGI::metavar_dict_type dict = CGI::make_metavars_from_envp(envp);
    HTTP::byte_string path      = HTTP::strfy("/Users/corvvs/reps/expserv/dev/a.out");
    HTTP::byte_string query     = HTTP::strfy("");
    CGI *cgi                    = CGI::start_cgi(path, query, dict, content.size());
    cgi->set_content(content);
    while (true) {
        VOUT(1);
        cgi->send_content();
        cgi->receive();
    }
}
