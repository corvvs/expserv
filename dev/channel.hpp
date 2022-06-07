#ifndef CHANNEL_HPP
# define CHANNEL_HPP
# include <map>
# include <utility>
# include "socketlistening.hpp"
# include "isocketlike.hpp"
# include "iobserver.hpp"

class Conenction;

// [チャネルクラス]
// listenしているソケット1つを保持し,
// 接続要求に応じてコネクションを作成する.
// (予定)
// - 同じプロトコル/ポートを持つバーチャルホスト(server)をまとめて管理する
class Channel: public ISocketLike {
public:
    // Channelを識別するためのID
    // プロトコルファミリーとポートの組
    typedef std::pair<t_socket_domain, t_port> t_channel_id;

private:
    SocketListening*    sock;

    // 呼び出し禁止
    // Channelは必ず有効なソケットと紐づく必要があるため.
    Channel();


public:

    Channel(t_socket_domain sdomain, t_socket_type stype, t_port port);

    ~Channel();

    t_fd            get_fd() const;
    void            notify(IObserver& loop);

    t_channel_id    get_id() const;
};

#endif
