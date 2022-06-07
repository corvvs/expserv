#ifndef CONNECTION_HPP
# define CONNECTION_HPP
# include "socketlistening.hpp"
# include "socketconnected.hpp"
# include "isocketlike.hpp"
# include "iobserver.hpp"
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

class Channel;

// [コネクションクラス]
// 通信可能ソケット1つを保持し,
// このソケットを通したクライアントとの通信すべてを担当する.
// 具体的には:
// - リクエストの受信
// - リクエストの解釈
//   - リクエストのルーティングは他のやつが担当しそう
// - レスポンスの作成(あやしい)
// - レスポンスの送信
// これらを内部状態を切り替えつつ処理する.
class Connection: public ISocketLike {
private:
    t_connection_phase  phase;
    bool                dying;

    SocketConnected*    sock;

    RequestHTTP*        current_req;
    ResponseHTTP*       current_res;

    // 直接呼び出し禁止
    // ConnectionオブジェクトはChannelオブジェクトによってのみ作成されて欲しいため
    Connection();

    // current_req, current_res をクリアし,
    // 送信待ち状態に遷移する
    void    clear_currents();

public:
    Connection(SocketConnected* sock_given);
    ~Connection();

    t_fd    get_fd() const;
    void    notify(IObserver& loop);
};

#endif
