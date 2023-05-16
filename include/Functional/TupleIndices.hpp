#include <cstdlib>
#include <tuple>

namespace proxy {
template <std::size_t...> struct TupleIndices {};

template <std::size_t Sp, class IntTuple, std::size_t Ep>
struct MakeIndicesImpl;

template <std::size_t Sp, std::size_t... Indices, std::size_t Ep>
struct MakeIndicesImpl<Sp, TupleIndices<Indices...>, Ep> {
  typedef
      typename MakeIndicesImpl<Sp + 1, TupleIndices<Indices..., Sp>, Ep>::type
          type;
};

template <std::size_t Ep, std::size_t... Indices>
struct MakeIndicesImpl<Ep, TupleIndices<Indices...>, Ep> {
  typedef TupleIndices<Indices...> type;
};

template <std::size_t Ep, std::size_t Sp = 0> struct MakeTupleIndices {
  typedef typename MakeIndicesImpl<Sp, TupleIndices<>, Ep>::type type;
};
} // namespace proxy
