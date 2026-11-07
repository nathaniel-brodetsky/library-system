#pragma once

#include <string>

#include "library_registries.h"

class БібліотечнийКонтролер {
public:
    РеєстрЧитачів реєстрЧитачів;
    РеєстрПримірників реєстрПримірників;
    РеєстрТранзакцій реєстрТранзакцій;

    std::string видатиКнигу(int idЧитача, int інвНомер, Дата дата);
};