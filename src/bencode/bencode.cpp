#include "bencode.h"

#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <iostream>

static void _free(BenValue *v) {
    switch (v->type) {
    case BEN_STR:
        delete v->s;
        break;
    case BEN_LIST:
        delete v->l;
        break;
    case BEN_DICT:
        delete v->d;
        break;
    default:
        break;
    }
    v->type = BEN_NULL;
    v->s = nullptr;
}
BenValue::~BenValue() { _free(this); }
BenValue::BenValue(const BenValue &v) : type(v.type) {
    switch (type) {
    case BEN_STR:
        s = new string(*v.s);
        return;
    case BEN_LIST:
        l = new vector<BenValue>(*v.l);
        return;
    case BEN_DICT:
        d = new map<string, BenValue>(*v.d);
        return;
    default:
        n = v.n;
        return;
    }
}
BenValue::BenValue(BenValue &&v) : type(v.type) {
    s = v.s;
    v.type = BEN_NULL;
    v.s = nullptr;
}
static int ben_parse_value(BenContext *c, BenValue *v);

int64_t ben_get_int(const BenValue *v) {
    assert(v != nullptr && v->type == BEN_INT);
    return v->n;
}
string ben_get_str(const BenValue *v) {
    assert(v != nullptr && v->type == BEN_STR);
    return *v->s;
}
static int64_t _parse_int(BenContext *c, BenValue *v) {
    _free(v);
    char *end;
    ++c->bencode;
    --c->len;
    v->n = strtoll(c->bencode, &end, 10);
    if (*end != 'e' ||
        ((v->n == LLONG_MAX || v->n == LLONG_MIN) && errno == ERANGE))
        return BEN_PARSE_INVALID_VALUE;
    v->type = BEN_INT;
    c->len -= end - c->bencode;
    c->bencode = end + 1;
    return BEN_PARSE_OK;
}
static int64_t _parse_str_raw(BenContext *c, string *s) {
    char *end;
    int64_t len = strtoll(c->bencode, &end, 10);
    c->len -= end - c->bencode;
    if (*end != ':' || len < 0 ||
        ((len == LLONG_MAX || len == LLONG_MIN) && errno == ERANGE) ||
        static_cast<int64_t>(c->len) < len)
        return BEN_PARSE_INVALID_VALUE;
    s->assign(end + 1, len);
    c->len -= len;
    c->bencode = end + len + 1;
    return BEN_PARSE_OK;
}
static int64_t _parse_str(BenContext *c, BenValue *v) {
    _free(v);
    v->type = BEN_STR;
    v->s = new string;
    return _parse_str_raw(c, v->s);
}
static int64_t _parse_list(BenContext *c, BenValue *v) {
    _free(v);
    ++c->bencode;
    --c->len;
    v->type = BEN_LIST;
    v->l = new vector<BenValue>;

    BenValue *child = new BenValue;
    int ret;
    while (*c->bencode != 'e' &&
           (ret = ben_parse_value(c, child)) == BEN_PARSE_OK)
        v->l->push_back(std::move(*child));
    delete child;
    child = nullptr;

    ++c->bencode;
    --c->len;
    return ret;
}
static int64_t _parse_dict(BenContext *c, BenValue *v) {
    _free(v);
    ++c->bencode;
    --c->len;
    v->type = BEN_DICT;
    v->d = new map<string, BenValue>;

    string key;
    BenValue *value = new BenValue;
    int ret = BEN_PARSE_OK;
    while (*c->bencode != 'e' &&
           (ret = _parse_str_raw(c, &key)) == BEN_PARSE_OK &&
           (ret = ben_parse_value(c, value)) == BEN_PARSE_OK)
        v->d->insert(std::make_pair(std::move(key), std::move(*value)));
    delete value;
    value = nullptr;

    ++c->bencode;
    --c->len;
    return ret;
}
static int ben_parse_value(BenContext *c, BenValue *v) {
    switch (*c->bencode) {
    case 'i':
        return _parse_int(c, v);
    case 'l':
        return _parse_list(c, v);
    case 'd':
        return _parse_dict(c, v);
    case '0' ... '9':
        return _parse_str(c, v);
    default:
        return BEN_PARSE_INVALID_VALUE;
    }
}
int64_t BenParse(BenValue *v, const char *bencode,size_t len) {
    BenContext c;
    assert(v != nullptr);
    c.bencode = bencode;
    c.len = len;
    return ben_parse_value(&c, v);
}
