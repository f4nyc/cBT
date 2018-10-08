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
        v->s = NULL;
        return;
    case BEN_LIST:
        delete v->l;
        v->l = NULL;
        return;
    case BEN_DICT:
        delete v->l;
        v->l = NULL;
        return;
    default:
        return;
    }
}
BenValue::~BenValue() { _free(this); }
BenValue::BenValue(const BenValue &v) : type(v.type) {
    switch (type) {
    case BEN_INT:
        n = v.n;
        break;
    case BEN_STR:
        s = new string(*v.s);
        break;
    case BEN_LIST:
        l = new vector<BenValue>(*v.l);
        break;
    }
}
BenValue::BenValue(BenValue &&v) : type(v.type) {
    switch (type) {
    case BEN_INT:
        n = v.n;
        break;
    default:
        s = v.s;
        v.s = nullptr;
        break;
    }
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
    c->bencode++;
    v->n = strtoll(c->bencode, &end, 10);
    if (*end != 'e' ||
        ((v->n == LLONG_MAX || v->n == LLONG_MIN) && errno == ERANGE))
        return BEN_PARSE_INVALID_VALUE;
    v->type = BEN_INT;
    c->bencode = end + 1;
    return BEN_PARSE_OK;
}

static int64_t _parse_str(BenContext *c, BenValue *v) {
    _free(v);
    char *end;
    int64_t len = strtoll(c->bencode, &end, 10);
    if (*end != ':' || len < 0 ||
        ((len == LLONG_MAX || len == LLONG_MIN) && errno == ERANGE) ||
        static_cast<int64_t>(strlen(end + 1)) < len)
        return BEN_PARSE_INVALID_VALUE;
    v->s = new string;
    v->s->assign(end + 1, len);
    c->bencode = end + len + 1;
    v->type = BEN_STR;
    return BEN_PARSE_OK;
}
static int64_t _parse_list(BenContext *c, BenValue *v) {
    _free(v);
    ++c->bencode;
    v->type = BEN_LIST;
    v->l = new vector<BenValue>;

    BenValue *child = new BenValue;
    int ret;
    while ((ret = ben_parse_value(c, child)) == BEN_PARSE_OK) {
        v->l->push_back(std::move(*child));
        delete child;
        child = nullptr;
        child = new BenValue;
    }
    delete child;
    child = nullptr;

    if (ret != BEN_PARSE_END)
        return ret;
    ++c->bencode;
    return BEN_PARSE_OK;
}
static int64_t _parse_dict(BenContext *c, BenValue *v) {}
static int ben_parse_value(BenContext *c, BenValue *v) {
    switch (*c->bencode) {
    case 'e':
        return BEN_PARSE_END;
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
int64_t BenParse(BenValue *v, const char *bencode) {
    BenContext c;
    assert(v != nullptr);
    c.bencode = bencode;
    return ben_parse_value(&c, v);
}
