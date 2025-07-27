#include <Blacklist.h>
#include <iostream>


Blacklist::Blacklist(const std::vector<std::string>& blacklistedImsis) {
    setBlacklist(blacklistedImsis);
}

bool Blacklist::isBlacklisted(const std::string& imsi) const {
    return _blacklistedImsis.contains(imsi);
}

void Blacklist::setBlacklist(const std::vector<std::string>& blacklistedImsis) {
    _blacklistedImsis.clear();
    
    // Резервируем память для уменьшения перераспределения
    _blacklistedImsis.reserve(blacklistedImsis.size());
    
    // Вставляем все IMSI за одну операцию
    _blacklistedImsis.insert(blacklistedImsis.begin(), blacklistedImsis.end());
}