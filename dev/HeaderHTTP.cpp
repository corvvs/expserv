#include "HeaderHTTP.hpp"
#define DEFINE_ATTR(key, list, aggr, uniq) { HeaderHTTPAttribute attr = { list, aggr, uniq, }; predefined_attrs[key] = attr; }

// [HeaderHTTPAttribute]

HeaderHTTPAttribute::attr_dict_type HeaderHTTPAttribute::predefined_attrs = HeaderHTTPAttribute::attr_dict_type();

void    HeaderHTTPAttribute::set_predefined_attrs() {
    // [transfer-encoding]
    // https://triple-underscore.github.io/RFC7230-ja.html#_xref-6-10
    // "TE"ヘッダとは別物(TEは同じメッセージへの応答に対する指定; Transfer-Encodingは同じメッセージに対する指定)
    // - list, multiple
    DEFINE_ATTR(HeaderHTTP::transfer_encoding,  1, 1, 0);
    // [te]
    // https://triple-underscore.github.io/RFC7230-ja.html#section-4.3
    DEFINE_ATTR(HeaderHTTP::te,                 1, 1, 0);
    // [set-cookie]
    // https://wiki.suikawiki.org/n/Set-Cookie%3A
    // 複数存在可能 & 集約禁止
    DEFINE_ATTR(HeaderHTTP::set_cookie,         0, 0, 0);
    // [content-length]
    // https://wiki.suikawiki.org/n/Content-Length%3A#anchor-88
    DEFINE_ATTR(HeaderHTTP::content_length,     0, 0, 1);
    // [host]
    // https://triple-underscore.github.io/RFC7230-ja.html#header.host
    DEFINE_ATTR(HeaderHTTP::host,               0, 0, 1);

}

// [HeaderHTTPItem]

HeaderHTTPItem::HeaderHTTPItem(const header_key_type& key):
    key(key) {
    // 属性(attr)
    HeaderHTTPAttribute::attr_dict_type::iterator it = HeaderHTTPAttribute::predefined_attrs.find(key);
    if (it == HeaderHTTPAttribute::predefined_attrs.end()) {
        HeaderHTTPAttribute a = {};
        attr = a;
    } else {
        // predefineされているならそれを設定する
        attr = it->second;
    }
}

void    HeaderHTTPItem::add_val(const header_val_type& val) {
    if (values.size() > 0) {
        // すでにvalueがある場合 -> 必要に応じてしかるべく処理する
        DSOUT() << "* MULTIPLE VALUES * " << values.size() << std::endl;
    }
    values.push_back(val);
}

const HeaderHTTPItem::header_val_type*
        HeaderHTTPItem::get_val() const {
    return values.empty() ? NULL : &(values.front());
}

const HeaderHTTPItem::header_val_type*
        HeaderHTTPItem::get_back_val() const {
    return values.empty() ? NULL : &(values.back());
}

const HeaderHTTPItem::value_list_type&
        HeaderHTTPItem::get_vals() const {
    return values;
}

// [HeaderHTTPHolder]

void    HeaderHTTPHolder::add_item(const light_string& key, const header_val_type& val) {
    header_key_type norm_key = ParserHelper::normalize_header_key(key);
    HeaderHTTPItem* item = dict[norm_key];
    if (item == NULL) {
        list.push_back(HeaderHTTPItem(norm_key));
        item = &(list.back());
        dict[norm_key] = item;
    }
    item->add_val(val);
}

const HeaderHTTPHolder::header_item_type*
        HeaderHTTPHolder::get_item(const header_key_type& normalized_key) const {
    dict_type::const_iterator it = dict.find(normalized_key);
    if (it == dict.end()) { return NULL; }
    return it->second;
}

const HeaderHTTPHolder::header_val_type*
        HeaderHTTPHolder::get_val(const header_key_type& normalized_key) const {
    const HeaderHTTPItem* p = get_item(normalized_key);
    return p ? p->get_val() : NULL;
}

const HeaderHTTPHolder::header_val_type*
        HeaderHTTPHolder::get_back_val(const header_key_type& normalized_key) const {
    const HeaderHTTPItem* p = get_item(normalized_key);
    return p ? p->get_back_val() : NULL;
}

const HeaderHTTPHolder::value_list_type*
        HeaderHTTPHolder::get_vals(const header_key_type& normalized_key) const {
    const HeaderHTTPItem* p = get_item(normalized_key);
    return p ? &(p->get_vals()) : NULL;
}
