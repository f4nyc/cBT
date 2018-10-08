#ifndef BENCODE_H_
#define BENCODE_H_

#include <string>
#include <vector>
using std::string;
using std::vector;
enum BenType { BEN_NULL, BEN_STR, BEN_INT, BEN_LIST, BEN_DICT };
enum {
    BEN_PARSE_OK = 0,
    BEN_PARSE_END,
    BEN_PARSE_EXPECT_VALUE,
    BEN_PARSE_INVALID_VALUE
};
struct BenValue {
    BenType type = BEN_NULL;
    union {
        int64_t n;
        string *s;
        vector<BenValue> *l;
    };
    BenValue() = default;
    BenValue(const BenValue& v);
    BenValue(BenValue&& v);
    ~BenValue();
};
struct BenContext {
    const char *bencode;
};

int64_t BenParse(BenValue *v, const char *bencode);
BenType ben_get_type(const BenValue *v);
int64_t ben_get_int(const BenValue *v);
string ben_get_str(const BenValue *v);

#endif /* ifndef BENCODE_H_ */