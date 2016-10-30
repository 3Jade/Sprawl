#include "../if/if.hpp"
#include "../tag/tag.hpp"

template<int i>
using IntToString = IF(i == 1, SPRAWL_TAG("one"))
					ELSEIF(i == 2, SPRAWL_TAG("two"))
					ELSEIF(i == 3, SPRAWL_TAG("three"))
					ELSEIF(i == 4, SPRAWL_TAG("four"))
					ELSEIF(i == 5, SPRAWL_TAG("five"))
					ELSEIF(i == 6, SPRAWL_TAG("six"))
					ELSEIF(i == 7, SPRAWL_TAG("seven"))
					ELSEIF(i == 8, SPRAWL_TAG("eight"))
					ELSEIF(i == 9, SPRAWL_TAG("nine"))
					ELSE(SPRAWL_TAG("Unknown?"))
					ENDIF;

static_assert(IntToString<1>::EqualTo("one"), "if fail");
static_assert(IntToString<5>::EqualTo("five"), "if fail");
static_assert(IntToString<9>::EqualTo("nine"), "if fail");
static_assert(IntToString<231>::EqualTo("Unknown?"), "if fail");