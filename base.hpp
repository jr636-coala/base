#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <numeric>
#include <string_view>
#include <unordered_set>
#include <vector>
typedef uint64_t u64; typedef int64_t i64;
typedef uint32_t u32; typedef int32_t i32;
typedef uint16_t u16; typedef int16_t i16;
typedef uint8_t u8;   typedef int8_t i8;

template <>
struct std::hash<std::unordered_set<i32>> {
  std::size_t operator()(const std::unordered_set<i32>& k) const {
    return std::reduce(k.begin(), k.end(), 0, [](auto acc, auto x){ return acc + x; });
  }
};

template <typename T>
concept mvectorable = requires { requires sizeof(T) <= sizeof(T*); };

struct { size_t cpy {}; size_t mv {}; } stats;

template <mvectorable T>
struct mvector {
  mvector(const mvector& x) : data(nullptr) { *this = x; }
  mvector(mvector&& x) { *this = std::move(x); }
  mvector& operator=(const mvector& x) {
    ++stats.cpy;
    if (hasData()) free(data);
    _capacity = x._capacity;
    _size = x._size;
    if (x.hasValue()) val = x.val;
    else {
      data = static_cast<T*>(malloc(x._capacity * sizeof(T)));
      memcpy(data, x.data, x._size * sizeof(T));
    }
    return *this;
  }
  mvector& operator=(mvector&& x) {
    ++stats.mv;
    if (this != &x) {
      if (hasData()) free(data);
      data = x.data;
      _capacity = x._capacity;
      _size = x._size;
      x.data = nullptr;
    }
    return *this;
  }
  static const size_t INITIAL_CAPACITY = 1;
  mvector()
      : data(INITIAL_CAPACITY ? static_cast<T*>(malloc(INITIAL_CAPACITY * sizeof(T))) : nullptr), _capacity(INITIAL_CAPACITY), _size(0) {}
  ~mvector() { if (hasData()) free(data); }
  mvector(T val) : val(val), _capacity(std::numeric_limits<size_t>::max()) {}
  void push_back(T val) {
    if (_size == _capacity) expand();
    data[_size++] = val;
  }
  bool hasData() const { return !hasValue(); }
  bool hasValue() const { return _capacity == std::numeric_limits<size_t>::max(); }
  void expand() {
    const size_t ncap = (!_capacity + _capacity) * 2;
    const auto allocSize = ncap * sizeof(T);
    T* ndata = static_cast<T*>(realloc(data, allocSize));
    if (!ndata) {
      ndata = static_cast<T*>(malloc(allocSize));
      memcpy(ndata, data, sizeof(T) * _size);
      free(data);
    }
    data = ndata;
    _capacity = ncap;
  }
  size_t size() const { return _size; }
  struct iterator {
    using difference_type = std::ptrdiff_t;
    using element_type = T;

    iterator(T* ptr) : ptr(ptr) {}
    T& operator*() const { return *ptr; }
    auto& operator++() { ptr++; return *this; }
    auto operator++(int) { auto tmp = *this; ++(*this); return tmp; }
    auto operator<=>(const iterator&) const = default;
    T* ptr;
  };

  iterator begin() const { return iterator(data); }
  iterator end() const { return iterator(&data[_size]); }
  union { T* data; T val; };
  size_t _size;
  size_t _capacity;
};

template <typename T, T T_NULL, T (*TF)(T,T)>
struct RegexLexer {
  using transition_table_t = std::vector<std::unordered_map<char, mvector<i32>>>;
  using state_t = T;

  constexpr RegexLexer() : start(0), transitions({{}}), accept({}), states({ T_NULL }), alphabet({}) {}
  RegexLexer(RegexLexer&& x) { *this = std::move(x); }
  RegexLexer& operator=(RegexLexer&& x) {
    start = x.start;
    transitions = std::move(x.transitions);
    accept = std::move(x.accept);
    states = std::move(x.states);
    alphabet = std::move(x.alphabet);
    return *this;
  }
  RegexLexer(const RegexLexer& x) { *this = x; };
  RegexLexer& operator=(const RegexLexer& x) {
    start = x.start;
    transitions = x.transitions;
    accept = x.accept;
    states = x.states;
    alphabet = x.alphabet;
    return *this;
  }

  constexpr auto addState() {
    states.push_back(T_NULL);
    transitions.push_back({});
    /*if(transitions.size() != states.size())
      exit(-1);*/
    return states.size() - 1;
  }

  constexpr RegexLexer& concat(char c, const std::optional<T> t = {}) {
    const i32 ne = addState();
    if (t.has_value()) states[ne] = *t;
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n][c].push_back(ne); });
    if (!accept.size()) transitions[start][c].push_back(ne);
    accept = { ne };
    if (std::find(alphabet.begin(), alphabet.end(), c) == alphabet.end()) alphabet.push_back(c);
    return *this;
  }

  constexpr RegexLexer& concat(const RegexLexer& x) {
    if (!accept.size()) return *this = x;
    const auto maxx = states.size();
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n]['\0'].push_back(x.start + maxx); });
    std::copy_if(x.alphabet.begin(), x.alphabet.end(), std::back_inserter(alphabet), [&](auto c) {
      return std::find(alphabet.begin(), alphabet.end(), c) == alphabet.end();
    });
    transitions.reserve(states.size() + x.states.size());
    for (auto s1 = 0; s1 < x.states.size(); ++s1) {
      transitions.push_back({});
      const auto& m = x.transitions[s1];
      for (const auto& [c, v] : m) {
        auto& tr = transitions[s1 + maxx][c];
        for (const auto& s2 : v) tr.push_back(s2 + maxx);
      }
    }
    states.insert(states.end(), x.states.begin(), x.states.end());
    accept = x.accept;
    std::transform(accept.begin(), accept.end(), accept.begin(), [&](auto n) { return n + maxx; });
    return *this;
  }

  char escapeChar(char c) const {
    switch (c) {
      case 't': return '\t';
      case 'n': return '\n';
      default: return c;
    }
  }


  RegexLexer& concatClass(const std::string_view& str) {
    const i32 ne = addState();
    bool escape = false;
    if (str[0] == '^') {
      bool exclude[256] = {};
      const auto str2 = str.substr(1);
      for (auto c : str2) {
        if (!escape && c == '\\') {
          escape = true;
          continue;
        }
        if (escape) {
          escape = false;
          c = escapeChar(c);
        }
        exclude[c] = true;
      }
      for (auto c = 1; c < 256; ++c) {
        if (exclude[c]) continue;
        std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n][c].push_back(ne); });
        if (!accept.size()) transitions[start][c].push_back(ne);
        if (std::find(alphabet.begin(), alphabet.end(), c) == alphabet.end()) alphabet.push_back(c);
      }
    }
    else {
      for (auto c : str) {
        if (!escape && c == '\\') {
          escape = true;
          continue;
        }
        if (escape) {
          escape = false;
          c = escapeChar(c);
        }
        std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n][c].push_back(ne); });
        if (!accept.size()) transitions[start][c].push_back(ne);
        if (std::find(alphabet.begin(), alphabet.end(), c) == alphabet.end()) alphabet.push_back(c);
      }
    }
    accept = { ne };
    return *this;
  }

  constexpr RegexLexer& alter(const RegexLexer& x) { 
    if (!accept.size()) return *this = x;
    const auto maxx = states.size();
    transitions[start]['\0'].push_back(x.start + maxx);
    std::copy_if(x.alphabet.begin(), x.alphabet.end(), std::back_inserter(alphabet), [&](auto c) {
      return std::find(alphabet.begin(), alphabet.end(), c) == alphabet.end();
    });
    transitions.reserve(states.size() + x.states.size());
    for (auto s1 = 0; s1 < x.states.size(); ++s1) {
      transitions.push_back({});
      const auto& m = x.transitions[s1];
      for (const auto& [c, v] : m) {
        auto& tr = transitions[s1 + maxx][c];
        for (const auto& s2 : v) tr.push_back(s2 + maxx);
      }
    }
    states.insert(states.end(), x.states.begin(), x.states.end());
    std::transform(x.accept.begin(), x.accept.end(), std::back_inserter(accept), [&](auto n) { return n + maxx; });
    return *this; 
  }

  constexpr RegexLexer& close() {
    if (!accept.size()) return *this;
    const i32 ne = addState();
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n]['\0'].push_back(ne); });
    transitions[start]['\0'].push_back(ne);
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n]['\0'].push_back(start); });
    accept = { ne };
    return *this;
  }

  constexpr RegexLexer& optional() {
    if (!accept.size()) return *this;
    const i32 ne = addState();
    transitions[start]['\0'].push_back(ne);
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n]['\0'].push_back(ne); });
    accept = { ne };
    return *this;
  }

  constexpr RegexLexer& concatClose() {
    if (!accept.size()) return *this;
    const i32 ne = addState();
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n]['\0'].push_back(ne); });
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n]['\0'].push_back(start); });
    accept = { ne };
    return *this;
  }

  RegexLexer& dfa() {
    std::vector<std::vector<i32>> epsilonCache;
    epsilonCache.reserve(states.size());
    const std::function<std::vector<i32>(std::vector<i32>)> epsilonClosure = [&](auto s) {
      auto s2 = s;
      for (const auto n : s2) {
        if (epsilonCache.size() > n) {
          const auto& t2 = epsilonCache[n];
          std::copy_if(t2.begin(), t2.end(), std::back_inserter(s), [&](auto n) {
            for (const auto x : s) { if (x == n) return false; }
            return true;
          });
        }
        else {
          std::vector<i32> t2;
          const auto& tr = transitions[n]['\0'];
          t2.insert(t2.end(), tr.begin(), tr.end());
          t2 = epsilonClosure(t2);
          std::copy_if(t2.begin(), t2.end(), std::back_inserter(s), [&](auto n) {
            for (const auto x : s) { if (x == n) return false; }
            return true;
          });
        }
      }
      return s;
    };
    for (i32 i = 0; i < states.size(); ++i) epsilonCache.push_back(std::move(epsilonClosure({ i })));
    const auto epsilonCached = [&](std::unordered_set<i32>&& s) {
      std::vector<i32> s2(s.begin(), s.end());
      for (const auto& n : s2) {
        const auto& c = epsilonCache[n];
        s.insert(c.begin(), c.end());
      }
      return std::move(s);
    };

    const auto delta = [&](const auto& s, auto c) {
      std::unordered_set<i32> out;
      for (const auto n : s) {
        const auto& dc = transitions[n];
        const auto& dcc = dc.find(c);
        if (dcc != dc.end()) out.insert(dcc->second.begin(), dcc->second.end());
      }
      return out;
    };

    std::vector<std::unordered_set<i32>> ns, work = { epsilonCached({ start }) };
    std::vector<state_t> nstates = { T_NULL };
    start = 0;
    transition_table_t _transitions;

    std::unordered_map<std::unordered_set<i32>, i32> nsc;
    const auto getns = [&](const std::unordered_set<i32>& set) -> size_t {
      const auto& found = nsc.find(set);
      if (found != nsc.end()) return found->second;
      ns.push_back(set);
      const auto n = ns.size() - 1;
      nsc[set] = n;
      _transitions.push_back({});
      return n;
    };

    while (work.size()) {
      auto curr = work.back();
      work.pop_back();
      const auto nn = getns(curr);
      for (const auto c : alphabet) {
        std::unordered_set<i32> s = epsilonCached(delta(curr, c));
        if (!s.size()) continue;
        const auto beforeSize = ns.size();
        const auto to = getns(s);
        _transitions[nn][c] = { static_cast<i32>(to) };
        if (beforeSize != ns.size()) {
          nstates.push_back(std::reduce(s.begin(), s.end(), T_NULL, [&](auto acc, auto n) {
            return TF(acc, states[n]);
          }));
          work.push_back(std::move(s));
        }
      }
    }

    std::vector<i32> _accept = accept;
    accept.clear();
    for (auto i = 0; i < ns.size(); ++i) {
      if (std::find_if(_accept.begin(), _accept.end(), [&](auto n) { return ns[i].contains(n); }) != _accept.end())
        accept.push_back(i);
    }
    transitions = std::move(_transitions);
    states = std::move(nstates);
    return *this;
  }

  RegexLexer& minimise() {
    std::vector<std::unordered_set<i32>> p;
    std::vector<i32> group(states.size()); 
    p.push_back({accept.begin(), accept.end()});
    p.push_back({});
    for (auto i = 0; i < states.size(); ++i) {
      if (std::find(accept.begin(), accept.end(), i) == accept.end()) {
        group[i] = 1;
        p[1].insert(i);
      }
    }

    const auto indistinguishable = [&](auto s1, auto s2, auto c) {
      return group[transitions[s1].find(c)->second.val] == group[transitions[s2].find(c)->second.val];
    };

    auto k = 0;
    do {
      k = p.size();
      for (auto si = 0; si < p.size(); ++si) {
        auto& s = p[si];
        i32 s0 = (*s.begin());
        std::unordered_set<i32> indist = { s0 };
        const auto& ti = transitions[s0];
        for (const auto j : s) {
          if (j == s0) continue;
          bool ind = true;
          const auto& tj = transitions[j];
          if (states[s0] != states[j] || ti.size() != tj.size()) continue;
          for (const auto& [c, v] : ti) {
            if (!tj.contains(c) || !indistinguishable(s0, j, c)) { ind = false; break; }
          }
          if (ind) indist.insert(j);
        }
        if (indist.size() != s.size()) {
          std::erase_if(s, [&](auto n) { return indist.contains(n); });
          for (const auto& x : indist) group[x] = p.size();
          p.push_back({indist.begin(), indist.end()});
        }
      }
    } while (k != p.size());

    std::vector<state_t> nstates(k);
    transition_table_t _transitions(k);
    std::vector<i32> _accept;
    auto _start = 0;
    for (auto i = 0; i < k; ++i) {
      const auto s0 = *p[i].begin();
      for (const auto& [c, v] : transitions[s0]) {
        _transitions[i][c] = { static_cast<i32>(group[v.val]) };
      }
      nstates[i] = states[s0];
      auto accepted = false;
      for (const auto x : p[i]) {
        if (!accepted && std::find(accept.begin(), accept.end(), x) != accept.end()) {
          _accept.push_back(i);
          accepted = true;
        }
        if (x == start) _start = i;
      }
    }

    states = std::move(nstates);
    transitions = std::move(_transitions);
    accept = std::move(_accept);
    start = _start;

    return *this;
  }

  static RegexLexer parse(const std::string_view& str, T t) {
    auto i = 0;
    auto out = parse(str, i);
    std::for_each(out.accept.begin(), out.accept.end(), [&](auto n) { out.states[n] = t; });
    return out;
  }

  static RegexLexer parse(const std::string_view& str, auto& i) {
    const auto iscontrol = [&](auto c) {
      return c == '(' || c == ')' || c == '*' || c == '|' || c == '\\' || c == '+' || c == '?' || c == '[' || c == ']';
    };
    const auto parse_3 = [&]() {
      RegexLexer out;
      RegexLexer token;
      if (i >= str.length()) return out;
      do {
        if (!iscontrol(str[i])) {
          out.concat(token);
          token = std::move(RegexLexer().concat(str[i]));
          continue;
        }
        if (str[i] == '\\') {
          out.concat(token);
          token = std::move(RegexLexer().concat(str[++i]));
          continue;
        }
        if (str[i] == '[') {
          out.concat(token);
          const auto start = i + 1;
          while (str[++i] != ']' && str[i - 1] != '\\');
          token = std::move(RegexLexer().concatClass(str.substr(start, i - start)));
          continue;
        }
        if (str[i] == '?') {
          token.optional();
          continue;
        }
        if (str[i] == '+') {
          token.concatClose();
          continue;
        }
        if (str[i] == '*') {
          token.close();
          continue;
        }
        else if (str[i] == '(') {
          ++i;
          out.concat(token);
          token = std::move(parse(str, i));
          continue;
        }
        break;
      } while (++i < str.length());
      return std::move(out.concat(token));
    };
    const std::function<RegexLexer()> parse_ = [&]() {
      auto lhs = parse_3();
      if (i >= str.length()) return lhs;
      switch (str[i]) {
        case ')': return lhs;
        case '|': ++i; return lhs.alter(parse_());
      }
      return lhs;
    };
    return parse_();
  }
  T match(const std::string_view& str) const {
    auto curr = start;
    for (const auto c : str) {
      if (std::find(alphabet.begin(), alphabet.end(), c) == alphabet.end()) return T_NULL;
      const auto& next = transitions[curr].find(c);
      if (next == transitions[curr].end()) break;
      curr = next->second.val;
    }
    return states[curr];
  }

  std::vector<state_t> states;
  transition_table_t transitions;
  std::vector<char> alphabet;
  i32 start;
  std::vector<i32> accept;
};
