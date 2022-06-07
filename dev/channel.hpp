#ifndef CHANNEL_HPP
# define CHANNEL_HPP
# include <map>
# include <utility>
# include "socketlistening.hpp"
# include "isocketlike.hpp"
# include "iobserver.hpp"
# include "irouter.hpp"

// [チャネルクラス]
// [責務]
// - リスニングソケット1つを保持すること
// - 接続要求に応じてコネクションクラスを生成して返すこと
// - TODO: 同じプロトコル/ポートを持つバーチャルホスト(server)をまとめて管理すること
class Channel: public ISocketLike {
public:
    // Channelを識別するためのID
    // プロトコルファミリーとポートの組
    typedef std::pair<t_socket_domain, t_port> t_channel_id;

private:
    SocketListening*    sock;
    IRouter*            router_;

    // 呼び出し禁止
    // Channelは必ず有効なソケットと紐づく必要があるため.
    Channel();


public:

    Channel(
        IRouter* router,
        t_socket_domain sdomain,
        t_socket_type stype,
        t_port port
    );

    ~Channel();

    t_fd            get_fd() const;
    void            notify(IObserver& loop);

    t_channel_id    get_id() const;
};

#endif
