#ifndef IROUTER_HPP
#define IROUTER_HPP
#include "requesthttp.hpp"
#include "responsehttp.hpp"

// [ルータインターフェース]
// [責務]
// - 受け取ったリクエストをルーティングし, レスポンスを作成して返すこと
// - 与えられたHTTPエラーからエラーレスポンスを作成して返すこと
class IRouter {
public:
    virtual ~IRouter(){};

    // リクエストからレスポンスを生成する
    virtual ResponseHTTP *route(RequestHTTP *request) = 0;

    // HTTPエラーからレスポンスを生成する
    virtual ResponseHTTP *respond_error(RequestHTTP *request, http_error error) = 0;
};

#endif
