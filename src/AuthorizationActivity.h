#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <string>

class AuthorizationActivity
{
public:
    struct Entry
    {
        uint32_t authorizationId = 0;
        uint32_t index = 0;
        std::string name;
        std::string dateLastActive;
        std::string action;
        std::string trigger;
        std::string completionStatus;
    };

    bool observe(const Entry& entry, size_t maxEntries)
    {
        if(maxEntries == 0 || entry.authorizationId == 0 ||
                entry.authorizationId == std::numeric_limits<uint32_t>::max() ||
                entry.index == 0 || entry.name.find_first_not_of(" \t\r\n") == std::string::npos ||
                _listed.count(entry.authorizationId) != 0)
        {
            return false;
        }

        const auto existing = _entries.find(entry.authorizationId);
        if(existing != _entries.end())
        {
            if(entry.index <= existing->second.index)
            {
                return false;
            }
            existing->second = entry;
            return true;
        }

        // Preserve existing sensors when the observation limit is reached.
        if(_entries.size() >= maxEntries)
        {
            return false;
        }

        _entries.emplace(entry.authorizationId, entry);
        return true;
    }

    void updateListed(const std::set<uint32_t>& authorizationIds)
    {
        _listed = authorizationIds;
        _listReceived = true;
        for(auto entry = _entries.begin(); entry != _entries.end();)
        {
            if(_listed.count(entry->first) != 0)
            {
                entry = _entries.erase(entry);
            }
            else
            {
                ++entry;
            }
        }
    }

    const std::map<uint32_t, Entry>& entries() const
    {
        return _entries;
    }

    bool listReceived() const
    {
        return _listReceived;
    }

    bool isListed(uint32_t authorizationId) const
    {
        return _listed.count(authorizationId) != 0;
    }

    void clear()
    {
        _entries.clear();
        _listed.clear();
        _listReceived = false;
    }

private:
    std::map<uint32_t, Entry> _entries;
    std::set<uint32_t> _listed;
    bool _listReceived = false;
};
