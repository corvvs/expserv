#ifndef CONNECTION_HPP
# define CONNECTION_HPP
# include "socketconnected.hpp"
# include "isocketlike.hpp"
# include "iobserver.hpp"
# include "irouter.hpp"
# include "requesthttp.hpp"
# include "responsehttp.hpp"
# include <string>
# include <iostream>

// コネクションオブジェクトの内部状態
enum t_connection_phase {
    // 受信モード
    CONNECTION_RECEIVING,
    // 送信モード(通常応答)
    CONNECTION_RESPONDING,
    // 送信モード(エラー応答)
    CONNECTION_ERROR_RESPONDING
};

// [コネクションクラス]
// [責務]
// - 通信可能ソケット1つを保持すること
// - このソケット経由の双方向通信を管理すること
//  - リクエストの受信と解釈
//  - レスポンスの送信
//    - 生成は ルーティングクラス(IRouter) が行う
class Connection: public ISocketLike {
private:
    IRouter*            router_;
    t_connection_phase  phase;
    bool                dying;

    SocketConnected*    sock;

    RequestHTTP*        current_req;
    ResponseHTTP*       current_res;

    // 直接呼び出し禁止
    // ConnectionオブジェクトはChannelオブジェクトによってのみ作成されて欲しいため
    Connection();

    // current_req, current_res をクリアする
    void    clear_currents();

public:
    Connection(IRouter* router, SocketConnected* sock_given);
    ~Connection();

    t_fd    get_fd() const;
    void    notify(IObserver& observer);
    
};

#endif
